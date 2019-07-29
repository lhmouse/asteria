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
        return Value::null();
      }
    }
    // Apply the last modifier.
    return (qref = last.apply_const_opt(*qref)) ? *qref : Value::null();
  }

Value& Reference::do_open(const Reference_Modifier* mods, size_t nmod, const Reference_Modifier& last) const
  {
    auto qref = std::addressof(this->m_root.dereference_mutable());
    for(size_t i = 0; i != nmod; ++i) {
      // Apply a modifier.
      qref = mods[i].apply_mutable_opt(*qref, true);  // create new
      if(!qref) {
        ROCKET_ASSERT(false);
      }
    }
    // Apply the last modifier.
    return (qref = last.apply_mutable_opt(*qref, true)), ROCKET_ASSERT(qref), *qref;
  }

Value Reference::do_unset(const Reference_Modifier* mods, size_t nmod, const Reference_Modifier& last) const
  {
    auto qref = std::addressof(this->m_root.dereference_mutable());
    for(size_t i = 0; i != nmod; ++i) {
      // Apply a modifier.
      qref = mods[i].apply_mutable_opt(*qref, false);  // no create
      if(!qref) {
        return Value::null();
      }
    }
    // Apply the last modifier.
    return last.apply_and_erase(*qref);
  }

Reference& Reference::do_finish_call(const Global_Context& global)
  {
    // Note that `*this` is overwritten before the wrapped function is called.
    cow_vector<Reference_Root::S_tail_call> rqueue;
    // The function call shall yield an rvalue unless all wrapped calls return by reference.
    bool conj_ref = true;
    // Unpack all tail call wrappers.
    while(this->m_root.is_tail_call()) {
      auto& xroot = rqueue.emplace_back(rocket::move(this->m_root.open_tail_call()));
      // Unpack the function reference.
      conj_ref &= xroot.by_ref;
      const auto& target = xroot.target;
      // Unpack arguments.
      *this = rocket::move(xroot.args_self.mut_back());
      auto& args = xroot.args_self.pop_back();
      // Call the function now.
      const auto& sloc = xroot.sloc;
      const auto& func = xroot.func;
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
        std::for_each(rqueue.rbegin(), rqueue.rend(), [&](const auto& r) { except.push_frame_func(r.sloc, r.func);  });
        throw;
      }
      catch(const std::exception& stdex) {
        ASTERIA_DEBUG_LOG("Caught `std::exception` thrown inside function call at \'", sloc, "\' inside `", func, "`: ", stdex.what());
        // Translate the exception, append all frames that have been expanded so far, and throw the new exception.
        Exception except(stdex);
        std::for_each(rqueue.rbegin(), rqueue.rend(), [&](const auto& r) { except.push_frame_func(r.sloc, r.func);  });
        throw except;
      }
    }
    if(!this->is_rvalue() && !conj_ref) {
      // If the result is not an rvalue and it is not passed by reference, convert it to an rvalue.
      Reference_Root::S_temporary xroot = { this->read() };
      *this = rocket::move(xroot);
    }
    return *this;
  }

void Reference::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    this->m_root.enumerate_variables(callback);
  }

}  // namespace Asteria
