// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference.hpp"
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

void Reference::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    return this->m_root.enumerate_variables(callback);
  }

}  // namespace Asteria
