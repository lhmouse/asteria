// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
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
Instantiated_Function(const cow_vector<phsh_string>& params, refcnt_ptr<Variadic_Arguer>&& zvarg,
                      const cow_vector<AIR_Node>& code)
  :
    m_params(params), m_zvarg(::std::move(zvarg))
  {
    ::rocket::for_each(code, [&](const AIR_Node& node) { node.solidify(this->m_rod);  });
    this->m_rod.finalize();
  }

Instantiated_Function::
~Instantiated_Function()
  {
  }

tinyfmt&
Instantiated_Function::
describe(tinyfmt& fmt) const
  {
    return fmt << "`" << this->m_zvarg->func() << "` at '" << this->m_zvarg->sloc() << "'";
  }

void
Instantiated_Function::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    this->m_rod.collect_variables(staged, temp);
  }

Reference&
Instantiated_Function::
invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const
  {
    // Create the stack and context for this function.
    Reference_Stack alt_stack;
    Executive_Context ctx_func(Executive_Context::M_function(), global, stack, alt_stack,
                               this->m_zvarg, this->m_params, ::std::move(self));

    // Execute the function body, using `stack` for evaluation.
    AIR_Status status;
    try {
      status = this->m_rod.execute(ctx_func);
    }
    catch(Runtime_Error& except) {
      ctx_func.on_scope_exit_exceptional(except);
      except.push_frame_function(this->m_zvarg->sloc(), this->m_zvarg->func());
      throw;
    }
    ctx_func.on_scope_exit_normal(status);

    switch(status) {
      case air_status_next:
      case air_status_return_void:
        // Return void if the control flow reached the end of the function.
        return self.set_void();

      case air_status_return_ref: {
        // Return the reference at the top of `stack`.
        return self = ::std::move(stack.mut_top());
      }

      case air_status_break_unspec:
      case air_status_break_switch:
      case air_status_break_while:
      case air_status_break_for:
        throw Runtime_Error(Runtime_Error::M_format(), "Stray `break` statement");

      case air_status_continue_unspec:
      case air_status_continue_while:
      case air_status_continue_for:
        throw Runtime_Error(Runtime_Error::M_format(), "Stray `continue` statement");

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), status);
    }
  }

}  // namespace asteria
