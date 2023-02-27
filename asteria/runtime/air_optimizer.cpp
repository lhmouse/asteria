// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "air_optimizer.hpp"
#include "analytic_context.hpp"
#include "instantiated_function.hpp"
#include "enums.hpp"
#include "../compiler/statement.hpp"
#include "../compiler/expression_unit.hpp"
#include "../utils.hpp"
namespace asteria {

AIR_Optimizer::
~AIR_Optimizer()
  {
  }

void
AIR_Optimizer::
reload(Abstract_Context* ctx_opt, const cow_vector<phsh_string>& params,
       const Global_Context& global, const cow_vector<Statement>& stmts)
  {
    this->m_code.clear();
    this->m_params = params;

    if(stmts.empty())
      return;

    // Create a context of the function body.
    Analytic_Context ctx_func(Analytic_Context::M_function(), ctx_opt, this->m_params);

    // Generate code for all statements.
    for(size_t k = 0;  k + 1 < stmts.size();  ++k)
      stmts.at(k).generate_code(this->m_code, nullptr, global, ctx_func, this->m_opts,
              stmts.at(k + 1).is_empty_return() ? ptc_aware_void : ptc_aware_none);

    stmts.back().generate_code(this->m_code, nullptr, global, ctx_func, this->m_opts,
            ptc_aware_void);

    // Check whether optimization is enabled during translation.
    if(this->m_opts.optimization_level < 2)
      return;

    // TODO: Insert optimization passes
  }

void
AIR_Optimizer::
rebind(Abstract_Context* ctx_opt, const cow_vector<phsh_string>& params,
       const cow_vector<AIR_Node>& code)
  {
    this->m_code = code;
    this->m_params = params;

    if(code.empty())
      return;

    // Create a context of the function body.
    Analytic_Context ctx_func(Analytic_Context::M_function(),
                              ctx_opt, this->m_params);

    // Rebind all nodes recursively.
    // Don't trigger copy-on-write unless a node needs rewriting.
    for(size_t k = 0;  k < code.size();  ++k)
      if(auto qnode = code.at(k).rebind_opt(ctx_func))
        this->m_code.mut(k) = ::std::move(*qnode);

    // Check whether optimization is enabled during execution.
    if(this->m_opts.optimization_level < 3)
      return;

    // TODO: Insert optimization passes
  }

cow_function
AIR_Optimizer::
create_function(const Source_Location& sloc, stringR name)
  {
    // Compose the function signature.
    // We only do this if `name` really looks like a function name.
    cow_string func = name;
    if(is_cmask(name.front(), cmask_namei) && (name.back() != ')')) {
      func << '(';
      if(this->m_params.size()) {
        func << this->m_params[0];
        for(size_t k = 1;  k < this->m_params.size();  ++k)
          func << ", " << this->m_params[k];
      }
      func << ')';
    }

    // Instantiate the function.
    return ::rocket::make_refcnt<Instantiated_Function>(this->m_params,
               ::rocket::make_refcnt<Variadic_Arguer>(sloc, ::std::move(func)),
               this->m_code);
  }

}  // namespace asteria
