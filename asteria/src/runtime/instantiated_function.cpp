// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "instantiated_function.hpp"
#include "../runtime/air_node.hpp"
#include "reference_stack.hpp"
#include "function_executive_context.hpp"
#include "../utilities.hpp"

namespace Asteria {

Instantiated_Function::~Instantiated_Function()
  {
  }

void Instantiated_Function::describe(std::ostream &os) const
  {
    os << this->m_zvarg->get_function_signature() << " @ " << this->m_zvarg->get_source_location();
  }

void Instantiated_Function::invoke(Reference &self_io, const Global_Context &global, Cow_Vector<Reference> &&args) const
  {
    // Create a stack and a context for this function.
    Reference_Stack stack;
    Function_Executive_Context ctx(this->m_zvarg, this->m_params, rocket::move(self_io), rocket::move(args));
    const auto &func = this->m_zvarg->get_function_signature();
    // Execute AIR nodes one by one.
    auto status = Air_Node::status_next;
    rocket::any_of(this->m_code, [&](const Air_Node &node) { return (status = node.execute(stack, ctx, func, global))
                                                                    != Air_Node::status_next;  });
    switch(status) {
    case Air_Node::status_next:
      {
        // Return `null` if the control flow reached the end of the function.
        self_io = Reference_Root::S_uninitialized();
        // Fallthough.
    case Air_Node::status_return:
        // Return the reference in `self_io`.
        break;
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
      ASTERIA_TERMINATE("An unknown execution result enumeration `", status, "` has been encountered.");
    }
  }

void Instantiated_Function::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    // Enumerate all variables inside the function body.
    rocket::for_each(this->m_code, [&](const Air_Node &node) { node.enumerate_variables(callback);  });
  }

}
