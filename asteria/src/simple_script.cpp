// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "simple_script.hpp"
#include "compiler/token_stream.hpp"
#include "compiler/statement_sequence.hpp"
#include "runtime/air_optimizer.hpp"
#include "utilities.hpp"

namespace Asteria {

Simple_Script&
Simple_Script::
reload(tinybuf& cbuf, const cow_string& name)
  {
    // Initialize the parameter list. This is the same for all scripts so we only do this once.
    if(ROCKET_UNEXPECT(this->m_params.empty()))
      this->m_params.emplace_back(::rocket::sref("..."));

    // Parse source code.
    Token_Stream tstrm(this->m_opts);
    tstrm.reload(cbuf, name);

    Statement_Sequence stmtq(this->m_opts);
    stmtq.reload(tstrm);

    // Instantiate the function.
    AIR_Optimizer optmz(this->m_opts);
    optmz.reload(nullptr, this->m_params, stmtq);
    this->m_func = optmz.create_function(Source_Location(name, 0, 0), ::rocket::sref("<file scope>"));
    return *this;
  }

Simple_Script&
Simple_Script::
reload_string(const cow_string& code, const cow_string& name)
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(code, tinybuf::open_read);
    return this->reload(cbuf, name);
  }

Simple_Script&
Simple_Script::
reload_file(const char* path)
  {
    // Resolve the path to an absolute one.
    uptr<char, void (&)(void*)> abspath(::realpath(path, nullptr), ::free);
    if(!abspath)
      ASTERIA_THROW_SYSTEM_ERROR("realpath");

    // Open the file denoted by this path.
    ::rocket::tinybuf_file cbuf;
    cbuf.open(abspath, tinybuf::open_read);
    return this->reload(cbuf, cow_string(abspath));
  }

Simple_Script&
Simple_Script::
reload_stdin()
  {
    // Initialize a stream using `stdin`.
    ::rocket::tinybuf_file cbuf;
    cbuf.reset(stdin, nullptr);
    return this->reload(cbuf, ::rocket::sref("<stdin>"));
  }

Reference
Simple_Script::
execute(Global_Context& global, cow_vector<Reference>&& args)
const
  {
    if(!this->m_func)
      ASTERIA_THROW("no script loaded");

    // Execute the script as a plain function.
    const StdIO_Sentry sentry;
    return this->m_func.invoke(global, ::std::move(args));
  }

Reference
Simple_Script::
execute(Global_Context& global, cow_vector<Value>&& vals)
const
  {
    // Convert all arguments to references to temporaries.
    cow_vector<Reference> args;
    args.reserve(vals.size());
    for(size_t i = 0;  i < vals.size();  ++i) {
      Reference_root::S_temporary xref = { ::std::move(vals.mut(i)) };
      args.emplace_back(::std::move(xref));
    }
    return this->execute(global, ::std::move(args));
  }

}  // namespace Asteria
