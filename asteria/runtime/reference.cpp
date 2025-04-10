// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "reference.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "executive_context.hpp"
#include "runtime_error.hpp"
#include "variable.hpp"
#include "ptc_arguments.hpp"
#include "enums.hpp"
#include "../llds/avm_rod.hpp"
#include "../llds/reference_stack.hpp"
#include "../llds/variable_hashmap.hpp"
#include "../utils.hpp"
namespace asteria {

void
Reference::
do_throw_not_dereferenceable() const
  {
    if(this->m_stor.ptr<St_invalid>())
      throw Runtime_Error(xtc_format,
               "Reference not initialized");

    if(this->m_stor.ptr<St_void>())
      throw Runtime_Error(xtc_format,
               "Void reference not dereferenceable");

    if(this->m_stor.ptr<St_ptc>())
      throw Runtime_Error(xtc_format,
               "Proper tail call (PTC) not dereferenceable");

    ROCKET_ASSERT(false);
  }

void
Reference::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    if(auto st1 = this->m_stor.ptr<St_temp>())
      st1->val.collect_variables(staged, temp);

    if(auto st2 = this->m_stor.ptr<St_var>()) {
      // For a variable, we must record the identity of the pointer.
      auto var = unerase_pointer_cast<Variable>(st2->var);
      if(staged.insert(&(st2->var), var))
        temp.insert(var.get(), var);
    }
  }

const Value&
Reference::
do_dereference_readonly_slow() const
  {
    const Value* valp;
    const cow_vector<Subscript>* subsp;

    if(auto st1 = this->m_stor.ptr<St_temp>()) {
      valp = &(st1->val);
      subsp = &(st1->subs);
    }
    else if(auto st2 = this->m_stor.ptr<St_var>()) {
      // Check whether initialization was skipped, which may happen if its
      // initializer references itself like in `var n = n + 1;`.
      auto var = unerase_cast<Variable*>(st2->var.get());
      if(!var->is_initialized())
        throw Runtime_Error(xtc_format,
                 "Variable not initialized");

      valp = &(var->get_value());
      subsp = &(st2->subs);
    }
    else
      this->do_throw_not_dereferenceable();

    for(auto it = subsp->begin();  it != subsp->end();  ++it) {
      // Apply a subscript. If a field is not found, a pointer to the static
      // null value shall be returned.
      valp = it->apply_read_opt(*valp);
      if(!valp)
        return null;
    }

    return *valp;
  }

Value&
Reference::
dereference_mutable() const
  {
    Value* valp;
    const cow_vector<Subscript>* subsp;

    if(this->m_stor.ptr<St_temp>()) {
      throw Runtime_Error(xtc_format,
               "Temporary value not modifiable");
    }
    else if(auto st2 = this->m_stor.ptr<St_var>()) {
      // Check whether initialization was skipped, which may happen if its
      // initializer references itself like in `var n = n + 1;`.
      auto var = unerase_cast<Variable*>(st2->var.get());
      if(!var->is_initialized())
        throw Runtime_Error(xtc_format,
                 "Variable not initialized");

      if(var->is_immutable())
        throw Runtime_Error(xtc_format,
                 "`const` variable not modifiable");

      valp = &(var->mut_value());
      subsp = &(st2->subs);
    }
    else
      this->do_throw_not_dereferenceable();

    for(auto it = subsp->begin();  it != subsp->end();  ++it) {
      // Apply a subscript. This shall not produce a null pointer.
      valp = &(it->apply_open(*valp));
    }

    return *valp;
  }

Value
Reference::
dereference_unset() const
  {
    Value* valp;
    const cow_vector<Subscript>* subsp;

    if(this->m_stor.ptr<St_temp>()) {
      throw Runtime_Error(xtc_format,
               "Temporary value not modifiable");
    }
    else if(auto st2 = this->m_stor.ptr<St_var>()) {
      // Check whether initialization was skipped, which may happen if its
      // initializer references itself like in `var n = n + 1;`.
      auto var = unerase_cast<Variable*>(st2->var.get());
      if(!var->is_initialized())
        throw Runtime_Error(xtc_format,
                 "Variable not initialized");

      if(var->is_immutable())
        throw Runtime_Error(xtc_format,
                 "`const` variable not modifiable");

      valp = &(var->mut_value());
      subsp = &(st2->subs);
    }
    else
      this->do_throw_not_dereferenceable();

    if(subsp->size() == 0)
      throw Runtime_Error(xtc_format,
               "Only elements of an array or object may be unset");

    for(auto it = subsp->begin();  it != subsp->end() - 1;  ++it) {
      // Apply a subscript. If a field is not found, a null value shall be
      // returned.
      valp = it->apply_write_opt(*valp);
      if(!valp)
        return null;
    }

    return subsp->back().apply_unset(*valp);
  }

void
Reference::
do_use_function_result_slow(Global_Context& global)
  {
    refcnt_ptr<PTC_Arguments> ptcg;
    cow_vector<refcnt_ptr<PTC_Arguments>> frames;
    opt<Value> result_value;
    Reference_Stack defer_stack, defer_alt_stack;
    Executive_Context defer_ctx(xtc_defer, global, defer_stack, defer_alt_stack);

    try {
      // Unpack frames until a non-PTC result is encountered.
      while(auto st = this->m_stor.mut_ptr<St_ptc>()) {
        ptcg.reset(unerase_cast<PTC_Arguments*>(st->release()));
        ROCKET_ASSERT(ptcg.use_count() == 1);
        this->m_stor = St_invalid();

        global.call_hook(&Abstract_Hooks::on_call, ptcg->sloc(), ptcg->target());
        frames.emplace_back(ptcg);
        *this = move(ptcg->self());
        ptcg->target().invoke_ptc_aware(*this, global, move(ptcg->mut_stack()));
      }

      // Check the result.
      if(ptcg->ptc_aware() == ptc_aware_void)
        this->m_stor = St_void();
      else if(!this->m_stor.ptr<St_void>())
        result_value = this->dereference_readonly();

      // This is the normal return path.
      while(!frames.empty()) {
        ptcg = move(frames.mut_back());
        frames.pop_back();
        const auto caller = ptcg->caller_opt();

        if((ptcg->ptc_aware() == ptc_aware_by_val) && result_value) {
          // Convert the result.
          auto& st = this->m_stor.emplace<St_temp>();
          st.val = move(*result_value);
          result_value.reset();
        }

        // Evaluate deferred expressions.
        global.call_hook(&Abstract_Hooks::on_return, ptcg->sloc(), ptcg->ptc_aware());
        defer_ctx.stack() = move(ptcg->mut_stack());
        defer_ctx.mut_defer() = move(ptcg->mut_defer());
        defer_ctx.on_scope_exit_normal(air_status_next);
      }
    }
    catch(Runtime_Error& except) {
      // This is the exceptional path.
      while(!frames.empty()) {
        ptcg = move(frames.mut_back());
        frames.pop_back();
        const auto caller = ptcg->caller_opt();

        // Note that if we arrive here, there must have been an exception thrown
        // when unpacking the last frame (i.e. the last call did not return), so
        // the last frame does not have its enclosing function set.
        except.push_frame_plain(ptcg->sloc(), &"[proper tail call]");

        if(caller)
          except.push_frame_function(caller->sloc(), caller->func());

        // Evaluate deferred expressions.
        defer_ctx.stack() = move(ptcg->mut_stack());
        defer_ctx.mut_defer() = move(ptcg->mut_defer());
        defer_ctx.on_scope_exit_exceptional(except);
      }

      // The exception object has been updated, so rethrow it.
      throw;
    }
  }

}  // namespace asteria
