// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "instantiated_function.hpp"
#include "air_node.hpp"
#include "evaluation_stack.hpp"
#include "executive_context.hpp"
#include "analytic_context.hpp"
#include "../syntax/statement.hpp"
#include "../utilities.hpp"

namespace Asteria {

Instantiated_Function::~Instantiated_Function()
  {
  }

rcobj<Variadic_Arguer> Instantiated_Function::do_create_zvarg(const Source_Location& sloc, const cow_string& name) const
  {
    // Create a zero-ary argument getter, which also stores the source location and prototype string.
    cow_string func;
    func << name << '(';
    auto epos = this->m_params.size() - 1;
    if(epos != SIZE_MAX) {
      for(size_t i = 0; i != epos; ++i) {
        func << this->m_params[i] << ", ";
      }
      func << this->m_params[epos];
    }
    func << ')';
    return rocket::make_refcnt<Variadic_Arguer>(sloc, rocket::move(func));
  }

AVMC_Queue Instantiated_Function::do_generate_code(const Compiler_Options& options, const Abstract_Context* ctx_opt, const cow_vector<Statement>& stmts) const
  {
    // Generate IR nodes for the function body.
    cow_vector<AIR_Node> code_func;
    Analytic_Context ctx_func(rocket::ref(ctx_opt), this->m_params);
    auto epos = stmts.size() - 1;
    if(epos != SIZE_MAX) {
      // Statements other than the last one cannot be the end of function.
      for(size_t i = 0; i != epos; ++i) {
        stmts[i].generate_code(code_func, nullptr, ctx_func, options, stmts[i+1].is_empty_return() ? tco_aware_nullify : tco_aware_none);
      }
      // The last statement may be TCO'd.
      stmts[epos].generate_code(code_func, nullptr, ctx_func, options, tco_aware_nullify);
    }
    // TODO: Insert optimization passes here.
    // Solidify IR nodes.
    AVMC_Queue queue;
    rocket::for_each(code_func, [&](const AIR_Node& node) { node.solidify(queue, 0);  });  // 1st pass: Reserve storage.
    rocket::for_each(code_func, [&](const AIR_Node& node) { node.solidify(queue, 1);  });  // 2nd pass: Construct nodes.
    return queue;
  }

std::ostream& Instantiated_Function::describe(std::ostream& ostrm) const
  {
    return ostrm << this->m_zvarg->func() << " @ " << this->m_zvarg->sloc();
  }

Reference& Instantiated_Function::invoke(Reference& self, const Global_Context& global, cow_vector<Reference>&& args) const
  {
    // Create the stack and context for this function.
    Evaluation_Stack stack;
    Executive_Context ctx_func(rocket::ref(global), rocket::ref(stack), rocket::ref(this->m_zvarg),
                               this->m_params, rocket::move(self), rocket::move(args));
    stack.reserve(rocket::move(args));
    // Execute the function body.
    auto status = this->m_queue.execute(ctx_func);
    switch(status) {
    case air_status_next:
      {
        // Return `null` if the control flow reached the end of the function.
        return self = Reference_Root::S_null();
      }
    case air_status_return:
      {
        // Return the reference at the top of `stack`.
        return self = rocket::move(stack.open_top());
      }
    case air_status_break_unspec:
    case air_status_break_switch:
    case air_status_break_while:
    case air_status_break_for:
      {
        ASTERIA_THROW_RUNTIME_ERROR("`break` statements are not allowed outside matching `switch` or loop statements.");
      }
    case air_status_continue_unspec:
    case air_status_continue_while:
    case air_status_continue_for:
      {
        ASTERIA_THROW_RUNTIME_ERROR("`continue` statements are not allowed outside matching loop statements.");
      }
    default:
      ASTERIA_TERMINATE("An invalid status code `", status, "` was returned from a function. This is likely a bug. Please report.");
    }
  }

Variable_Callback& Instantiated_Function::enumerate_variables(Variable_Callback& callback) const
  {
    return this->m_queue.enumerate_variables(callback);
  }

}  // namespace Asteria
