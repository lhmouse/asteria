// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

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

void
Reference::
do_throw_not_dereferenceable() const
  {
    ASTERIA_THROW_RUNTIME_ERROR((
        "Reference type `$1` not dereferenceable"),
        describe_xref(this->m_xref));
  }

void
Reference::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    this->m_value.collect_variables(staged, temp);

    if(auto var = unerase_pointer_cast<Variable>(this->m_var))
      if(staged.insert(&(this->m_var), var))
        temp.insert(var.get(), var);
  }

const Value&
Reference::
do_dereference_readonly_slow() const
  {
    const Value* valp = nullptr;
    size_t mi = 0;

    switch(this->m_xref) {
      case xref_invalid:
        ASTERIA_THROW_RUNTIME_ERROR(("Reference not initialized"));

      case xref_void:
        ASTERIA_THROW_RUNTIME_ERROR(("Void reference not dereferenceable"));

      case xref_temporary:
        valp = &(this->m_value);
        break;

      case xref_variable: {
        auto var = unerase_cast<Variable*>(this->m_var.get());
        ROCKET_ASSERT(var);

        if(!var->is_initialized())
          ASTERIA_THROW_RUNTIME_ERROR(("Reference not initialized"));

        valp = &(var->get_value());
        break;
      }

      case xref_ptc:
        ASTERIA_THROW_RUNTIME_ERROR(("Proper tail call not expanded"));

      default:
        ASTERIA_TERMINATE(("Invalid reference type (xref `$1`)"), this->m_xref);
    }

    while(valp && (mi != this->m_mods.size()))
      valp = this->m_mods[mi++].apply_read_opt(*valp);

    if(!valp)
      return null_value;

    return *valp;
  }

Value&
Reference::
dereference_mutable() const
  {
    Value* valp = nullptr;
    size_t mi = 0;

    switch(this->m_xref) {
      case xref_invalid:
        ASTERIA_THROW_RUNTIME_ERROR(("Reference not initialized"));

      case xref_void:
        ASTERIA_THROW_RUNTIME_ERROR(("Void reference not dereferenceable"));

      case xref_temporary:
        ASTERIA_THROW_RUNTIME_ERROR(("Attempt to modify a temporary value"));

      case xref_variable: {
        auto var = unerase_cast<Variable*>(this->m_var.get());
        ROCKET_ASSERT(var);

        if(!var->is_initialized())
          ASTERIA_THROW_RUNTIME_ERROR(("Reference not initialized"));

        if(var->is_immutable())
          ASTERIA_THROW_RUNTIME_ERROR(("Attempt to modify a `const` variable"));

        valp = &(var->mut_value());
        break;
      }

      case xref_ptc:
        ASTERIA_THROW_RUNTIME_ERROR(("Proper tail call not expanded"));

      default:
        ASTERIA_TERMINATE(("Invalid reference type (xref `$1`)"), this->m_xref);
    }

    while(valp && (mi != this->m_mods.size()))
      valp = &(this->m_mods[mi++].apply_open(*valp));

    return *valp;
  }

Value
Reference::
dereference_unset() const
  {
    Value* valp = nullptr;
    size_t mi = 0;

    switch(this->m_xref) {
      case xref_invalid:
        ASTERIA_THROW_RUNTIME_ERROR(("Reference not initialized"));

      case xref_void:
        ASTERIA_THROW_RUNTIME_ERROR(("Void reference not dereferenceable"));

      case xref_temporary:
        ASTERIA_THROW_RUNTIME_ERROR(("Attempt to modify a temporary value"));

      case xref_variable: {
        auto var = unerase_cast<Variable*>(this->m_var.get());
        ROCKET_ASSERT(var);

        if(!var->is_initialized())
          ASTERIA_THROW_RUNTIME_ERROR(("Reference not initialized"));

        if(var->is_immutable())
          ASTERIA_THROW_RUNTIME_ERROR(("Attempt to modify a `const` variable"));

        valp = &(var->mut_value());
        break;
      }

      case xref_ptc:
        ASTERIA_THROW_RUNTIME_ERROR(("Proper tail call not expanded"));

      default:
        ASTERIA_TERMINATE(("Invalid reference type (xref `$1`)"), this->m_xref);
    }

    if(this->m_mods.size() == 0)
      ASTERIA_THROW_RUNTIME_ERROR(("Only elements of an array or object may be unset"));

    while(valp && (mi != this->m_mods.size() - 1))
      valp = this->m_mods[mi++].apply_write_opt(*valp);

    if(!valp)
      return null_value;

    return this->m_mods[mi].apply_unset(*valp);
  }

void
Reference::
do_use_function_result_slow(Global_Context& global)
  {
    refcnt_ptr<PTC_Arguments> ptc;
    cow_vector<refcnt_ptr<PTC_Arguments>> frames;
    bool is_void = false;
    bool by_value = false;
    Reference_Stack alt_stack;

    try {
      while(this->m_xref == xref_ptc) {
        ptc.reset(unerase_cast<PTC_Arguments*>(this->m_ptc.release()));
        ROCKET_ASSERT(ptc.use_count() == 1);
        frames.emplace_back(ptc);
        ASTERIA_CALL_GLOBAL_HOOK(global, on_single_step_trap, ptc->sloc());

        // Get the `this` reference and all the other arguments.
        *this = ::std::move(ptc->stack().mut_top());
        ptc->stack().pop();

        if(ptc->ptc_aware() == ptc_aware_void)
          is_void = true;
        else if(ptc->ptc_aware() == ptc_aware_by_val)
          by_value = true;

        // Perform a non-tail call.
        ASTERIA_CALL_GLOBAL_HOOK(global, on_function_call, ptc->sloc(), ptc->target());
        ptc->target().invoke_ptc_aware(*this, global, ::std::move(ptc->stack()));
      }

      while(frames.size()) {
        ptc = ::std::move(frames.mut_back());
        frames.pop_back();

        if(ptc->defer().size())
          Executive_Context(Executive_Context::M_defer(),
                global, ptc->stack(), alt_stack, ::std::move(ptc->defer()))
            .on_scope_exit(air_status_next);

        ASTERIA_CALL_GLOBAL_HOOK(global, on_function_return, ptc->sloc(),
                                 ptc->target(), *this);
      }
    }
    catch(Runtime_Error& except) {
      while(frames.size()) {
        ptc = ::std::move(frames.mut_back());
        frames.pop_back();

        // Note that if we arrive here, there must have been an exception thrown
        // when unpacking the last frame (i.e. the last call did not return), so
        // the last frame does not have its enclosing function set.
        except.push_frame_plain(ptc->sloc(), sref("[proper tail call]"));

        if(auto qcall = ptc->caller_opt())
          except.push_frame_func(qcall->sloc(), qcall->func());

        if(ptc->defer().size())
          Executive_Context(Executive_Context::M_defer(),
                global, ptc->stack(), alt_stack, ::std::move(ptc->defer()))
            .on_scope_exit(except);

        ASTERIA_CALL_GLOBAL_HOOK(global, on_function_except, ptc->sloc(),
                                 ptc->target(), except);
      }
      throw;
    }

    if(is_void)
      this->set_void();
    else if(by_value && (this->m_xref != xref_void))
      this->dereference_copy();
  }

}  // namespace asteria
