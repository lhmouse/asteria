// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference::~Reference()
  {
  }

void Reference::do_throw_unset_no_modifier() const
  {
    ASTERIA_THROW_RUNTIME_ERROR("Only array elements or object members may be `unset`.");
  }

    namespace {

    union Constant_null
      {
        Value value;

        Constant_null() noexcept
          {
          }
        ~Constant_null()
          {
          }

        operator const Value & () const noexcept
          {
            return this->value;
          }
      }
    const s_null;  // Don't play with this at home.

    }

const Value & Reference::do_read_with_modifiers() const
  {
    // Dereference the root.
    auto cur = std::ref(this->m_root.dereference_const());
    // Apply modifiers.
    const auto end = this->m_mods.end();
    for(auto it = this->m_mods.begin(); it != end; ++it) {
      const auto qnext = it->apply_const_opt(cur);
      if(!qnext) {
        return s_null;
      }
      cur = std::ref(*qnext);
    }
    return cur;
  }

Value & Reference::do_mutate_with_modifiers() const
  {
    // Dereference the root.
    auto cur = std::ref(this->m_root.dereference_mutable());
    // Apply modifiers.
    const auto end = this->m_mods.end();
    for(auto it = this->m_mods.begin(); it != end; ++it) {
      const auto qnext = it->apply_mutable_opt(cur, true);
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
    const auto end = this->m_mods.end() - 1;
    for(auto it = this->m_mods.begin(); it != end; ++it) {
      const auto qnext = it->apply_mutable_opt(cur, false);
      if(!qnext) {
        return D_null();
      }
      cur = std::ref(*qnext);
    }
    return end->apply_and_erase(cur);
  }

}
