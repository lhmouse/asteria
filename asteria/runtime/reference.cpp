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

ROCKET_FLATTEN
Reference::
~Reference()
  {
    this->m_stor.~variant_type();
  }

void
Reference::
do_throw_not_dereferenceable() const
  {
    if(this->m_stor.ptr<St_bad>())
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

ROCKET_FLATTEN
const Value&
Reference::
do_dereference_readonly_slow() const
  {
    if(auto st1 = this->m_stor.ptr<St_temp>()) {
      // Read this temporary value.
      if(ROCKET_EXPECT(st1->subs.size() == 0))
        return st1->val;

      // Apply subscripts.
      auto valp = &(st1->val);
      for(auto it = st1->subs.begin();  it != st1->subs.end();  ++it) {
        valp = it->apply_read_opt(*valp);
        if(!valp) {
          // If a subvalue is not found, a reference to the static null
          // value shall be returned.
          return null;
        }
      }

      // Return a reference to the subelement.
      return *valp;
    }

    if(auto st2 = this->m_stor.ptr<St_var>()) {
      // Check whether initialization was skipped, which may happen if its
      // initializer references itself like in `var n = n + 1;`.
      auto var = unerase_cast<Variable*>(st2->var.get());
      if(!var->is_initialized())
        throw Runtime_Error(xtc_format, "Variable not initialized");

      // Apply subscripts.
      auto valp = &(var->get_value());
      for(auto it = st2->subs.begin();  it != st2->subs.end();  ++it) {
        valp = it->apply_read_opt(*valp);
        if(!valp) {
          // If a subvalue is not found, a reference to the static null
          // value shall be returned.
          return null;
        }
      }

      // Return a reference to the subelement.
      return *valp;
    }

    this->do_throw_not_dereferenceable();
  }

ROCKET_FLATTEN
Value&
Reference::
do_dereference_copy_slow()
  {
    if(auto st1 = this->m_stor.mut_ptr<St_temp>()) {
      // Read this temporary value.
      if(ROCKET_EXPECT(st1->subs.size() == 0))
        return st1->val;

      // Apply subscripts.
      auto valp = &(st1->val);
      for(auto it = st1->subs.begin();  it != st1->subs.end();  ++it) {
        valp = it->apply_write_opt(*valp);
        if(!valp) {
          // If a subvalue is not found, clear the current value.
          st1->val = V_null();
          st1->subs.clear();
          return st1->val;
        }
      }

      // Ensure the value is copied before being moved, so we are never
      // assigning an element into its own container.
      Value saved_val = move(*valp);
      st1->val = move(saved_val);
      st1->subs.clear();
      return st1->val;
    }

    if(auto st2 = this->m_stor.ptr<St_var>()) {
      // Check whether initialization was skipped, which may happen if its
      // initializer references itself like in `var n = n + 1;`. We also
      // have to ensure that the variable doesn't get destroyed by
      // `m_stor.emplace<St_temp()`.
      auto var = unerase_pointer_cast<Variable>(st2->var);
      if(!var->is_initialized())
        throw Runtime_Error(xtc_format, "Variable not initialized");

      // Apply subscripts.
      auto valp = &(var->get_value());
      for(auto it = st2->subs.begin();  it != st2->subs.end();  ++it) {
        valp = it->apply_read_opt(*valp);
        if(!valp) {
          // If a subvalue is not found, clear the current value.
          auto& st1 = this->m_stor.emplace<St_temp>();
          return st1.val;
        }
      }

      // Replace the current value with the one from the variable, which
      // can't alias `m_stor`.
      auto& st1 = this->m_stor.emplace<St_temp>();
      st1.val = *valp;
      return st1.val;
    }

    this->do_throw_not_dereferenceable();
  }

ROCKET_FLATTEN
Value&
Reference::
dereference_mutable() const
  {
    if(this->m_stor.ptr<St_temp>()) {
      // The user is attempting to modify a temporary value. Unlike
      // `dereference_copy()` which is for our internal use, this is a user
      // function, so fail.
      throw Runtime_Error(xtc_format, "Temporary value not modifiable");
    }

    if(auto st2 = this->m_stor.ptr<St_var>()) {
      // Check whether initialization was skipped, which may happen if its
      // initializer references itself like in `var n = n + 1;`.
      auto var = unerase_cast<Variable*>(st2->var.get());
      if(!var->is_initialized())
        throw Runtime_Error(xtc_format, "Variable not initialized");

      if(var->is_immutable())
        throw Runtime_Error(xtc_format, "`const` variable not modifiable");

      // Apply subscripts.
      auto valp = &(var->mut_value());
      for(auto it = st2->subs.begin();  it != st2->subs.end();  ++it) {
        auto& sub = it->apply_open(*valp);
        valp = &sub;
      }

      // Return a reference to the subelement.
      return *valp;
    }

    this->do_throw_not_dereferenceable();
  }

ROCKET_FLATTEN
Value
Reference::
dereference_unset() const
  {
    if(this->m_stor.ptr<St_temp>()) {
      // The user is attempting to modify a temporary value. Unlike
      // `dereference_copy()` which is for our internal use, this is a user
      // function, so fail.
      throw Runtime_Error(xtc_format, "Temporary value not modifiable");
    }

    if(auto st2 = this->m_stor.ptr<St_var>()) {
      // Check whether initialization was skipped, which may happen if its
      // initializer references itself like in `var n = n + 1;`.
      auto var = unerase_cast<Variable*>(st2->var.get());
      if(!var->is_initialized())
        throw Runtime_Error(xtc_format, "Variable not initialized");

      if(var->is_immutable())
        throw Runtime_Error(xtc_format, "`const` variable not modifiable");

      // Get the parent value of the final subelement.
      if(st2->subs.empty())
        throw Runtime_Error(xtc_format,
                 "Only an element of an array or object may be unset");

      auto valp = &(var->mut_value());
      for(auto it = st2->subs.begin();  it != st2->subs.end() - 1;  ++it) {
        valp = it->apply_write_opt(*valp);
        if(!valp) {
          // If a subvalue is not found, return a null value.
          return V_null();
        }
      }

      // Delete the subelement.
      return st2->subs.back().apply_unset(*valp);
    }

    this->do_throw_not_dereferenceable();
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
        this->m_stor = St_bad();
        ROCKET_ASSERT(ptcg.use_count() == 1);

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
