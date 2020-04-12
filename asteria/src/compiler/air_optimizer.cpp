// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_optimizer.hpp"
#include "statement.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../utilities.hpp"

namespace Asteria {

AIR_Optimizer::
~AIR_Optimizer()
  {
  }

AIR_Optimizer&
AIR_Optimizer::
reload(const Abstract_Context* ctx_opt, const cow_vector<phsh_string>& params,
       const cow_vector<Statement>& stmts)
  {
    this->m_code.clear();
    this->m_params = params;

    // Generate code for all statements.
    Analytic_Context ctx_func(ctx_opt, this->m_params);
    for(size_t i = 0;  i < stmts.size();  ++i) {
      stmts[i].generate_code(this->m_code, nullptr, ctx_func, this->m_opts,
                     ((i + 1 == stmts.size()) || stmts.at(i + 1).is_empty_return())
                          ? ptc_aware_void : ptc_aware_none);
    }

    // TODO: Insert optimization passes
    return *this;
  }

AIR_Optimizer&
AIR_Optimizer::
rebind(const Abstract_Context* ctx_opt, const cow_vector<phsh_string>& params,
       const cow_vector<AIR_Node>& code)
  {
    this->m_code = code;
    this->m_params = params;

    // Rebind all nodes recursively.
    // Don't trigger copy-on-write unless a node needs rewriting.
    Analytic_Context ctx_func(ctx_opt, params);
    for(size_t i = 0;  i < code.size();  ++i) {
      auto qnode = code.at(i).rebind_opt(ctx_func);
      if(!qnode)
        continue;
      this->m_code.mut(i) = ::std::move(*qnode);
    }

    // TODO: Insert optimization passes
    return *this;
  }

cow_function
AIR_Optimizer::
create_function(const Source_Location& sloc, const cow_string& name)
  {
    // Append the parameter list to `name`.
    // We only do this if `name` really looks like a function name.
    cow_string func = name;
    if((func.size() != 0) && (func.front() != '<') && (func.back() != '>')) {
      func << '(';
      for(const auto& param : this->m_params)
        func << param << ", ";
      if(!this->m_params.empty())
        func.erase(func.size() - 2);
      func << ')';
    }

    // Instantiate the function.
    return ::rocket::make_refcnt<Instantiated_Function>(this->m_params,
                             ::rocket::make_refcnt<Variadic_Arguer>(sloc, func),
                             this->m_code);
  }

}  // namespace Asteria
