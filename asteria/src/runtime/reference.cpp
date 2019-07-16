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

const Value& Reference::do_read(const Reference_Modifier* mods, std::size_t nmod, const Reference_Modifier& last) const
  {
    auto qref = std::addressof(this->m_root.dereference_const());
    for(std::size_t i = 0; i != nmod; ++i) {
      // Apply a modifier.
      qref = mods[i].apply_const_opt(*qref);
      if(!qref) {
        return Value::null();
      }
    }
    // Apply the last modifier.
    return (qref = last.apply_const_opt(*qref)) ? *qref : Value::null();
  }

Value& Reference::do_open(const Reference_Modifier* mods, std::size_t nmod, const Reference_Modifier& last) const
  {
    auto qref = std::addressof(this->m_root.dereference_mutable());
    for(std::size_t i = 0; i != nmod; ++i) {
      // Apply a modifier.
      qref = mods[i].apply_mutable_opt(*qref, true);  // create new
      if(!qref) {
        ROCKET_ASSERT(false);
      }
    }
    // Apply the last modifier.
    return (qref = last.apply_mutable_opt(*qref, true)), ROCKET_ASSERT(qref), *qref;
  }

Value Reference::do_unset(const Reference_Modifier* mods, std::size_t nmod, const Reference_Modifier& last) const
  {
    auto qref = std::addressof(this->m_root.dereference_mutable());
    for(std::size_t i = 0; i != nmod; ++i) {
      // Apply a modifier.
      qref = mods[i].apply_mutable_opt(*qref, false);  // no create
      if(!qref) {
        return Value::null();
      }
    }
    // Apply the last modifier.
    return last.apply_and_erase(*qref);
  }

Reference& Reference::do_unwrap_tail_calls(const Global_Context& global)
  {
    Cow_Bivector<Source_Location, Cow_String> backtrace;
    Reference self;
    Cow_Vector<Reference> args;
    for(;;) {
      // Unpack a tail call.
      auto target = this->m_root.unpack_tail_call_opt(backtrace, self, args);
      if(!target) {
        break;
      }
      const auto& sloc = backtrace.back().first;
      const auto& func = backtrace.back().second;
      try {
        // Unwrap the function call.
        ASTERIA_DEBUG_LOG("Unpacking tail call at \'", sloc, "\' inside `", func, "`: target = ", *target);
        target->invoke(self, global, rocket::move(args));
        // The result will have been stored into `self`.
        ASTERIA_DEBUG_LOG("Returned from tail call at \'", sloc, "\' inside `", func, "`: target = ", *target, ", result = ", self.read());
      }
      catch(Exception& except) {
        ASTERIA_DEBUG_LOG("Caught `Asteria::Exception` thrown inside tail call at \'", sloc, "\' inside `", func, "`: ", except.get_value());
        // Append all frames that have been expanded so far and rethrow the exception.
        std::for_each(backtrace.rbegin(), backtrace.rend(), [&](const auto& p) { except.push_frame_func(p.first, p.second);  });
        throw;
      }
      catch(const std::exception& stdex) {
        ASTERIA_DEBUG_LOG("Caught `std::exception` thrown inside function call at \'", sloc, "\' inside `", func, "`: ", stdex.what());
        // Translate the exception, append all frames that have been expanded so far, and throw the new exception.
        Exception except(stdex);
        std::for_each(backtrace.rbegin(), backtrace.rend(), [&](const auto& p) { except.push_frame_func(p.first, p.second);  });
        throw except;
      }
      *this = rocket::move(self);
    }
    return *this;
  }

void Reference::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    this->m_root.enumerate_variables(callback);
  }

}  // namespace Asteria
