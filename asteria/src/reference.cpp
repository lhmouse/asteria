// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference::Reference(const Reference &) noexcept
  = default;
Reference & Reference::operator=(const Reference &) noexcept
  = default;
Reference::Reference(Reference &&) noexcept
  = default;
Reference & Reference::operator=(Reference &&) noexcept
  = default;
Reference::~Reference()
  = default;

Value read_reference(const Reference &ref)
  {
    const auto nmod = ref.get_modifier_count();
    // Dereference the root.
    auto cur = std::ref(dereference_root_readonly_partial(ref.get_root()));
    // Apply modifiers.
    for(std::size_t i = 0; i < nmod; ++i) {
      const auto ptr = apply_reference_modifier_readonly_partial_opt(ref.get_modifier(i), cur);
      if(!ptr) {
        return { };
      }
      cur = std::ref(*ptr);
    }
    // Return a reference to the current value.
    return cur;
  }

Value & write_reference(const Reference &ref, Value value)
  {
    const auto nmod = ref.get_modifier_count();
    // Dereference the root.
    auto cur = std::ref(dereference_root_mutable_partial(ref.get_root()));
    // Apply modifiers.
    for(std::size_t i = 0; i < nmod; ++i) {
      const auto ptr = apply_reference_modifier_mutable_partial_opt(ref.get_modifier(i), cur, true, nullptr);
      if(!ptr) {
        ROCKET_ASSERT(false);
      }
      cur = std::ref(*ptr);
    }
    // Set the new value.
    cur.get() = std::move(value);
    return cur;
  }

Value unset_reference(const Reference &ref)
  {
    const auto nmod = ref.get_modifier_count();
    if(nmod == 0) {
      ASTERIA_THROW_RUNTIME_ERROR("Only array elements or object members may be `unset`.");
    }
    // Dereference the root.
    auto cur = std::ref(dereference_root_mutable_partial(ref.get_root()));
    // Apply modifiers except the last one.
    for(std::size_t i = 0; i < nmod - 1; ++i) {
      const auto ptr = apply_reference_modifier_mutable_partial_opt(ref.get_modifier(i), cur, false, nullptr);
      if(!ptr) {
        return { };
      }
      cur = std::ref(*ptr);
    }
    // Erase the element referenced by the last modifier.
    Value erased;
    apply_reference_modifier_mutable_partial_opt(ref.get_modifier(nmod - 1), cur, false, &erased);
    return std::move(erased);
  }

Reference reference_constant(Value value)
  {
    Reference_root::S_constant ref_c = { std::move(value) };
    return Reference_root(std::move(ref_c));
  }

Reference reference_temp_value(Value value)
  {
    Reference_root::S_temp_value ref_c = { std::move(value) };
    return Reference_root(std::move(ref_c));
  }

Reference & materialize_reference(Reference &ref)
  {
    if(ref.get_root().index() == Reference_root::index_variable) {
      return ref;
    }
    auto value = read_reference(ref);
    auto var = rocket::make_refcounted<Variable>(std::move(value), false);
    Reference_root::S_variable ref_c = { std::move(var) };
    ref.set_root(std::move(ref_c));
    return ref;
  }

Reference & dematerialize_reference(Reference &ref)
  {
    if(ref.get_root().index() != Reference_root::index_variable) {
      return ref;
    }
    const auto &alt = ref.get_root().as<Reference_root::S_variable>();
    if(alt.var.unique() == false) {
      return ref;
    }
    auto value = read_reference(ref);
    Reference_root::S_temp_value ref_c = { std::move(value) };
    ref.set_root(std::move(ref_c));
    return ref;
  }

}
