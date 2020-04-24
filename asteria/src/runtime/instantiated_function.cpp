// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "instantiated_function.hpp"
#include "air_node.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "runtime_error.hpp"
#include "ptc_arguments.hpp"
#include "../utilities.hpp"

namespace Asteria {

Instantiated_Function::
~Instantiated_Function()
  {
  }

void
Instantiated_Function::
do_solidify(const cow_vector<AIR_Node>& code)
  {
    ::rocket::all_of(code, [&](const AIR_Node& node) { return node.solidify(this->m_queue);  });
    this->m_queue.shrink_to_fit();
  }

tinyfmt&
Instantiated_Function::
describe(tinyfmt& fmt)
const
  {
    return fmt << this->m_zvarg->func() << " @ " << this->m_zvarg->sloc();
  }

Variable_Callback&
Instantiated_Function::
enumerate_variables(Variable_Callback& callback)
const
  {
    return this->m_queue.enumerate_variables(callback);
  }

Reference&
Instantiated_Function::
invoke_ptc_aware(Reference& self, Global_Context& global, cow_vector<Reference>&& args)
const
  {
    // Create the stack and context for this function.
    Evaluation_Stack stack;
    Executive_Context ctx_func(::rocket::ref(global), ::rocket::ref(stack),
                               this->m_zvarg, this->m_params,
                               ::std::move(self), ::std::move(args));
    stack.reserve(::std::move(args));

    // Execute the function body.
    AIR_Status status;
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
        self = Reference_root::S_void();
        break;

      case air_status_return_ref:
        // Return the reference at the top of `stack`.
        self = ::std::move(stack.open_top());

        // In case of PTCs, set up source location.
        // This cannot be set at the call site where such information isn't available.
        if(auto tca = self.get_tail_call_opt())
          tca->set_enclosing_function(this->m_zvarg->sloc(), this->m_zvarg->func());
        break;

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

    // Enable `args` to be reused after this call.
    stack.unreserve(args);
    return self;
  }

}  // namespace Asteria
