// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "instantiated_function.hpp"
#include "air_node.hpp"
#include "evaluation_stack.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "../utilities.hpp"

namespace Asteria {

Instantiated_Function::~Instantiated_Function()
  {
  }

tinyfmt& Instantiated_Function::describe(tinyfmt& fmt) const
  {
    return fmt << this->m_zvarg->func() << " @ " << this->m_zvarg->sloc();
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
    // Handle the return reference.
    switch(status) {
    case air_status_next:
      {
        // Return `null` if the control flow reached the end of the function.
        self = Reference_Root::S_null();
        break;
      }
    case air_status_return:
      {
        // Return the reference at the top of `stack`.
        self = rocket::move(stack.open_top());
        break;
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
      ASTERIA_TERMINATE("An invalid status code `", status, "` was returned from a function." ASTERIA_REPORT_BUG);
    }
    return self;
  }

Variable_Callback& Instantiated_Function::enumerate_variables(Variable_Callback& callback) const
  {
    return this->m_queue.enumerate_variables(callback);
  }

}  // namespace Asteria
