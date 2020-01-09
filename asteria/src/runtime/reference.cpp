// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "runtime_error.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Reference::do_throw_unset_no_modifier() const
  {
    ASTERIA_THROW("non-members can't be unset");
  }

const Value& Reference::do_read(const Reference_Modifier* mods, size_t nmod, const Reference_Modifier& last) const
  {
    auto qref = ::std::addressof(this->m_root.dereference_const());
    for(size_t i = 0; i != nmod; ++i) {
      // Apply a modifier.
      qref = mods[i].apply_const_opt(*qref);
      if(!qref)
        return null_value;
    }
    // Apply the last modifier.
    qref = last.apply_const_opt(*qref);
    if(!qref)
      return null_value;
    return *qref;
  }

Value& Reference::do_open(const Reference_Modifier* mods, size_t nmod, const Reference_Modifier& last) const
  {
    auto qref = ::std::addressof(this->m_root.dereference_mutable());
    for(size_t i = 0; i != nmod; ++i) {
      // Apply a modifier.
      qref = mods[i].apply_mutable_opt(*qref, true);  // create new
      ROCKET_ASSERT(qref);
    }
    // Apply the last modifier.
    qref = last.apply_mutable_opt(*qref, true);
    ROCKET_ASSERT(qref);
    return *qref;
  }

Value Reference::do_unset(const Reference_Modifier* mods, size_t nmod, const Reference_Modifier& last) const
  {
    auto qref = ::std::addressof(this->m_root.dereference_mutable());
    for(size_t i = 0; i != nmod; ++i) {
      // Apply a modifier.
      qref = mods[i].apply_mutable_opt(*qref, false);  // no create
      if(!qref)
        return null_value;
    }
    // Apply the last modifier.
    return last.apply_and_erase(*qref);
  }

Reference& Reference::do_finish_call(Global_Context& global)
  {
    // Note that `*this` is overwritten before the wrapped function is called.
    // We must rebuild the backtrace using this queue if an exception is thrown.
    cow_vector<rcptr<Tail_Call_Arguments>> frames;
    // The function call shall yield an rvalue unless all wrapped calls return by reference.
    PTC_Aware ptc_conj = ptc_aware_by_ref;
    // Unpack all tail call wrappers.
    while(this->m_root.is_tail_call()) {
      // Unpack a frame.
      auto& tca = frames.emplace_back(this->m_root.as_tail_call());
      const auto& sloc = tca->sloc();
      const auto& inside = tca->inside();
      const auto& qhooks = global.get_hooks_opt();
      // Generate a single-step trap.
      if(qhooks) {
        qhooks->on_single_step_trap(sloc, inside, nullptr);
      }
      // Tell out how to forward the result.
      if((tca->ptc() == ptc_aware_by_val) && (ptc_conj == ptc_aware_by_ref)) {
        ptc_conj = ptc_aware_by_val;
      }
      else if(tca->ptc() == ptc_aware_prune) {
        ptc_conj = ptc_aware_prune;
      }
      // Unpack the function reference.
      const auto& target = tca->get_target();
      // Unpack arguments.
      auto args = ::rocket::move(tca->open_arguments_and_self());
      *this = ::rocket::move(args.mut_back());
      args.pop_back();
      // Call the hook function if any.
      if(qhooks) {
        qhooks->on_function_call(sloc, inside, target);
      }
      // Unwrap the function call.
      try {
        try {
          target->invoke(*this, global, ::rocket::move(args));
        }
        catch(Runtime_Error& /*except*/) {
          throw;
        }
        catch(exception& stdex) {
          throw Runtime_Error(stdex);
        }
      }
      catch(Runtime_Error& except) {
        // Append all frames that have been expanded so far and rethrow the exception.
        ::std::for_each(frames.rbegin(), frames.rend(),
                        [&](const auto& p) { except.push_frame_func(p->sloc(), p->inside());  });
        // Call the hook function if any.
        if(qhooks) {
          qhooks->on_function_except(sloc, inside, except);
        }
        throw;
      }
      // Call the hook function if any.
      if(qhooks) {
        qhooks->on_function_return(sloc, inside, *this);
      }
    }
    if(ptc_conj == ptc_aware_prune) {
      // Return `void`.
      *this = Reference_Root::S_void();
    }
    else {
      // Ensure the result is dereferenceable.
      const auto& val = this->read();
      if((ptc_conj == ptc_aware_by_val) && this->is_glvalue()) {
        // Convert the result to an rvalue if it shouldn't be passed by reference.
        Reference_Root::S_temporary xref = { val };
        *this = ::rocket::move(xref);
      }
    }
    return *this;
  }

Variable_Callback& Reference::enumerate_variables(Variable_Callback& callback) const
  {
    return this->m_root.enumerate_variables(callback);
  }

}  // namespace Asteria
