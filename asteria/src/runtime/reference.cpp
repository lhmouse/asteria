// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "executive_context.hpp"
#include "runtime_error.hpp"
#include "variable.hpp"
#include "variable_callback.hpp"
#include "ptc_arguments.hpp"
#include "enums.hpp"
#include "../llds/avmc_queue.hpp"
#include "../llds/reference_stack.hpp"
#include "../utils.hpp"

namespace asteria {

const Value&
Reference::
do_dereference_readonly_slow() const
  {
    const Value* qval;

    // Get the root value.
    switch(this->index()) {
      case index_uninit:
        ASTERIA_THROW("attempt to use a bypassed variable or reference");

      case index_void:
        ASTERIA_THROW("attempt to use the result of a function call which returned no value");

      case index_temporary:
        qval = &(this->m_stor.value);
        break;

      case index_variable: {
        auto qvar = unerase_cast<Variable*>(this->m_stor.var);
        if(!qvar)
          ASTERIA_THROW("attempt to use a moved-away reference (this is probably a bug)");

        if(qvar->is_uninitialized())
          ASTERIA_THROW("attempt to read from an uninitialized variable");

        qval = &(qvar->get_value());
        break;
      }

      case index_ptc_args:
        ASTERIA_THROW("tail call wrapper not dereferenceable");

      default:
        ASTERIA_TERMINATE("invalid reference type enumeration (index `$1`)", this->index());
    }

    // Apply modifiers.
    auto bpos = this->m_stor.mods.begin();
    auto epos = this->m_stor.mods.end();

    while(bpos != epos)
      if(!(qval = bpos++->apply_read_opt(*qval)))
        return null_value;

    return *qval;
  }

Value&
Reference::
do_mutate_into_temporary_slow()
  {
    // Dereference and copy the value. Don't move-assign a value into itself.
    auto val = this->dereference_readonly();
    this->set_temporary(::std::move(val));
    return this->m_stor.value;
  }

Reference&
Reference::
do_finish_call_slow(Global_Context& global)
  {
    // We must rebuild the backtrace using this queue if an exception is thrown.
    cow_vector<rcptr<PTC_Arguments>> frames;
    rcptr<PTC_Arguments> ptca;
    int ptc_conj = ptc_aware_by_ref;
    Reference_Stack alt_stack;

    ASTERIA_RUNTIME_TRY {
      // Unpack all frames recursively.
      // Note that `*this` is overwritten before the wrapped function is called.
      while(this->m_index == index_ptc_args) {
        ROCKET_ASSERT(this->m_stor.ptca);
        ptca.reset(static_cast<PTC_Arguments*>(this->m_stor.ptca.release()));
        this->m_index = index_uninit;

        // Generate a single-step trap before unpacking arguments.
        if(auto qhooks = global.get_hooks_opt())
          qhooks->on_single_step_trap(ptca->sloc());

        // Get the `this` reference and all the other arguments.
        auto& stack = ptca->open_stack();
        *this = ::std::move(stack.mut_back());
        stack.pop_back();

        // Call the hook function if any.
        if(auto qhooks = global.get_hooks_opt())
          qhooks->on_function_call(ptca->sloc(), ptca->get_target());

        // Record this frame.
        frames.emplace_back(ptca);
        ptc_conj |= ptca->ptc_aware();

        // Perform a non-tail call.
        ptca->get_target().invoke_ptc_aware(*this, global, ::std::move(stack));
      }

      // Check for deferred expressions.
      while(frames.size()) {
        // Pop frames in reverse order.
        ptca = ::std::move(frames.mut_back());
        frames.pop_back();

        // Evaluate deferred expressions if any.
        if(ptca->get_defer().size())
          Executive_Context(Executive_Context::M_defer(), global, ptca->open_stack(),
                            alt_stack, ::std::move(ptca->open_defer()))
            .on_scope_exit(air_status_next);

        // Call the hook function if any.
        if(auto qhooks = global.get_hooks_opt())
          qhooks->on_function_return(ptca->sloc(), ptca->get_target(), *this);
      }
    }
    ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
      // Check for deferred expressions.
      while(frames.size()) {
        // Pop frames in reverse order.
        ptca = ::std::move(frames.mut_back());
        frames.pop_back();

        // Push the function call.
        except.push_frame_plain(ptca->sloc(), sref("[proper tail call]"));

        // Call the hook function if any.
        if(auto qhooks = global.get_hooks_opt())
          qhooks->on_function_except(ptca->sloc(), ptca->get_target(), except);

        // Evaluate deferred expressions if any.
        if(ptca->get_defer().size())
          Executive_Context(Executive_Context::M_defer(), global, ptca->open_stack(),
                            alt_stack, ::std::move(ptca->open_defer()))
            .on_scope_exit(except);

        // Push the caller.
        // Note that if we arrive here, there must have been an exception thrown when
        // unpacking the last frame (i.e. the last call did not return), so the last
        // frame does not have its enclosing function set.
        if(auto qcall = ptca->caller_opt())
          except.push_frame_func(qcall->sloc, qcall->func);
      }
      throw;
    }

