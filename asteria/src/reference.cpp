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
    const auto end = this->m_mods.end();
    for(auto it = this->m_mods.begin(); it != end; ++it) {
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
    const auto end = this->m_mods.end();
    for(auto it = this->m_mods.begin(); it != end; ++it) {
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
    if(this->m_mods.empty()) {
      ASTERIA_THROW_RUNTIME_ERROR("Only array elements or object members may be `unset`.");
    }
    // Dereference the root.
    auto cur = std::ref(this->m_root.dereference_mutable());
    // Apply modifiers.
    const auto end = this->m_mods.end() - 1;
    for(auto it = this->m_mods.begin(); it != end; ++it) {
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

Reference & Reference::zoom_in(Reference_modifier mod)
  {
    // Append a modifier.
    this->m_mods.emplace_back(std::move(mod));
    return *this;
  }

Reference & Reference::zoom_out()
  {
    if(this->m_mods.empty()) {
      // If there is no modifier, set `*this` to a null reference.
      Reference_root::S_constant ref_c = { D_null() };
      *this = std::move(ref_c);
      return *this;
    }
    // Drop the last modifier.
    this->m_mods.pop_back();
    return *this;
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

Reference & Reference::convert_to_variable(Global_context &global)
  {
    if(this->m_root.index() == Reference_root::index_variable) {
      return *this;
    }
    // Create an lvalue by allocating a variable and assign it to `*this`.
    auto var = global.create_tracked_variable();
    var->reset(this->read(), false);
    Reference_root::S_variable ref_c = { std::move(var) };
    *this = std::move(ref_c);
    return *this;
  }

void Reference::enumerate_variables(const Abstract_variable_callback &callback) const
  {
    this->m_root.enumerate_variables(callback);
  }

}
