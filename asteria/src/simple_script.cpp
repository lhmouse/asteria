// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "simple_script.hpp"
#include "compiler/token_stream.hpp"
#include "compiler/statement_sequence.hpp"
#include "compiler/statement.hpp"
#include "compiler/expression_unit.hpp"
#include "runtime/air_optimizer.hpp"
#include "llds/reference_stack.hpp"
#include "utils.hpp"

namespace asteria {

Simple_Script&
Simple_Script::
reload(const cow_string& name, int line, tinybuf& cbuf)
  {
    // Parse source code.
    Token_Stream tstrm(this->m_opts);
    tstrm.reload(name, line, cbuf);

    Statement_Sequence stmtq(this->m_opts);
    stmtq.reload(tstrm);

    // Initialize the argument list. This is done only once.
    if(this->m_params.empty())
      this->m_params.emplace_back(sref("..."));

    // Instantiate the function.
    Source_Location sloc(name, 0, 0);
    AIR_Optimizer optmz(this->m_opts);
    optmz.reload(nullptr, this->m_params, this->m_global, stmtq);

    this->m_func = optmz.create_function(sloc, sref("[file scope]"));
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
    return this->reload(sref("[stdin]"), line, cbuf);
  }

Simple_Script&
Simple_Script::
reload_file(const char* path)
  {
    // Resolve the path to an absolute one.
    auto abspath = ::rocket::make_unique_handle(::realpath(path, nullptr), ::free);
    if(!abspath)
      ASTERIA_THROW("could not open script file '$2'\n"
                    "[`realpath()` failed: $1]",
                    format_errno(), path);

    // Open the file denoted by this path.
    ::rocket::tinybuf_file cbuf;
    cbuf.open(abspath, tinybuf::open_read);
    return this->reload(cow_string(abspath), 1, cbuf);
  }

Reference
Simple_Script::
execute(Reference_Stack&& stack)
  {
    // Execute the script as a plain function.
    Reference self;
    self.set_temporary(nullopt);

    const STDIO_Sentry sentry;
    this->m_func.invoke(self, this->m_global, ::std::move(stack));
    return self;
  }

Reference
Simple_Script::
execute(cow_vector<Value>&& vals)
  {
    // Push all arguments backwards as temporaries.
    Reference_Stack stack;
    for(auto it = vals.mut_begin();  it != vals.end();  ++it)
      stack.push().set_temporary(::std::move(*it));

    return this->execute(::std::move(stack));
  }

}  // namespace asteria
