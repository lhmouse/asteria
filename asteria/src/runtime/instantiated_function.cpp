// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "instantiated_function.hpp"
#include "air_node.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "runtime_error.hpp"
#include "ptc_arguments.hpp"
#include "enums.hpp"
#include "../llds/reference_stack.hpp"
#include "../utils.hpp"

namespace asteria {

Instantiated_Function::
~Instantiated_Function()
  {
  }

void
Instantiated_Function::
do_solidify(const cow_vector<AIR_Node>& code)
  {
    this->m_queue.clear();
    ::rocket::all_of(code, [&](const AIR_Node& node) { return node.solidify(this->m_queue);  });
    this->m_queue.finalize();
  }

tinyfmt&
Instantiated_Function::
describe(tinyfmt& fmt) const
  {
    return fmt << "`" << this->m_zvarg->func() << "` at '" << this->m_zvarg->sloc() << "'";
  }

void
Instantiated_Function::
get_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    this->m_queue.get_variables(staged, temp);
  }

Reference&
Instantiated_Function::
invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const
  {
    // Create the stack and context for this function.
    AIR_Status status;
    Reference_Stack alt_stack;
    Executive_Context ctx_func(Executive_Context::M_function(), global,
          stack, alt_stack, this->m_zvarg, this->m_params, ::std::move(self));

    // Execute the function body.
    ASTERIA_RUNTIME_TRY {
      status = this->m_queue.execute(ctx_func);
    }
    ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
      ctx_func.on_scope_exit(except);
      except.push_frame_func(this->m_zvarg->sloc(), this->m_zvarg->func());
      throw;
    }
    ctx_func.on_scope_exit(status);

    switch(status) {
      case air_status_next:
      case air_status_return_void:
        // Return void if the control flow reached the end of the function.
        self.set_void();
        break;

      case air_status_return_ref: {
        // In case of PTCs, set up source location. This cannot be set at the
        // call site where such information isn't available.
        auto ptca = stack.top().get_ptc_args_opt();
        if(ptca)
          ptca->set_caller(this->m_zvarg);

        // Return the reference at the top of `stack`.
        self.swap(stack.mut_top());
        break;
      }

      case air_status_break_unspec:
      case air_status_break_switch:
      case air_status_break_while:
      case air_status_break_for:
        ASTERIA_THROW("stray `break` statement");

      case air_status_continue_unspec:
      case air_status_continue_while:
      case air_status_continue_for:
        ASTERIA_THROW("stray `continue` statement");

      default:
        ASTERIA_TERMINATE("invalid AIR status code (status `$1`)", status);
    }
    return self;
  }

}  // namespace asteria
