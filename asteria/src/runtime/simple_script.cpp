// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "simple_script.hpp"
#include "air_node.hpp"
#include "analytic_context.hpp"
#include "instantiated_function.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/statement_sequence.hpp"
#include "../utilities.hpp"

namespace Asteria {

Simple_Script& Simple_Script::reload(tinybuf& cbuf, const cow_string& name)
  {
    // Tokenize the character stream.
    Token_Stream tstrm;
    tstrm.reload(cbuf, name, this->m_opts);
    // Parse tokens.
    Statement_Sequence stmtq;
    stmtq.reload(tstrm, this->m_opts);

    // Initialize the parameter list. This is the same for all scripts so we only do this once.
    if(ROCKET_UNEXPECT(this->m_params.empty())) {
      this->m_params.emplace_back(::rocket::sref("..."));
    }
    // Create the zero-ary argument getter.
    auto zvarg = ::rocket::make_refcnt<Variadic_Arguer>(name, 1, ::rocket::sref("<top level>"));
    // Generate IR nodes for the function body.
    // TODO: Move this elsewhere.
    cow_vector<AIR_Node> code_body;
    size_t epos = stmtq.size() - 1;
    if(epos != SIZE_MAX) {
      Analytic_Context ctx_func(nullptr, this->m_params);
      // Generate code with regard to proper tail calls.
      for(size_t i = 0;  i < epos;  ++i) {
        stmtq.at(i).generate_code(code_body, nullptr, ctx_func, this->m_opts,
                                  stmtq.at(i + 1).is_empty_return() ? ptc_aware_prune : ptc_aware_none);
      }
      stmtq.at(epos).generate_code(code_body, nullptr, ctx_func, this->m_opts, ptc_aware_prune);
    }
    // TODO: Insert optimization passes.
    // Instantiate the function.
    this->m_cptr = ::rocket::make_refcnt<Instantiated_Function>(this->m_params, ::rocket::move(zvarg), code_body);
    return *this;
  }

Simple_Script& Simple_Script::reload_string(const cow_string& code, const cow_string& name)
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(code, tinybuf::open_read);
    return this->reload(cbuf, name);
  }

Simple_Script& Simple_Script::reload_file(const cow_string& path)
  {
    ::rocket::tinybuf_file cbuf;
    cbuf.open(path.c_str(), tinybuf::open_read);
    return this->reload(cbuf, path);
  }

Simple_Script& Simple_Script::reload_stdin()
  {
    ::rocket::tinybuf_file cbuf;
    cbuf.reset(stdin, nullptr);
    return this->reload(cbuf, ::rocket::sref("<stdin>"));
  }

rcptr<Abstract_Function> Simple_Script::copy_function_opt() const noexcept
  {
    return ::rocket::dynamic_pointer_cast<Abstract_Function>(this->m_cptr);
  }

Reference Simple_Script::execute(Global_Context& global, cow_vector<Reference>&& args) const
  {
    auto qtarget = this->copy_function_opt();
    if(!qtarget) {
      ASTERIA_THROW("no script loaded");
    }
    Reference self = Reference_Root::S_constant();
    qtarget->invoke(self, global, ::rocket::move(args));
    self.finish_call(global);
    return self;
  }

Reference Simple_Script::execute(Global_Context& global, cow_vector<Value>&& vals) const
  {
    cow_vector<Reference> args;
    args.reserve(vals.size());
    for(size_t i = 0;  i < args.size();  ++i) {
      Reference_Root::S_temporary xref = { ::rocket::move(vals.mut(i)) };
      args.emplace_back(::rocket::move(xref));
    }
    return this->execute(global, ::rocket::move(args));
  }

Reference Simple_Script::execute(Global_Context& global) const
  {
    cow_vector<Reference> args;
    return this->execute(global, ::rocket::move(args));
  }

}  // namespace Asteria
