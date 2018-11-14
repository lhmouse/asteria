// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "global_context.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference::~Reference()
  {
  }

void Reference::do_throw_unset_no_modifier() const
  {
    ASTERIA_THROW_RUNTIME_ERROR("Only array elements or object members may be `unset`.");
  }

Value Reference::do_read_with_modifiers() const
  {
    // Dereference the root.
    auto cur = std::ref(this->m_root.dereference_const());
    // Apply modifiers.
    const auto end = this->m_mods.end();
    for(auto it = this->m_mods.begin(); it != end; ++it) {
      const auto qnext = it->apply_const_opt(cur);
      if(!qnext) {
        return D_null();
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

Reference & Reference::convert_to_temporary()
  {
    if(this->m_root.index() == Reference_root::index_temporary) {
      return *this;
    }
    // Create an rvalue.
    Reference_root::S_temporary ref_c = { this->read() };
    *this = std::move(ref_c);
    return *this;
  }

Reference & Reference::convert_to_variable(Global_context &global, bool immutable)
  {
    if(this->m_root.index() == Reference_root::index_variable) {
      return *this;
    }
    if((this->m_root.index() == Reference_root::index_constant) && immutable) {
      return *this;
    }
    // Create an lvalue by allocating a variable and assign it to `*this`.
    auto var = global.create_tracked_variable(this, immutable);
    Reference_root::S_variable ref_c = { std::move(var) };
    *this = std::move(ref_c);
    return *this;
  }

}
