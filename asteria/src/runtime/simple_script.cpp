// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "simple_script.hpp"
#include "air_node.hpp"
#include "analytic_context.hpp"
#include "instantiated_function.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/statement_sequence.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

struct StdIO_Sentry
  {
    StdIO_Sentry()
      {
        if(!::freopen(nullptr, "r", stdin))
          ::std::terminate();
        if(!::freopen(nullptr, "w", stdout))
          ::std::terminate();
      }
    ~StdIO_Sentry()
      {
        ::fflush(nullptr);
      }

    StdIO_Sentry(const StdIO_Sentry&)
      = delete;
    StdIO_Sentry& operator=(const StdIO_Sentry&)
      = delete;
  };

}  // namespace

Simple_Script& Simple_Script::reload(tinybuf& cbuf, const cow_string& name)
  {
    // Tokenize the character stream.
    Token_Stream tstrm;
    tstrm.reload(cbuf, name, this->m_opts);
    // Parse tokens.
    Statement_Sequence stmtq;
    stmtq.reload(tstrm, this->m_opts);

    // Initialize the parameter list. This is the same for all scripts so we only do this once.
    if(ROCKET_UNEXPECT(this->m_params.empty()))
      this->m_params.emplace_back(::rocket::sref("..."));

    // Generate IR nodes for the function body.
    // TODO: Move this elsewhere.
    cow_vector<AIR_Node> code_body;
    Analytic_Context ctx_func(nullptr, this->m_params);
    for(size_t i = 0;  i < stmtq.size();  ++i)
      stmtq.at(i).generate_code(code_body, nullptr, ctx_func, this->m_opts,
                     ((i + 1 == stmtq.size()) || stmtq.at(i + 1).is_empty_return())
                          ? ptc_aware_void     // last or preceding empty return
                          : ptc_aware_none);
    // TODO: Insert optimization passes.
    // Instantiate the function.
    this->m_func = ::rocket::make_refcnt<Instantiated_Function>(
                          this->m_params,
                          ::rocket::make_refcnt<Variadic_Arguer>(name, 0, ::rocket::sref("<top level>")),
                          code_body);
    return *this;
  }

Simple_Script& Simple_Script::reload_string(const cow_string& code, const cow_string& name)
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(code, tinybuf::open_read);
    return this->reload(cbuf, name);
  }

Simple_Script& Simple_Script::reload_file(const char* path)
  {
    // Resolve the path to an absolute one.
    ::rocket::unique_ptr<char, void (&)(void*)> abspath(::realpath(path, nullptr), ::free);
    if(!abspath)
      ASTERIA_THROW_SYSTEM_ERROR("realpath");
    // Open the file denoted by this path.
    ::rocket::tinybuf_file cbuf;
    cbuf.open(abspath.get(), tinybuf::open_read);
    return this->reload(cbuf, cow_string(abspath.get()));
  }

Simple_Script& Simple_Script::reload_stdin()
  {
    // Initialize a stream using `stdin`.
    ::rocket::tinybuf_file cbuf;
    cbuf.reset(stdin, nullptr);
    return this->reload(cbuf, ::rocket::sref("<stdin>"));
  }

Reference Simple_Script::execute(Global_Context& global, cow_vector<Reference>&& args) const
  {
    if(!this->m_func) {
      ASTERIA_THROW("no script loaded");
    }
    const StdIO_Sentry iocerb;
    return this->m_func.invoke(global, ::std::move(args));
  }

Reference Simple_Script::execute(Global_Context& global, cow_vector<Value>&& vals) const
  {
    cow_vector<Reference> args;
    args.reserve(vals.size());
    for(size_t i = 0;  i < args.size();  ++i) {
      Reference_root::S_temporary xref = { ::std::move(vals.mut(i)) };
      args.emplace_back(::std::move(xref));
    }
    return this->execute(global, ::std::move(args));
  }

}  // namespace Asteria
