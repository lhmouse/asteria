// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "runtime_error.hpp"
#include "enums.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

Reference& do_unpack_tail_calls(Reference& self, Global_Context& global)
  {
    // Note that `self` is overwritten before the wrapped function is called.
    auto tca = self.get_tail_call_opt();
    if(ROCKET_EXPECT(!tca)) {
      return self;
    }
    // The function call shall yield an rvalue unless all wrapped calls return by reference.
    PTC_Aware ptc_conj = ptc_aware_by_ref;
    // We must rebuild the backtrace using this queue if an exception is thrown.
    cow_vector<rcptr<Tail_Call_Arguments>> frames;

    do {
      // Figure out how to forward the result.
      if(tca->ptc() == ptc_aware_prune) {
        ptc_conj = ptc_aware_prune;
      }
      else if((tca->ptc() == ptc_aware_by_val) && (ptc_conj == ptc_aware_by_ref)) {
        ptc_conj = ptc_aware_by_val;
      }
      // Record a frame.
      frames.emplace_back(tca);

      // Unpack arguments.
      const auto& sloc = tca->sloc();
      const auto& inside = tca->inside();
      const auto& qhooks = global.get_hooks_opt();

      // Generate a single-step trap.
      if(qhooks) {
        qhooks->on_single_step_trap(sloc, inside, nullptr);
      }
      // Get the `this` reference and all the other arguments.
      const auto& target = tca->get_target();
      auto args = ::rocket::move(tca->open_arguments_and_self());
      self = ::rocket::move(args.mut_back());
      args.pop_back();
      // Call the hook function if any.
      if(qhooks) {
        qhooks->on_function_call(sloc, inside, target);
      }
      // Perform a non-tail call.
      ASTERIA_RUNTIME_TRY {
        target->invoke_ptc_aware(self, global, ::rocket::move(args));
      }
      ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
        // Append all frames that have been unpacked so far and rethrow the exception.
        while(!frames.empty()) {
          tca = ::rocket::move(frames.mut_back());
          except.push_frame_func(tca->sloc(), tca->inside());
          frames.pop_back();
        }
        // Call the hook function if any.
        if(qhooks) {
          qhooks->on_function_except(sloc, inside, except);
        }
        throw;
      }
      // Call the hook function if any.
      if(qhooks) {
        qhooks->on_function_return(sloc, inside, self);
      }
      // Get the next frame.
    } while(tca = self.get_tail_call_opt());

    // Process the result.
    if(ptc_conj == ptc_aware_prune) {
      // Return `void`.
      self = Reference_Root::S_void();
    }
    else if((ptc_conj == ptc_aware_by_val) && self.is_glvalue()) {
      // Convert the result to an rvalue if it shouldn't be passed by reference.
      Reference_Root::S_temporary xref = { self.read() };
      self = ::rocket::move(xref);
    }
    return self;
  }

}  // namespace

void Reference::do_throw_unset_no_modifier() const
  {
    ASTERIA_THROW("non-members can't be unset");
  }

const Value& Reference::do_read(const Modifier* mods, size_t nmod, const Modifier& last) const
  {
    auto qref = ::std::addressof(this->m_root.dereference_const());
    for(size_t i = 0;  i < nmod;  ++i) {
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

Value& Reference::do_open(const Modifier* mods, size_t nmod, const Modifier& last) const
  {
    auto qref = ::std::addressof(this->m_root.dereference_mutable());
    for(size_t i = 0;  i < nmod;  ++i) {
      // Apply a modifier.
      qref = mods[i].apply_mutable_opt(*qref, true);  // create new
      ROCKET_ASSERT(qref);
    }
    // Apply the last modifier.
    qref = last.apply_mutable_opt(*qref, true);
    ROCKET_ASSERT(qref);
    return *qref;
  }

Value Reference::do_unset(const Modifier* mods, size_t nmod, const Modifier& last) const
  {
    auto qref = ::std::addressof(this->m_root.dereference_mutable());
    for(size_t i = 0;  i < nmod;  ++i) {
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
    return do_unpack_tail_calls(*this, global);
  }

Variable_Callback& Reference::enumerate_variables(Variable_Callback& callback) const
  {
    return this->m_root.enumerate_variables(callback);
  }

}  // namespace Asteria
