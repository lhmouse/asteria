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

Reference::
~Reference()
  {
  }

Variable_Callback&
Reference::
enumerate_variables(Variable_Callback& callback)
  const
  {
    switch(this->index()) {
      case index_uninit:
      case index_void:
        return callback;

      case index_constant:
        return this->m_root.as<index_constant>().val.enumerate_variables(callback);

      case index_temporary:
        return this->m_root.as<index_temporary>().val.enumerate_variables(callback);

      case index_variable: {
        auto var = unerase_pointer_cast<Variable>(this->m_root.as<index_variable>().var);
        if(!var)
          return callback;

        callback.process(var);
        return callback;
      }

      case index_ptc_args:
      case index_jump_src:
        return callback;

      default:
        ASTERIA_TERMINATE("invalid reference type enumeration (index `$1`)", this->index());
    }
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
      while(!!(ptca = this->get_ptc_args_opt())) {
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

    // The function call yields an rvalue unless all wrapped calls returned by reference.
    if(ptc_conj == ptc_aware_void)
      return *this = Reference::S_void();

    if(ptc_conj == ptc_aware_by_ref)
      return *this;

    if(!this->is_glvalue())
      return *this;

    Reference::S_temporary xref = { this->dereference_readonly() };
    return *this = ::std::move(xref);
  }

const Value&
Reference::
do_dereference_readonly_slow()
  const
  {
    const Value* qval;

    switch(this->index()) {
      case index_uninit:
        ASTERIA_THROW("Attempt to use a bypassed variable or reference");

      case index_void:
        ASTERIA_THROW("Attempt to use the result of a function call which returned no value");

      case index_constant:
        qval = &(this->m_root.as<index_constant>().val);
        break;

      case index_temporary:
        qval = &(this->m_root.as<index_temporary>().val);
        break;

      case index_variable: {
        auto var = unerase_cast<Variable*>(this->m_root.as<index_variable>().var);
        if(!var)
          return null_value;

        if(!var->is_initialized())
          ASTERIA_THROW("Attempt to read from an uninitialized variable");

        qval = &(var->get_value());
        break;
      }

      case index_ptc_args:
        ASTERIA_THROW("Tail call wrapper not dereferenceable");

      case index_jump_src:
        ASTERIA_THROW("Jump marker not dereferenceable");

      default:
        ASTERIA_TERMINATE("invalid reference type enumeration (index `$1`)", this->index());
    }

    // If there is no modifier, return the root verbatim.
    if(ROCKET_EXPECT(this->m_mods.empty()))
      return *qval;

    // We get to apply modifiers, one by one.
    for(size_t k = 0;  k != this->m_mods.size();  ++k) {
      const auto& mod = this->m_mods[k];
      switch(MIndex(mod.index())) {
        case mindex_array_index: {
          const auto& altr = mod.as<mindex_array_index>();

          // Elements of null values are also null values.
          if(qval->is_null())
            return null_value;
          else if(!qval->is_array())
            ASTERIA_THROW("Integer subscript inapplicable (value `$1`, index `$2`)", *qval, altr.index);

          // Get the element at the given index.
          const auto& arr = qval->as_array();
          auto w = wrap_index(altr.index, arr.size());
          if(w.nprepend | w.nappend)
            return null_value;

          qval = &(arr[w.rindex]);
          break;
        }

        case mindex_object_key: {
          const auto& altr = mod.as<mindex_object_key>();

          // Members of null values are also null values.
          if(qval->is_null())
            return null_value;
          else if(!qval->is_object())
            ASTERIA_THROW("String subscript inapplicable (value `$1`, key `$2`)", *qval, altr.key);

          // Get the value with the given key.
          const auto& obj = qval->as_object();
          auto it = obj.find(altr.key);
          if(it == obj.end())
            return null_value;

          qval = &(it->second);
          break;
        }

        case mindex_array_head: {
          // Elements of null values are also null values.
          if(qval->is_null())
            return null_value;
          else if(!qval->is_array())
            ASTERIA_THROW("Head operator inapplicable (value `$1`)", *qval);

          // Get the first element.
          const auto& arr = qval->as_array();
          if(arr.empty())
            return null_value;

          qval = &(arr.front());
          break;
        }

        case mindex_array_tail: {
          // Elements of null values are also null values.
          if(qval->is_null())
            return null_value;
          else if(!qval->is_array())
            ASTERIA_THROW("Tail operator inapplicable (value `$1`)", *qval);

          // Get the last element.
          const auto& arr = qval->as_array();
          if(arr.empty())
            return null_value;

          qval = &(arr.back());
          break;
        }

        default:
          ASTERIA_TERMINATE("invalid reference modifier type (index `$1`)", this->index());
      }
    }
    return *qval;
  }

Value&
Reference::
dereference_mutable()
  const
  {
    Value* qval;

    switch(this->index()) {
      case index_uninit:
        ASTERIA_THROW("Attempt to use a bypassed variable or reference");

      case index_void:
        ASTERIA_THROW("Attempt to use the result of a function call which returned no value");

      case index_constant:
        ASTERIA_THROW("Attempt to modify a constant `$1`", this->m_root.as<index_constant>().val);

      case index_temporary:
        ASTERIA_THROW("Attempt to modify a temporary `$1`", this->m_root.as<index_temporary>().val);

      case index_variable: {
        auto var = unerase_cast<Variable*>(this->m_root.as<index_variable>().var);
        if(!var)
          ASTERIA_THROW("Attempt to dereference a moved-away reference");

        if(!var->is_initialized())
          ASTERIA_THROW("Attempt to modify an uninitialized variable");

        if(var->is_immutable())
          ASTERIA_THROW("Attempt to modify an immutable variable `$1`", var->get_value());

        qval = &(var->open_value());
        break;
      }

      case index_ptc_args:
        ASTERIA_THROW("Tail call wrapper not dereferenceable");

      case index_jump_src:
        ASTERIA_THROW("Jump marker not dereferenceable");

      default:
        ASTERIA_TERMINATE("invalid reference type enumeration (index `$1`)", this->index());
    }

    // If there is no modifier, return the root verbatim.
    if(ROCKET_EXPECT(this->m_mods.empty()))
      return *qval;

    // We get to apply modifiers, one by one.
    for(size_t k = 0;  k != this->m_mods.size();  ++k) {
      const auto& mod = this->m_mods[k];
      switch(MIndex(mod.index())) {
        case mindex_array_index: {
          const auto& altr = mod.as<mindex_array_index>();

          // Create values as needed.
          if(qval->is_null())
            *qval = V_array();
          else if(!qval->is_array())
            ASTERIA_THROW("Integer subscript inapplicable (value `$1`, index `$2`)", *qval, altr.index);

          // Get the element at the given index.
          auto& arr = qval->open_array();
          auto w = wrap_index(altr.index, arr.size());
          if(w.nprepend)
            arr.insert(arr.begin(), w.nprepend);
          else if(w.nappend)
            arr.append(w.nappend);
          qval = arr.mut_data() + w.rindex;
          break;
        }

        case mindex_object_key: {
          const auto& altr = mod.as<mindex_object_key>();

          // Create values as needed.
          if(qval->is_null())
            *qval = V_object();
          else if(!qval->is_object())
            ASTERIA_THROW("String subscript inapplicable (value `$1`, key `$2`)", *qval, altr.key);

          // Get the value with the given key.
          auto& obj = qval->open_object();
          auto it = obj.try_emplace(altr.key).first;
          qval = &(it->second);
          break;
        }

        case mindex_array_head: {
          // Create values as needed.
          if(qval->is_null())
            *qval = V_array();
          else if(!qval->is_array())
            ASTERIA_THROW("Head operator inapplicable (value `$1`)", *qval);

          // Prepend a copy of the first element.
          auto& arr = qval->open_array();
          auto it = arr.insert(arr.begin(), arr.empty() ? null_value : arr.front());
          qval = &*it;
          break;
        }

        case mindex_array_tail: {
          // Create values as needed.
          if(qval->is_null())
            *qval = V_array();
          else if(!qval->is_array())
            ASTERIA_THROW("Tail operator inapplicable (value `$1`)", *qval);

          // Append a copy of the last element.
          auto& arr = qval->open_array();
          auto& back = arr.emplace_back(arr.empty() ? null_value : arr.back());
          qval = &back;
          break;
        }

        default:
          ASTERIA_TERMINATE("invalid reference modifier type (index `$1`)", this->index());
      }
    }
    return *qval;
  }

Value
Reference::
dereference_unset()
  const
  {
    Value* qval;

    switch(this->index()) {
      case index_uninit:
        ASTERIA_THROW("Attempt to use a bypassed variable or reference");

      case index_void:
        ASTERIA_THROW("Attempt to use the result of a function call which returned no value");

      case index_constant:
        ASTERIA_THROW("Attempt to modify a constant `$1`", this->m_root.as<index_constant>().val);

      case index_temporary:
        ASTERIA_THROW("Attempt to modify a temporary `$1`", this->m_root.as<index_temporary>().val);

      case index_variable: {
        auto var = unerase_cast<Variable*>(this->m_root.as<index_variable>().var);
        if(!var)
          ASTERIA_THROW("Attempt to dereference a moved-away reference");

        if(!var->is_initialized())
          ASTERIA_THROW("Attempt to modify an uninitialized variable");

        if(var->is_immutable())
          ASTERIA_THROW("Attempt to modify an immutable variable `$1`", var->get_value());

        qval = &(var->open_value());
        break;
      }

      case index_ptc_args:
        ASTERIA_THROW("Tail call wrapper not dereferenceable");

      case index_jump_src:
        ASTERIA_THROW("Jump marker not dereferenceable");

      default:
        ASTERIA_TERMINATE("invalid reference type enumeration (index `$1`)", this->index());
    }

    // If there is no modifier, fail.
    if(this->m_mods.empty())
      ASTERIA_THROW("Non-member values cannot be unset");

    // We get to apply modifiers other than the last one.
    for(size_t k = 0;  k != this->m_mods.size() - 1;  ++k) {
      const auto& mod = this->m_mods[k];
      switch(MIndex(mod.index())) {
        case mindex_array_index: {
          const auto& altr = mod.as<mindex_array_index>();

          // Elements of null values are also null values.
          if(qval->is_null())
            return nullopt;
          else if(!qval->is_array())
            ASTERIA_THROW("Integer subscript inapplicable (value `$1`, index `$2`)", *qval, altr.index);

          // Get the element at the given index.
          auto& arr = qval->open_array();
          auto w = wrap_index(altr.index, arr.size());
          if(w.nprepend | w.nappend)
            return nullopt;

          qval = &(arr.mut(w.rindex));
          break;
        }

        case mindex_object_key: {
          const auto& altr = mod.as<mindex_object_key>();

          // Members of null values are also null values.
          if(qval->is_null())
            return nullopt;
          else if(!qval->is_object())
            ASTERIA_THROW("String subscript inapplicable (value `$1`, key `$2`)", *qval, altr.key);

          // Get the value with the given key.
          auto& obj = qval->open_object();
          auto it = obj.mut_find(altr.key);
          if(it == obj.end())
            return nullopt;

          qval = &(it->second);
          break;
        }

        case mindex_array_head: {
          // Elements of null values are also null values.
          if(qval->is_null())
            return nullopt;
          else if(!qval->is_array())
            ASTERIA_THROW("Head operator inapplicable (value `$1`)", *qval);

          // Get the first element.
          auto& arr = qval->open_array();
          if(arr.empty())
            return nullopt;

          qval = &(arr.mut_front());
          break;
        }

        case mindex_array_tail: {
          // Elements of null values are also null values.
          if(qval->is_null())
            return nullopt;
          else if(!qval->is_array())
            ASTERIA_THROW("Tail operator inapplicable (value `$1`)", *qval);

          // Get the last element.
          auto& arr = qval->open_array();
          if(arr.empty())
            return nullopt;

          qval = &(arr.mut_back());
          break;
        }

        default:
          ASTERIA_TERMINATE("invalid reference modifier type (index `$1`)", this->index());
      }
    }

    // Apply the last modifier.
    const auto& mod = this->m_mods.back();
    switch(MIndex(mod.index())) {
      case mindex_array_index: {
        const auto& altr = mod.as<mindex_array_index>();

        // Elements of null values are also null values.
        if(qval->is_null())
          return nullopt;
        else if(!qval->is_array())
          ASTERIA_THROW("Integer subscript inapplicable (value `$1`, index `$2`)", *qval, altr.index);

        // Get the element at the given index.
        auto& arr = qval->open_array();
        auto w = wrap_index(altr.index, arr.size());
        if(w.nprepend | w.nappend)
          return nullopt;

        auto val = ::std::move(arr.mut(w.rindex));
        arr.erase(w.rindex, 1);
        return val;
      }

      case mindex_object_key: {
        const auto& altr = mod.as<mindex_object_key>();

        // Members of null values are also null values.
        if(qval->is_null())
          return nullopt;
        else if(!qval->is_object())
          ASTERIA_THROW("String subscript inapplicable (value `$1`, key `$2`)", *qval, altr.key);

        // Get the value with the given key.
        auto& obj = qval->open_object();
        auto it = obj.mut_find(altr.key);
        if(it == obj.end())
          return nullopt;

        auto val = ::std::move(it->second);
        obj.erase(it);
        return val;
      }

      case mindex_array_head: {
        // Elements of null values are also null values.
        if(qval->is_null())
          return nullopt;
        else if(!qval->is_array())
          ASTERIA_THROW("Head operator inapplicable (value `$1`)", *qval);

        // Get the first element.
        auto& arr = qval->open_array();
        if(arr.empty())
          return nullopt;

        auto val = ::std::move(arr.mut_front());
        arr.erase(arr.begin());
        return val;
      }

      case mindex_array_tail: {
        // Elements of null values are also null values.
        if(qval->is_null())
          return nullopt;
        else if(!qval->is_array())
          ASTERIA_THROW("Tail operator inapplicable (value `$1`)", *qval);

        // Get the last element.
        auto& arr = qval->open_array();
        if(arr.empty())
          return nullopt;

        auto val = ::std::move(arr.mut_back());
        arr.pop_back();
        return val;
      }

      default:
        ASTERIA_TERMINATE("invalid reference modifier type (index `$1`)", this->index());
    }
  }

Value&
Reference::
do_mutate_into_temporary_slow()
  {
    // Otherwise, try dereferencing.
    S_temporary xref = { this->dereference_readonly() };
    this->m_root = ::std::move(xref);
    this->m_mods.clear();

    ROCKET_ASSERT(this->m_root.index() == index_temporary);
    return this->m_root.as<index_temporary>().val;
  }

}  // namespace asteria
