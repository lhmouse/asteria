// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference.hpp"
#include "exception.hpp"
#include "../utilities.hpp"

namespace Asteria {

Value Reference::do_throw_unset_no_modifier() const
  {
    ASTERIA_THROW_RUNTIME_ERROR("Only array elements or object members may be `unset`.");
  }

const Value& Reference::do_read(const Reference_Modifier* mods, size_t nmod, const Reference_Modifier& last) const
  {
    auto qref = std::addressof(this->m_root.dereference_const());
    for(size_t i = 0; i != nmod; ++i) {
      // Apply a modifier.
      qref = mods[i].apply_const_opt(*qref);
      if(!qref) {
        return null_value;
      }
    }
    // Apply the last modifier.
    qref = last.apply_const_opt(*qref);
    if(!qref) {
      return null_value;
    }
    return *qref;
  }

Value& Reference::do_open(const Reference_Modifier* mods, size_t nmod, const Reference_Modifier& last) const
  {
    auto qref = std::addressof(this->m_root.dereference_mutable());
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
    auto qref = std::addressof(this->m_root.dereference_mutable());
    for(size_t i = 0; i != nmod; ++i) {
      // Apply a modifier.
      qref = mods[i].apply_mutable_opt(*qref, false);  // no create
      if(!qref) {
        return null_value;
      }
    }
    // Apply the last modifier.
    return last.apply_and_erase(*qref);
  }

Reference& Reference::do_convert_to_temporary()
  {
    // Replace `*this` with a reference to temporary.
    Reference_Root::S_temporary xref = { this->read() };
    *this = rocket::move(xref);
    return *this;
  }

Reference& Reference::do_finish_call(const Global_Context& global)
  {
    // Note that `*this` is overwritten before the wrapped function is called.
    // We must rebuild the backtrace using this queue if an exception is thrown.
    cow_vector<rcptr<Tail_Call_Arguments>> frames;
    // The function call shall yield an rvalue unless all wrapped calls return by reference.
    TCO_Aware tco_conj = tco_aware_by_ref;
    // Unpack all tail call wrappers.
    while(this->m_root.is_tail_call()) {
      auto& tca = frames.emplace_back(this->m_root.as_tail_call());
      // Tell out how to forward the result.
      if((tca->get_tco_awareness() == tco_aware_by_val) && (tco_conj == tco_aware_by_ref)) {
        tco_conj = tco_aware_by_val;
      }
      else if(tca->get_tco_awareness() == tco_aware_nullify) {
        tco_conj = tco_aware_nullify;
      }
      // Unpack the function reference.
      const auto& target = tca->get_target();
      // Unpack arguments.
      auto args = rocket::move(tca->open_arguments_and_self());
      *this = rocket::move(args.mut_back());
      args.pop_back();
      // Call the function now.
      const auto& sloc = tca->get_source_location();
      const auto& func = tca->get_function_signature();
      try {
        // Unwrap the function call.
        ASTERIA_DEBUG_LOG("Unpacking tail call at \'", sloc, "\' inside `", func, "`: target = ", *target);
        target->invoke(*this, global, rocket::move(args));
        // The result will have been stored into `*this`.
        ASTERIA_DEBUG_LOG("Returned from tail call at \'", sloc, "\' inside `", func, "`: target = ", *target);
      }
      catch(Exception& except) {
        ASTERIA_DEBUG_LOG("Caught `Asteria::Exception` thrown inside tail call at \'", sloc, "\' inside `", func, "`: ", except.get_value());
        // Append all frames that have been expanded so far and rethrow the exception.
        std::for_each(frames.rbegin(), frames.rend(), [&](const auto& p) { except.push_frame_func(p->get_source_location(),
                                                                                                  p->get_function_signature());  });
        throw;
      }
      catch(const std::exception& stdex) {
        ASTERIA_DEBUG_LOG("Caught `std::exception` thrown inside function call at \'", sloc, "\' inside `", func, "`: ", stdex.what());
        // Translate the exception, append all frames that have been expanded so far, and throw the new exception.
        Exception except(stdex);
        std::for_each(frames.rbegin(), frames.rend(), [&](const auto& p) { except.push_frame_func(p->get_source_location(),
                                                                                                  p->get_function_signature());  });
        throw except;
      }
    }
    if(tco_conj == tco_aware_by_val) {
      // Convert the result to an rvalue if it shouldn't be passed by reference.
      this->convert_to_rvalue();
    }
    else if(tco_conj == tco_aware_nullify) {
      // Return a `null`.
      *this = Reference_Root::S_null();
    }
    return *this;
  }

Variable_Callback& Reference::enumerate_variables(Variable_Callback& callback) const
  {
    return this->m_root.enumerate_variables(callback);
  }

}  // namespace Asteria
