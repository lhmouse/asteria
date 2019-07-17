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

std::ostream& Instantiated_Function::describe(std::ostream& os) const
  {
    return os << this->m_zvarg->get_function_signature() << " @ " << this->m_zvarg->get_source_location();
  }

Reference& Instantiated_Function::invoke(Reference& self, const Global_Context& global, Cow_Vector<Reference>&& args) const
  {
    // Create the context for this function.
    Executive_Context ctx_func(nullptr);
    ctx_func.prepare_function_arguments(this->m_zvarg, this->m_params, rocket::move(self), rocket::move(args));
    // Reuse the storage of `args` to create the stack.
    Evaluation_Stack stack(rocket::move(args));
    // Execute AIR nodes one by one.
    auto status = Air_Node::status_next;
    std::find_if(this->m_code.begin(), this->m_code.end(),
                 [&](const Air_Node& node) { return (status = node.execute(stack, ctx_func,
                                                                           this->m_zvarg->get_function_signature(), global))
                                                    != Air_Node::status_next;  });
    if(status == Air_Node::status_next) {
      // Return `null` if the control flow reached the end of the function.
      self = Reference_Root::S_null();
    }
    else if(status == Air_Node::status_return){
      // Return the reference at the top of `stack`.
      self = rocket::move(stack.open_top_reference());
    }
    else {
      ASTERIA_THROW_RUNTIME_ERROR("An invalid status code `", status, "` was returned from a function. This is likely a bug. Please report.");
    }
    return self;
  }

void Instantiated_Function::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    // Enumerate all variables inside the function body.
    rocket::for_each(this->m_code, [&](const Air_Node& node) { node.enumerate_variables(callback);  });
  }

}  // namespace Asteria
