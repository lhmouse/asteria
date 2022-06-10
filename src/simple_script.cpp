// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "precompiled.ipp"
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
reload(const cow_string& name, Statement_Sequence&& stmtq)
  {
    // Initialize the argument list. This is done only once.
    if(this->m_params.empty())
      this->m_params.emplace_back(sref("..."));

    // Instantiate the function.
    AIR_Optimizer optmz(this->m_opts);
    optmz.reload(nullptr, this->m_params, this->m_global, stmtq);

    Source_Location sloc(name, 0, 0);
    this->m_func = optmz.create_function(sloc, sref("[file scope]"));
    return *this;
  }

Simple_Script&
Simple_Script::
reload(const cow_string& name, Token_Stream&& tstrm)
  {
    Statement_Sequence stmtq(this->m_opts);
    stmtq.reload(::std::move(tstrm));
    return this->reload(name, ::std::move(stmtq));
  }

Simple_Script&
Simple_Script::
reload(const cow_string& name, int line, tinybuf&& cbuf)
  {
    Token_Stream tstrm(this->m_opts);
    tstrm.reload(name, line, ::std::move(cbuf));
    return this->reload(name, ::std::move(tstrm));
  }

Simple_Script&
Simple_Script::
reload_string(const cow_string& name, int line, const cow_string& code)
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(code, tinybuf::open_read);
    return this->reload(name, line, ::std::move(cbuf));
  }

Simple_Script&
Simple_Script::
reload_string(const cow_string& name, const cow_string& code)
  {
    return this->reload_string(name, 1, code);
  }

Simple_Script&
Simple_Script::
reload_stdin(int line)
  {
    ::rocket::tinybuf_file cbuf;
    cbuf.reset(stdin, nullptr);
    return this->reload(sref("[stdin]"), line, ::std::move(cbuf));
  }

Simple_Script&
Simple_Script::
reload_stdin()
  {
    return this->reload_stdin(1);
  }

Simple_Script&
Simple_Script::
reload_file(const char* path)
  {
    ::rocket::unique_ptr<char, void (void*)> abspath(::free);
    abspath.reset(::realpath(path, nullptr));
    if(!abspath)
      ASTERIA_THROW((
          "Could not open script file '$2'",
          "[`realpath()` failed: $1]"),
          format_errno(), path);

    ::rocket::tinybuf_file cbuf;
    cbuf.open(abspath, tinybuf::open_read);
    return this->reload(cow_string(abspath), 1, ::std::move(cbuf));
  }

Simple_Script&
Simple_Script::
reload_file(const cow_string& path)
  {
    return this->reload_file(path.safe_c_str());
  }

Reference
Simple_Script::
execute(Reference_Stack&& stack)
  {
    Reference self;
    self.set_temporary(nullopt);
    this->m_func.invoke(self, this->m_global, ::std::move(stack));
    ::fflush(nullptr);
    return self;
  }

Reference
Simple_Script::
execute(cow_vector<Value>&& args)
  {
    Reference_Stack stack;
    for(auto it = args.mut_begin();  it != args.end(); ++it)
      stack.push().set_temporary(::std::move(*it));
    return this->execute(::std::move(stack));
  }

Reference
Simple_Script::
execute()
  {
    Reference_Stack stack;
    return this->execute(::std::move(stack));
  }

}  // namespace asteria
