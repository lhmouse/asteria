// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference.hpp"
#include "../utilities.hpp"

namespace Asteria {

Reference::~Reference()
  {
  }

void Reference::do_throw_unset_no_modifier() const
  {
    ASTERIA_THROW_RUNTIME_ERROR("Only array elements or object members may be `unset`.");
  }

const Value & Reference::do_read_with_modifiers() const
  {
    // Dereference the root.
    auto cur = std::ref(this->m_root.dereference_const());
    // Apply modifiers.
    const auto epos = this->m_mods.size();
    for(std::size_t i = 0; i != epos; ++i) {
      const auto qnext = this->m_mods.at(i).apply_const_opt(cur);
      if(!qnext) {
        return Value::get_null();
      }
      cur = std::ref(*qnext);
    }
    return cur;
  }

Value & Reference::do_open_with_modifiers() const
  {
    // Dereference the root.
    auto cur = std::ref(this->m_root.dereference_mutable());
    // Apply modifiers.
    const auto epos = this->m_mods.size();
    for(std::size_t i = 0; i != epos; ++i) {
      const auto qnext = this->m_mods.at(i).apply_mutable_opt(cur, true);
      if(!qnext) {
        ROCKET_ASSERT(false);
      }
      cur = std::ref(*qnext);
    }
    return cur;
  }

Value Reference::do_unset_with_modifiers() const
  {
    // Dereference the root.
    auto cur = std::ref(this->m_root.dereference_mutable());
    // Apply modifiers.
    const auto epos = this->m_mods.size() - 1;
    for(std::size_t i = 0; i != epos; ++i) {
      const auto qnext = this->m_mods.at(i).apply_mutable_opt(cur, false);
      if(!qnext) {
        return D_null();
      }
      cur = std::ref(*qnext);
    }
    return this->m_mods.at(epos).apply_and_erase(cur);
  }

void Reference::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    this->m_root.enumerate_variables(callback);
  }

}
