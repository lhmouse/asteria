// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "evaluation_stack.hpp"
#include "executive_context.hpp"
#include "runtime_error.hpp"
#include "variable.hpp"
#include "ptc_arguments.hpp"
#include "enums.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

Runtime_Error&
do_unpack_frames(Runtime_Error& except, Global_Context& global, Evaluation_Stack& stack,
                 cow_vector<rcptr<PTC_Arguments>>&& frames)
  {
    while(frames.size()) {
      auto tca = ::std::move(frames.mut_back());
      frames.pop_back();

      // Push the function call.
      except.push_frame_plain(tca->sloc(), ::rocket::sref("<proper tail call>"));

      // Call the hook function if any.
      const auto qhooks = global.get_hooks_opt();
      if(qhooks)
        qhooks->on_function_except(tca->sloc(), tca->zvarg()->func(), except);

      // Evaluate deferred expressions if any.
      if(tca->get_defer_stack().size())
        Executive_Context(::rocket::ref(global), ::rocket::ref(stack),
                          ::rocket::ref(tca->zvarg()), ::std::move(tca->open_defer_stack()))
          .on_scope_exit(except);

      // Push the caller.
      except.push_frame_func(tca->zvarg()->sloc(), tca->zvarg()->func());
    }
    return except;
  }

Reference&
do_unpack_tail_calls(Reference& self, Global_Context& global)
  {
    // The function call shall yield an rvalue unless all wrapped calls return by reference.
    PTC_Aware ptc_conj = ptc_aware_by_ref;
    rcptr<PTC_Arguments> tca;
    // We must rebuild the backtrace using this queue if an exception is thrown.
    cow_vector<rcptr<PTC_Arguments>> frames;
    Evaluation_Stack stack;

    // Unpack all frames recursively.
    // Note that `self` is overwritten before the wrapped function is called.
    while(!!(tca = self.get_tail_call_opt())) {
      // Figure out how to forward the result.
      if(tca->ptc_aware() == ptc_aware_void) {
        ptc_conj = ptc_aware_void;
      }
      else if((tca->ptc_aware() == ptc_aware_by_val) && (ptc_conj == ptc_aware_by_ref)) {
        ptc_conj = ptc_aware_by_val;
      }
      // Record this frame.
      frames.emplace_back(tca);

      // Generate a single-step trap.
      const auto qhooks = global.get_hooks_opt();
      if(qhooks)
        qhooks->on_single_step_trap(tca->sloc(), tca->zvarg()->func(), nullptr);

      // Get the `this` reference and all the other arguments.
      const auto& target = tca->get_target();
      auto args = ::std::move(tca->open_arguments_and_self());
      self = ::std::move(args.mut_back());
      args.pop_back();

      // Call the hook function if any.
      if(qhooks)
        qhooks->on_function_call(tca->sloc(), tca->zvarg()->func(), target);

      // Perform a non-tail call.
      ASTERIA_RUNTIME_TRY {
        target.invoke_ptc_aware(self, global, ::std::move(args));
      }
      ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
        do_unpack_frames(except, global, stack, ::std::move(frames));
        throw;
      }
    }

    // Check for deferred expressions.
    while(frames.size()) {
      tca = ::std::move(frames.mut_back());
      frames.pop_back();
      // Evaluate deferred expressions if any.
      ASTERIA_RUNTIME_TRY {
        if(tca->get_defer_stack().size())
          Executive_Context(::rocket::ref(global), ::rocket::ref(stack),
                            ::rocket::ref(tca->zvarg()), ::std::move(tca->open_defer_stack()))
            .on_scope_exit(air_status_next);
      }
      ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
        do_unpack_frames(except, global, stack, ::std::move(frames));
        throw;
      }
    }

    // Process the result.
    if(ptc_conj == ptc_aware_void) {
      // Return `void`.
      self = Reference_root::S_void();
    }
    else if((ptc_conj == ptc_aware_by_val) && self.is_glvalue()) {
      // Convert the result to an rvalue if it shouldn't be passed by reference.
      Reference_root::S_temporary xref = { self.read() };
      self = ::std::move(xref);
    }
    return self;
  }

}  // namespace

Reference::
~Reference()
  {
  }

void
Reference::
do_throw_unset_no_modifier()
const
  {
    ASTERIA_THROW("non-members can't be unset");
  }

const Value&
Reference::
do_read(size_t nmod, const Reference_modifier& last)
const
  {
    auto qref = ::std::addressof(this->m_root.dereference_const());

    // Apply the first `nmod` modifiers.
    for(size_t i = 0;  i < nmod;  ++i)
      if(!(qref = this->m_mods[i].apply_const_opt(*qref)))
        return null_value;

    // Apply the last modifier.
    if(!(qref = last.apply_const_opt(*qref)))
      return null_value;

    return *qref;
  }

Value&
Reference::
do_open(size_t nmod, const Reference_modifier& last)
const
  {
    auto rref = ::std::ref(this->m_root.dereference_mutable());

    // Apply the first `nmod` modifiers.
    for(size_t i = 0;  i < nmod;  ++i)
      rref = ::std::ref(this->m_mods[i].apply_and_create(rref));

    // Apply the last modifier.
    rref = ::std::ref(last.apply_and_create(rref));

    return rref;
  }

Value
Reference::
do_unset(size_t nmod, const Reference_modifier& last)
const
  {
    auto qref = ::std::addressof(this->m_root.dereference_mutable());

    // Apply the first `nmod` modifiers.
    for(size_t i = 0;  i < nmod;  ++i)
      if(!(qref = this->m_mods[i].apply_mutable_opt(*qref)))
        return V_null();

    // Apply the last modifier.
    auto val = last.apply_and_erase(*qref);

    return val;
  }

Reference&
Reference::
do_finish_call(Global_Context& global)
  {
    return do_unpack_tail_calls(*this, global);
  }

Variable_Callback&
Reference::
enumerate_variables(Variable_Callback& callback)
const
  {
    return this->m_root.enumerate_variables(callback);
  }

}  // namespace Asteria