    if(!this->is_void()) {
      // If any of the calls inbetween is void, the result is void.
      // If all of the calls inbetween are by reference, the result is by reference.
      if(ptc_conj == ptc_aware_void)
        this->set_void();
      else if(ptc_conj != ptc_aware_by_ref)
        this->open_temporary();
    }

    return *this;
  }

Variable_Callback&
Reference::
enumerate_variables(Variable_Callback& callback) const
  {
    this->m_stor.value.enumerate_variables(callback);
    callback.process(&(this->m_stor.var), unerase_pointer_cast<Variable>(this->m_stor.var));
    return callback;
  }

Value&
Reference::
dereference_mutable() const
  {
    Value* qval;

    // Get the root value.
    switch(this->index()) {
      case index_uninit:
        ASTERIA_THROW("attempt to use a bypassed variable or reference");

      case index_void:
        ASTERIA_THROW("attempt to use the result of a function call which returned no value");

      case index_temporary:
        ASTERIA_THROW("attempt to modify a temporary `$1`", this->m_stor.value);
        break;

      case index_variable: {
        auto qvar = unerase_cast<Variable*>(this->m_stor.var);
        if(!qvar)
          ASTERIA_THROW("attempt to use a moved-away reference (this is probably a bug)");

        if(qvar->is_uninitialized())
          ASTERIA_THROW("attempt to read from an uninitialized variable");

        qval = &(qvar->open_value());
        break;
      }

      case index_ptc_args:
        ASTERIA_THROW("tail call wrapper not dereferenceable");

      default:
        ASTERIA_TERMINATE("invalid reference type enumeration (index `$1`)", this->index());
    }

    // Apply modifiers.
    auto bpos = this->m_stor.mods.begin();
    auto epos = this->m_stor.mods.end();

    while(bpos != epos)
      qval = &(bpos++->apply_open(*qval));

    return *qval;
  }

Value
Reference::
dereference_unset() const
  {
    Value* qval;

    // Get the root value.
    switch(this->index()) {
      case index_uninit:
        ASTERIA_THROW("attempt to use a bypassed variable or reference");

      case index_void:
        ASTERIA_THROW("attempt to use the result of a function call which returned no value");

      case index_temporary:
        ASTERIA_THROW("attempt to modify a temporary `$1`", this->m_stor.value);
        break;

      case index_variable: {
        auto qvar = unerase_cast<Variable*>(this->m_stor.var);
        if(!qvar)
          ASTERIA_THROW("attempt to use a moved-away reference (this is probably a bug)");

        if(qvar->is_uninitialized())
          ASTERIA_THROW("attempt to read from an uninitialized variable");

        qval = &(qvar->open_value());
        break;
      }

      case index_ptc_args:
        ASTERIA_THROW("tail call wrapper not dereferenceable");

      default:
        ASTERIA_TERMINATE("invalid reference type enumeration (index `$1`)", this->index());
    }

    // Apply modifiers except the last one.
    auto bpos = this->m_stor.mods.begin();
    auto epos = this->m_stor.mods.end();

    if(bpos == epos)
      ASTERIA_THROW("root values cannot be unset");

    while(bpos != epos - 1)
      if(!(qval = bpos++->apply_write_opt(*qval)))
        return null_value;

    return bpos->apply_unset(*qval);
  }

}  // namespace asteria
