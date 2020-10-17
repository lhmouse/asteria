// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "simple_script.hpp"
#include "compiler/token_stream.hpp"
#include "compiler/statement_sequence.hpp"
#include "compiler/statement.hpp"
#include "compiler/expression_unit.hpp"
#include "runtime/air_optimizer.hpp"
#include "llds/reference_stack.hpp"
#include "util.hpp"

namespace asteria {

Simple_Script&
Simple_Script::
reload(const cow_string& name, int line, tinybuf& cbuf)
  {
    // Initialize the parameter list.
    // This is the same for all scripts so we only do this once.
    if(ROCKET_UNEXPECT(this->m_params.empty()))
      this->m_params.emplace_back(::rocket::sref("..."));

    // Parse source code.
    Token_Stream tstrm(this->m_opts);
    tstrm.reload(name, line, cbuf);

    Statement_Sequence stmtq(this->m_opts);
    stmtq.reload(tstrm);

    // Instantiate the function.
    AIR_Optimizer optmz(this->m_opts);
    optmz.reload(nullptr, this->m_params, stmtq);
    this->m_func = optmz.create_function(Source_Location(name, 0, 0),
                                         ::rocket::sref("[file scope]"));
    return *this;
  }

Simple_Script&
Simple_Script::
reload_string(const cow_string& name, const cow_string& code)
  {
    return this->reload_string(name, 1, code);
  }

Simple_Script&
Simple_Script::
reload_string(const cow_string& name, int line, const cow_string& code)
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(code, tinybuf::open_read);
    return this->reload(name, line, cbuf);
  }

Simple_Script&
Simple_Script::
reload_stdin()
  {
    return this->reload_stdin(1);
  }

Simple_Script&
Simple_Script::
reload_stdin(int line)
  {
    // Initialize a stream using `stdin`.
    ::rocket::tinybuf_file cbuf;
    cbuf.reset(stdin, nullptr);
    return this->reload(::rocket::sref("[stdin]"), line, cbuf);
  }

Simple_Script&
Simple_Script::
reload_file(const char* path)
  {
    // Resolve the path to an absolute one.
    auto abspath = ::rocket::make_unique_handle(::realpath(path, nullptr), ::free);
    if(!abspath)
      ASTERIA_THROW("Could not open script file '$2'\n"
                    "[`realpath()` failed: $1]",
                    format_errno(errno), path);

    // Open the file denoted by this path.
    ::rocket::tinybuf_file cbuf;
    cbuf.open(abspath, tinybuf::open_read);
    return this->reload(cow_string(abspath), 1, cbuf);
  }

Reference
Simple_Script::
execute(Global_Context& global, Reference_Stack&& stack)
  const
  {
    // Execute the script as a plain function.
    const StdIO_Sentry sentry;
    return this->m_func.invoke(global, ::std::move(stack));
  }

Reference
Simple_Script::
execute(Global_Context& global, cow_vector<Value>&& vals)
  const
  {
    // Push all arguments backwards as temporaries.
    Reference_Stack stack;
    for(auto it = vals.mut_rbegin();  it != vals.rend();  ++it) {
      Reference::S_temporary xref = { ::std::move(*it) };
      stack.emplace_back(::std::move(xref));
    }
    return this->execute(global, ::std::move(stack));
  }

}  // namespace asteria
