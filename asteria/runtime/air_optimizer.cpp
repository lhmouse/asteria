// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "air_optimizer.hpp"
#include "air_node.hpp"
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
clear() noexcept
  {
    this->m_code.clear();
  }

void
AIR_Optimizer::
reload(const Abstract_Context* ctx_opt, const cow_vector<phsh_string>& params,
       const Global_Context& global, const cow_vector<Statement>& stmts)
  {
    this->m_code.clear();
    this->m_params = params;

    if(stmts.empty())
      return;

    // Generate code for the function body.
    Analytic_Context ctx_func(xtc_function, ctx_opt, this->m_params);

    for(size_t i = 0;  i < stmts.size();  ++i)
      stmts.at(i).generate_code(this->m_code, ctx_func, nullptr, global, this->m_opts,
                    ((i != stmts.size() - 1) && !stmts.at(i + 1).is_empty_return())
                    ? ptc_aware_none : ptc_aware_void);

    if(this->m_opts.optimization_level <= 0)
      return;

    // TODO: Insert optimization passes
  }

void
AIR_Optimizer::
rebind(const Abstract_Context* ctx_opt, const cow_vector<phsh_string>& params,
       const cow_vector<AIR_Node>& code)
  {
    this->m_code = code;
    this->m_params = params;

    if(code.empty())
      return;

    // Rebind all nodes recursively.
    Analytic_Context ctx_func(xtc_function, ctx_opt, this->m_params);

    for(size_t k = 0;  k < code.size();  ++k)
      if(auto qnode = code.at(k).rebind_opt(ctx_func))
        this->m_code.mut(k) = move(*qnode);
  }

cow_function
AIR_Optimizer::
create_function(const Source_Location& sloc, cow_stringR name)
  {
    cow_string func = name;
    if(is_cmask(func.front(), cmask_namei) && is_cmask(func.back(), cmask_name)) {
      // If `name` looks like a function name, append the parameter list
      // to form a function signature.
      func << "(";
      uint32_t off = 0;
      switch(this->m_params.size())
        {
          do {
            func << ", ";  // fallthrough
        default:
            func << this->m_params[off];
          } while(++off != this->m_params.size());  // fallthrough
        case 0:
          break;
        }
      func << ")";
    }

    // Instantiate the function object now.
    return ::rocket::make_refcnt<Instantiated_Function>(
                       sloc, func, this->m_params, this->m_code);
  }

}  // namespace asteria
