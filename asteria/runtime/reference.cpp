// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "reference.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "executive_context.hpp"
#include "runtime_error.hpp"
#include "variable.hpp"
#include "ptc_arguments.hpp"
#include "enums.hpp"
#include "../llds/avmc_queue.hpp"
#include "../llds/reference_stack.hpp"
#include "../llds/variable_hashmap.hpp"
#include "../utils.hpp"
namespace asteria {

const Value&
Reference::
do_dereference_readonly_slow() const
  {
    const Value* qval;

    // Get the root value.
    switch(this->index()) {
      case index_invalid:
        ASTERIA_THROW_RUNTIME_ERROR((
            "Attempt to use a bypassed variable or reference"));

      case index_void:
        ASTERIA_THROW_RUNTIME_ERROR((
            "Attempt to use the result of a function call which returned no value"));

      case index_temporary:
        qval = &(this->m_value);
        break;

      case index_variable: {
        auto qvar = unerase_cast<Variable*>(this->m_var);
        if(!qvar)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Attempt to use a moved-away reference (this is probably a bug)"));

        if(qvar->is_uninitialized())
          ASTERIA_THROW_RUNTIME_ERROR((
              "Attempt to reference an uninitialized variable"));

        qval = &(qvar->get_value());
        break;
      }

      case index_ptc_args:
        ASTERIA_THROW_RUNTIME_ERROR((
            "Tail call wrapper not dereferenceable"));

      default:
        ASTERIA_TERMINATE((
            "Invalid reference type enumeration (index `$1`)"),
            this->index());
    }

    // Apply modifiers.
    auto bpos = this->m_mods.begin();
    auto epos = this->m_mods.end();

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
    return this->m_value;
  }

Reference&
Reference::
do_finish_call_slow(Global_Context& global)
  {
    // We must rebuild the backtrace using this queue if an exception is thrown.
    cow_vector<refcnt_ptr<PTC_Arguments>> frames;
    refcnt_ptr<PTC_Arguments> ptca;
    int ptc_conj = ptc_aware_by_ref;
    Reference_Stack alt_stack;

    try {
      // Unpack all frames recursively.
      // Note that `*this` is overwritten before the wrapped function is called.
      while(this->m_index == index_ptc_args) {
        ROCKET_ASSERT(this->m_ptca.use_count() == 1);
        ptca.reset(static_cast<PTC_Arguments*>(this->m_ptca.release()));
        this->m_index = index_invalid;

        // Generate a single-step trap before unpacking arguments.
        if(auto qhooks = global.get_hooks_opt())
          qhooks->on_single_step_trap(ptca->sloc());

        // Get the `this` reference and all the other arguments.
        auto& stack = ptca->stack();
        *this = ::std::move(stack.mut_top());
        stack.pop();

        // Call the hook function if any.
        if(auto qhooks = global.get_hooks_opt())
          qhooks->on_function_call(ptca->sloc(), ptca->target());

        // Record this frame.
        frames.emplace_back(ptca);
        ptc_conj |= ptca->ptc_aware();

        // Perform a non-tail call.
        ptca->target().invoke_ptc_aware(*this, global, ::std::move(stack));
      }

      // Check for deferred expressions.
      while(frames.size()) {
        // Pop frames in reverse order.
        ptca = ::std::move(frames.mut_back());
        frames.pop_back();

        // Evaluate deferred expressions if any.
        if(ptca->defer().size())
          Executive_Context(Executive_Context::M_defer(), global, ptca->stack(),
                            alt_stack, ::std::move(ptca->defer()))
            .on_scope_exit(air_status_next);

        // Call the hook function if any.
        if(auto qhooks = global.get_hooks_opt())
          qhooks->on_function_return(ptca->sloc(), ptca->target(), *this);
      }
    }
    catch(Runtime_Error& except) {
      // Check for deferred expressions.
      while(frames.size()) {
        // Pop frames in reverse order.
        ptca = ::std::move(frames.mut_back());
        frames.pop_back();

        // Push the function call.
        except.push_frame_plain(ptca->sloc(), sref("[proper tail call]"));

        // Call the hook function if any.
        if(auto qhooks = global.get_hooks_opt())
          qhooks->on_function_except(ptca->sloc(), ptca->target(), except);

        // Evaluate deferred expressions if any.
        if(ptca->defer().size())
          Executive_Context(Executive_Context::M_defer(), global, ptca->stack(),
                            alt_stack, ::std::move(ptca->defer()))
            .on_scope_exit(except);

        // Push the caller.
        // Note that if we arrive here, there must have been an exception thrown when
        // unpacking the last frame (i.e. the last call did not return), so the last
        // frame does not have its enclosing function set.
        if(auto qcall = ptca->caller_opt())
          except.push_frame_func(qcall->sloc(), qcall->func());
      }
      throw;
    }

    if(!this->is_void()) {
      // If any of the calls inbetween is void, the result is void.
      // If all of the calls inbetween are by reference, the result is by reference.
      if(ptc_conj == ptc_aware_void)
        this->set_void();
      else if(ptc_conj != ptc_aware_by_ref)
        this->mut_temporary();
    }

    return *this;
  }

void
Reference::
get_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    this->m_value.get_variables(staged, temp);

