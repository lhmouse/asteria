// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference Reference::make_constant(Value value)
  {
    Reference_root::S_constant ref_c = { std::move(value) };
    return Reference_root(std::move(ref_c));
  }

Reference Reference::make_temporary(Value value)
  {
    Reference_root::S_temporary ref_c = { std::move(value) };
    return Reference_root(std::move(ref_c));
  }

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

Reference_root & Reference::set_root(Reference_root root, Vector<Reference_modifier> modifiers)
  {
    this->m_root = std::move(root);
    this->m_modifiers = std::move(modifiers);
    return this->m_root;
  }
void Reference::clear_modifiers() noexcept
  {
    this->m_modifiers.clear();
  }
Reference_modifier & Reference::push_modifier(Reference_modifier modifier)
  {
    return this->m_modifiers.emplace_back(std::move(modifier));
  }
void Reference::pop_modifier()
  {
    this->m_modifiers.pop_back();
  }

Value Reference::read() const
  {
    // Dereference the root.
    auto cur = std::ref(this->get_root().dereference_readonly());
    // Apply modifiers.
    const auto mend = this->m_modifiers.end();
    for(auto mit = this->m_modifiers.begin(); mit != mend; ++mit) {
      const auto ptr = mit->apply_readonly_opt(cur);
      if(!ptr) {
        return { };
      }
      cur = std::ref(*ptr);
    }
    // Return the value found.
    return cur;
  }
Value & Reference::write(Value value) const
  {
    // Dereference the root.
    auto cur = std::ref(this->get_root().dereference_mutable());
    // Apply modifiers.
    const auto mend = this->m_modifiers.end();
    for(auto mit = this->m_modifiers.begin(); mit != mend; ++mit) {
      const auto ptr = mit->apply_mutable_opt(cur, true, nullptr);
      if(!ptr) {
        ROCKET_ASSERT(false);
      }
      cur = std::ref(*ptr);
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
    auto cur = std::ref(this->get_root().dereference_mutable());
    // Apply modifiers.
    const auto mend = this->m_modifiers.end() - 1;
    for(auto mit = this->m_modifiers.begin(); mit != mend; ++mit) {
      const auto ptr = mit->apply_mutable_opt(cur, false, nullptr);
      if(!ptr) {
        return { };
      }
      cur = std::ref(*ptr);
    }
    // Erase the element referenced by the last modifier.
    Value erased;
    mend->apply_mutable_opt(cur, false, &erased);
    return std::move(erased);
  }

Reference & Reference::materialize()
  {
    if(this->get_root().is_lvalue() != false) {
      return *this;
    }
    auto value = this->read();
    auto var = rocket::make_refcounted<Variable>(std::move(value), false);
    Reference_root::S_variable ref_c = { std::move(var) };
    this->set_root(std::move(ref_c));
    return *this;
  }
Reference & Reference::dematerialize()
  {
    if(this->get_root().is_unique() == false) {
      return *this;
    }
    auto value = this->read();
    Reference_root::S_temporary ref_c = { std::move(value) };
    this->set_root(std::move(ref_c));
    return *this;
  }

}
