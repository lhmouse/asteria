// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference::~Reference()
  {
  }

Reference::Reference(const Reference &) noexcept
  = default;
Reference & Reference::operator=(const Reference &) noexcept
  = default;
Reference::Reference(Reference &&) noexcept
  = default;
Reference & Reference::operator=(Reference &&) noexcept
  = default;

Value Reference::read() const
  {
    // Dereference the root.
    auto cur = std::ref(this->m_root.dereference_readonly());
    // Apply modifiers.
    const auto end = this->m_modifiers.end();
    for(auto it = this->m_modifiers.begin(); it != end; ++it) {
      const auto qnext = it->apply_readonly_opt(cur);
      if(!qnext) {
        return { };
      }
      cur = std::ref(*qnext);
    }
    // Return the value found.
    return cur;
  }

Value & Reference::write(Value value) const
  {
    // Dereference the root.
    auto cur = std::ref(this->m_root.dereference_mutable());
    // Apply modifiers.
    const auto end = this->m_modifiers.end();
    for(auto it = this->m_modifiers.begin(); it != end; ++it) {
      const auto qnext = it->apply_mutable_opt(cur, true, nullptr);
      if(!qnext) {
        ROCKET_ASSERT(false);
      }
      cur = std::ref(*qnext);
    }
    // Set the new value.
    cur.get() = std::move(value);
    return cur;
  }

Value Reference::unset() const
  {
    if(this->m_modifiers.empty()) {
      ASTERIA_THROW_RUNTIME_ERROR("Only array elements or object members may be `unset`.");
    }
    // Dereference the root.
    auto cur = std::ref(this->m_root.dereference_mutable());
    // Apply modifiers.
    const auto end = this->m_modifiers.end() - 1;
    for(auto it = this->m_modifiers.begin(); it != end; ++it) {
      const auto qnext = it->apply_mutable_opt(cur, false, nullptr);
      if(!qnext) {
        return { };
      }
      cur = std::ref(*qnext);
    }
    // Erase the element referenced by the last modifier.
    Value erased;
    end->apply_mutable_opt(cur, false, &erased);
    return std::move(erased);
  }

Reference & Reference::zoom_in(Reference_modifier modifier)
  {
    // Append a modifier.
    this->m_modifiers.emplace_back(std::move(modifier));
    return *this;
  }

Reference & Reference::zoom_out()
  {
    if(this->m_modifiers.empty() == false) {
      // Drop the last modifier.
      this->m_modifiers.pop_back();
      return *this;
    }
    // If there is no modifier, set `*this` to a null reference.
    Reference_root::S_constant ref_c = { D_null() };
    this->m_root = std::move(ref_c);
    this->m_modifiers.clear();
    return *this;
  }

Reference & Reference::materialize()
  {
    if(this->m_root.index() == Reference_root::index_variable) {
      return *this;
    }
    // Create an lvalue by allocating a variable and assign it to `*this`.
    auto value = this->read();
    auto var = rocket::make_refcounted<Variable>(std::move(value), false);
    Reference_root::S_variable ref_c = { std::move(var) };
    this->m_root = std::move(ref_c);
    this->m_modifiers.clear();
    return *this;
  }

}
