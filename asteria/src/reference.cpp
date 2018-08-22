// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria
{

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
    const Value *ptr;
    // Get a pointer to the root value.
    switch(ref.get_root().index()) {
    case Reference_root::type_constant:
      {
        const auto &cand = ref.get_root().as<Reference_root::S_constant>();
        ptr = &(cand.src);
        break;
      }
    case Reference_root::type_temp_value:
      {
        const auto &cand = ref.get_root().as<Reference_root::S_temp_value>();
        ptr = &(cand.value);
        break;
      }
    case Reference_root::type_variable:
      {
        const auto &cand = ref.get_root().as<Reference_root::S_variable>();
        ptr = &(cand.var->get_value());
        break;
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", ref.get_root().index(), "` has been encountered.");
    }
    // Apply modifiers.
    for(auto modit = ref.get_modifiers().begin(); modit != ref.get_modifiers().end(); ++modit){
      switch(modit->index()) {
      case Reference_modifier::type_array_index:
        {
          const auto &cand = modit->as<Reference_modifier::S_array_index>();
          if(ptr->type() == Value::type_null) {
            return { };
          }
          if(ptr->type() != Value::type_array) {
            ASTERIA_THROW_RUNTIME_ERROR("Index `", cand.index, "` cannot be applied to `", *ptr, "` because it is not an array.");
          }
          const auto &array = ptr->as<D_array>();
          auto rindex = cand.index;
          if(rindex < 0) {
            // Wrap negative indices.
            rindex += static_cast<Signed>(array.size());
          }
          if(rindex < 0) {
            ASTERIA_DEBUG_LOG("Array index fell before the front: index = ", cand.index, ", array = ", array);
            return { };
          }
          else if(rindex >= static_cast<Signed>(array.size())) {
            ASTERIA_DEBUG_LOG("Array index fell after the back: index = ", cand.index, ", array = ", array);
            return { };
          }
          ptr = &(array.at(static_cast<std::size_t>(rindex)));
          break;
        }
      case Reference_modifier::type_object_key:
        {
          const auto &cand = modit->as<Reference_modifier::S_object_key>();
          if(ptr->type() == Value::type_null) {
            return { };
          }
          if(ptr->type() != Value::type_object) {
            ASTERIA_THROW_RUNTIME_ERROR("Key `", cand.key, "` cannot be applied to `", *ptr, "` because it is not an object.");
          }
          const auto &object = ptr->as<D_object>();
          auto rit = object.find(cand.key);
          if(rit == object.end()) {
            ASTERIA_DEBUG_LOG("Object key was not found: key = ", cand.key, ", object = ", object);
            return { };
          }
          ptr = &(rit->second);
          break;
        }
      default:
        ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", modit->index(), "` has been encountered.");
      }
    }
    return *ptr;
  }
void write_reference(const Reference &ref, Value value)
  {
    Value *ptr;
    // Get a pointer to the root value.
    switch(ref.get_root().index()) {
    case Reference_root::type_constant:
      {
        const auto &cand = ref.get_root().as<Reference_root::S_constant>();
        ASTERIA_THROW_RUNTIME_ERROR("The constant `", cand.src, "` cannot be modified.");
      }
    case Reference_root::type_temp_value:
      {
        const auto &cand = ref.get_root().as<Reference_root::S_temp_value>();
        ASTERIA_THROW_RUNTIME_ERROR("The temporary value `", cand.value, "` cannot be modified.");
      }
    case Reference_root::type_variable:
      {
        const auto &cand = ref.get_root().as<Reference_root::S_variable>();
        if(cand.var->is_immutable()) {
          ASTERIA_THROW_RUNTIME_ERROR("The variable having value `", cand.var->get_value(), "` is immutable and cannot be modified.");
        }
        ptr = &(cand.var->get_value());
        break;
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", ref.get_root().index(), "` has been encountered.");
    }
    // Apply modifiers.
    for(auto modit = ref.get_modifiers().begin(); modit != ref.get_modifiers().end(); ++modit){
      switch(modit->index()) {
      case Reference_modifier::type_array_index:
        {
          const auto &cand = modit->as<Reference_modifier::S_array_index>();
          if(ptr->type() == Value::type_null) {
            ptr->set(D_array());
          }
          if(ptr->type() != Value::type_array) {
            ASTERIA_THROW_RUNTIME_ERROR("Index `", cand.index, "` cannot be applied to `", *ptr, "` because it is not an array.");
          }
          auto &array = ptr->as<D_array>();
          auto rindex = cand.index;
          if(rindex < 0) {
            // Wrap negative indices.
            rindex += static_cast<Signed>(array.size());
          }
          if(rindex < 0) {
            ASTERIA_DEBUG_LOG("Array index fell before the front: index = ", cand.index, ", array = ", array);
            const auto size_add = -static_cast<Unsigned>(rindex);
            if(size_add >= array.max_size() - array.size()) {
              ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", array.size(), "` by `", size_add, "` would exceed system resource limits.");
            }
            ASTERIA_DEBUG_LOG("Prepending `null` elements to the array: size = ", array.size(), ", size_add = ", size_add);
            array.insert(array.begin(), static_cast<std::size_t>(size_add));
            rindex = 0;
          }
          else if(rindex >= static_cast<Signed>(array.size())) {
            ASTERIA_DEBUG_LOG("Array index fell after the back: index = ", cand.index, ", array = ", array);
            const auto size_add = static_cast<Unsigned>(rindex) + 1 - array.size();
            if(size_add >= array.max_size() - array.size()) {
              ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", array.size(), "` by `", size_add, "` would exceed system resource limits.");
            }
            ASTERIA_DEBUG_LOG("Appending `null` elements to the array: size = ", array.size(), ", size_add = ", size_add);
            array.insert(array.end(), static_cast<std::size_t>(size_add));
          }
          ptr = &(array.mut(static_cast<std::size_t>(rindex)));
          break;
        }
      case Reference_modifier::type_object_key:
        {
          const auto &cand = modit->as<Reference_modifier::S_object_key>();
          if(ptr->type() == Value::type_null) {
            ptr->set(D_object());
          }
          if(ptr->type() != Value::type_object) {
            ASTERIA_THROW_RUNTIME_ERROR("Key `", cand.key, "` cannot be applied to `", *ptr, "` because it is not an object.");
          }
          auto &object = ptr->as<D_object>();
          auto rpair = object.try_emplace(cand.key);
          if(rpair.second == false) {
            ASTERIA_DEBUG_LOG("Key inserted: key = ", cand.key, ", object = ", object);
          }
          ptr = &(rpair.first->second);
          break;
        }
      default:
        ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", modit->index(), "` has been encountered.");
      }
    }
    *ptr = std::move(value);
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
void materialize_reference(Reference &ref)
  {
    if(ref.get_root().index() == Reference_root::type_variable) {
      return;
    }
    // Create a variable.
    auto value = read_reference(ref);
    auto var = allocate<Variable>(std::move(value), false);
    // Make `ref` a reference to this variable.
    Reference_root::S_variable ref_v = { std::move(var) };
    ref.set_root(std::move(ref_v));
  }
Reference indirect_reference_from(const Reference &parent, Reference_modifier modifier)
  {
    auto mod = parent.get_modifiers();
    mod.emplace_back(std::move(modifier));
    return Reference(parent.get_root(), std::move(mod));
  }

}
