// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "instantiated_function.hpp"
#include "../runtime/air_node.hpp"
#include "evaluation_stack.hpp"
#include "executive_context.hpp"
#include "../utilities.hpp"

namespace Asteria {

Instantiated_Function::~Instantiated_Function()
  {
  }

std::ostream& Instantiated_Function::describe(std::ostream& ostrm) const
  {
    return ostrm << this->m_zvarg->get_function_signature() << " @ " << this->m_zvarg->get_source_location();
  }

Reference& Instantiated_Function::invoke(Reference& self, const Global_Context& global, cow_vector<Reference>&& args) const
  {
    // Create the stack and context for this function.
    Evaluation_Stack stack;
    Executive_Context ctx_func(1, global, stack, this->m_zvarg, this->m_params, rocket::move(self), rocket::move(args));
    stack.reserve_references(rocket::move(args));
    // Execute AIR nodes one by one.
    auto status = Air_Node::status_next;
    rocket::any_of(this->m_code, [&](const uptr<Air_Node>& q) { return (status = q->execute(ctx_func)) != Air_Node::status_next;  });
    switch(status) {
    case Air_Node::status_next:
      {
        // Return `null` if the control flow reached the end of the function.
        return self = Reference_Root::S_null();
      }
    case Air_Node::status_return:
      {
        // Return the reference at the top of `stack`.
        return self = rocket::move(stack.open_top_reference());
      }
    case Air_Node::status_break_unspec:
    case Air_Node::status_break_switch:
    case Air_Node::status_break_while:
    case Air_Node::status_break_for:
      {
        ASTERIA_THROW_RUNTIME_ERROR("`break` statements are not allowed outside matching `switch` or loop statements.");
      }
    case Air_Node::status_continue_unspec:
    case Air_Node::status_continue_while:
    case Air_Node::status_continue_for:
      {
        ASTERIA_THROW_RUNTIME_ERROR("`continue` statements are not allowed outside matching loop statements.");
      }
    default:
      ASTERIA_TERMINATE("An invalid status code `", status, "` was returned from a function. This is likely a bug. Please report.");
    }
  }

Variable_Callback& Instantiated_Function::enumerate_variables(Variable_Callback& callback) const
  {
    // Enumerate all variables inside the function body.
    rocket::for_each(this->m_code, [&](const uptr<Air_Node>& q) { q->enumerate_variables(callback);  });
    return callback;
  }

}  // namespace Asteria