    if(auto var = unerase_pointer_cast<Variable>(this->m_var))
      if(staged.insert(&(this->m_var), var))
        temp.insert(var.get(), var);
  }

Value&
Reference::
dereference_mutable() const
  {
    Value* qval;

    // Get the root value.
    switch(this->index()) {
      case index_invalid:
        ASTERIA_THROW_RUNTIME_ERROR((
            "Attempt to use a bypassed variable or reference"));

      case index_void:
        ASTERIA_THROW_RUNTIME_ERROR((
            "Attempt to use the result of a function call which returned no value"));

      case index_temporary:
        ASTERIA_THROW_RUNTIME_ERROR((
            "Attempt to modify a temporary `$1`"),
            this->m_value);

      case index_variable: {
        auto qvar = unerase_cast<Variable*>(this->m_var);
        if(!qvar)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Attempt to use a moved-away reference (this is probably a bug)"));

        if(qvar->is_uninitialized())
          ASTERIA_THROW_RUNTIME_ERROR((
              "Attempt to reference an uninitialized variable"));

        if(!qvar->is_mutable())
          ASTERIA_THROW_RUNTIME_ERROR((
              "Attempt to modify an immutable variable"));

        qval = &(qvar->mut_value());
        break;
      }

      case index_ptc_args:
        ASTERIA_THROW_RUNTIME_ERROR((
            "Tail call wrapper not dereferenceable"));

      default:
        ASTERIA_TERMINATE((
            "Invalid reference type enumeration (index `$1`)"),
            this->index());
    }

    // Apply modifiers.
    auto bpos = this->m_mods.begin();
    auto epos = this->m_mods.end();

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
      case index_invalid:
        ASTERIA_THROW_RUNTIME_ERROR((
            "Attempt to use a bypassed variable or reference"));

      case index_void:
        ASTERIA_THROW_RUNTIME_ERROR((
            "Attempt to use the result of a function call which returned no value"));

      case index_temporary:
        ASTERIA_THROW_RUNTIME_ERROR((
            "Attempt to modify a temporary `$1`"),
            this->m_value);

      case index_variable: {
        auto qvar = unerase_cast<Variable*>(this->m_var);
        if(!qvar)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Attempt to use a moved-away reference (this is probably a bug)"));

        if(qvar->is_uninitialized())
          ASTERIA_THROW_RUNTIME_ERROR((
              "Attempt to reference an uninitialized variable"));

        qval = &(qvar->mut_value());
        break;
      }

      case index_ptc_args:
        ASTERIA_THROW_RUNTIME_ERROR((
            "Tail call wrapper not dereferenceable"));

      default:
        ASTERIA_TERMINATE((
            "Invalid reference type enumeration (index `$1`)"),
            this->index());
    }

    // Apply modifiers except the last one.
    auto bpos = this->m_mods.begin();
    auto epos = this->m_mods.end();

    if(bpos == epos)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Root values cannot be unset"));

    while(bpos != epos - 1)
      if(!(qval = bpos++->apply_write_opt(*qval)))
        return null_value;

    return bpos->apply_unset(*qval);
  }

}  // namespace asteria
