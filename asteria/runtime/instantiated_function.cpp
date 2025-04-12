// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "instantiated_function.hpp"
#include "air_node.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "runtime_error.hpp"
#include "ptc_arguments.hpp"
#include "enums.hpp"
#include "../llds/reference_stack.hpp"
#include "../utils.hpp"
namespace asteria {

Instantiated_Function::
Instantiated_Function(const Source_Location& xsloc, const cow_string& xfunc,
                      const cow_vector<phsh_string>& xparams, const cow_vector<AIR_Node>& code)
  :
    m_sloc(xsloc), m_func(xfunc), m_params(xparams)
  {
    for(size_t i = 0;  i < code.size();  ++i)
      code.at(i).solidify(this->m_rod);
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
    return format(fmt, "`$1` at '$2'", this->m_func, this->m_sloc);
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
    Executive_Context ctx_func(xtc_function, global, stack, alt_stack, *this, move(self));

    // Execute the function body.
    global.call_hook(&Abstract_Hooks::on_function_enter, ctx_func, *this);
    AIR_Status status;
    try {
      status = this->m_rod.execute(ctx_func);
    }
    catch(Runtime_Error& except) {
      global.call_hook(&Abstract_Hooks::on_function_leave, ctx_func);
      ctx_func.on_scope_exit_exceptional(except);
      except.push_frame_function(this->m_sloc, this->m_func);
      throw;
    }
    global.call_hook(&Abstract_Hooks::on_function_leave, ctx_func);
    ctx_func.on_scope_exit_normal(status);

    switch(status)
      {
      case air_status_next:
      case air_status_return_void:
        self.set_void();
        return self;

      case air_status_return_ref:
        self = move(stack.mut_top());
        return self;

      case air_status_break_unspec:
      case air_status_break_switch:
      case air_status_break_while:
      case air_status_break_for:
        throw Runtime_Error(xtc_format, "Stray `break` statement");

      case air_status_continue_unspec:
      case air_status_continue_while:
      case air_status_continue_for:
        throw Runtime_Error(xtc_format, "Stray `continue` statement");

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), status);
    }
  }

}  // namespace asteria
