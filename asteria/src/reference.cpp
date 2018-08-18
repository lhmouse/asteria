// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "value.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria
{

Reference::Reference(Reference &&) noexcept = default;
Reference & Reference::operator=(Reference &&) noexcept = default;
Reference::~Reference() = default;

Value read_reference(const Reference &ref)
  {
    const Value *ptr;
    // Get a pointer to the root value.
    switch(ref.get_root().which()) {
    case Reference_root::type_constant:
      {
        const auto &cand = ref.get_root().as<Reference_root::S_constant>();
        ptr = &(cand.src);
        break;
      }
    case Reference_root::type_temporary_value:
      {
        const auto &cand = ref.get_root().as<Reference_root::S_temporary_value>();
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
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", ref.get_root().which(), "` is encountered.");
    }
    // Apply modifiers.
    for(const auto &modifier : ref.get_modifiers()) {
      switch(modifier.which()) {
      case Reference_modifier::type_array_index:
        {
          const auto &cand = modifier.as<Reference_modifier::S_array_index>();
          if(ptr->which() == Value::type_null) {
            return D_null();
          }
          if(ptr->which() != Value::type_array) {
            ASTERIA_THROW_RUNTIME_ERROR("Cannot read from index `", cand.index, "` of `", *ptr, "` which is not an array.");
          }
          const auto &array = ptr->as<D_array>();
          auto rindex = cand.index;
          if(rindex < 0) {
            // Wrap negative indices.
            rindex += static_cast<Signed>(array.size());
          }
          if(rindex < 0) {
            ASTERIA_DEBUG_LOG("Array index fell before the front: index = ", cand.index, ", array = ", array);
            return D_null();
          }
          if(rindex >= static_cast<Signed>(array.size())) {
            ASTERIA_DEBUG_LOG("Array index fell after the back: index = ", cand.index, ", array = ", array);
            return D_null();
          }
          ptr = std::addressof(array.at(static_cast<std::size_t>(rindex)));
          break;
        }
      case Reference_modifier::type_object_key:
        {
          const auto &cand = modifier.as<Reference_modifier::S_object_key>();
          if(ptr->which() == Value::type_null) {
            return D_null();
          }
          if(ptr->which() != Value::type_object) {
            ASTERIA_THROW_RUNTIME_ERROR("Cannot read from key `", cand.key, "` of `", *ptr, "` which is not an object.");
          }
          const auto &object = ptr->as<D_object>();
          auto rit = object.find(cand.key);
          if(rit == object.end()) {
            ASTERIA_DEBUG_LOG("Object key was not found: key = ", cand.key, ", object = ", object);
            return D_null();
          }
          ptr = std::addressof(rit->second);
          break;
        }
      default:
        ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", modifier.which(), "` is encountered.");
      }
    }
    return *ptr;
  }
void write_reference(const Reference &ref, Value &&value)
  {
    Value *ptr;
    // Get a pointer to the root value.
    switch(ref.get_root().which()) {
    case Reference_root::type_constant:
      {
        const auto &cand = ref.get_root().as<Reference_root::S_constant>();
        ASTERIA_THROW_RUNTIME_ERROR("The constant `", cand.src, "` cannot be modified.");
      }
    case Reference_root::type_temporary_value:
      {
        const auto &cand = ref.get_root().as<Reference_root::S_temporary_value>();
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
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", ref.get_root().which(), "` is encountered.");
    }
    // Apply modifiers.
    for(const auto &modifier : ref.get_modifiers()) {
      switch(modifier.which()) {
      case Reference_modifier::type_array_index:
        {
          const auto &cand = modifier.as<Reference_modifier::S_array_index>();
          if(ptr->which() == Value::type_null) {
            ptr->set(D_array());
          }
          if(ptr->which() != Value::type_array) {
            ASTERIA_THROW_RUNTIME_ERROR("Cannot write to index `", cand.index, "` of `", *ptr, "` which is not an array.");
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
            array.insert(array.begin(), static_cast<std::size_t>(size_add), D_null());
            rindex = 0;
            goto resized;
          }
          if(rindex >= static_cast<Signed>(array.size())) {
            ASTERIA_DEBUG_LOG("Array index fell after the back: index = ", cand.index, ", array = ", array);
            const auto size_add = static_cast<Unsigned>(rindex) + 1 - array.size();
            if(size_add >= array.max_size() - array.size()) {
              ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", array.size(), "` by `", size_add, "` would exceed system resource limits.");
            }
            ASTERIA_DEBUG_LOG("Appending `null` elements to the array: size = ", array.size(), ", size_add = ", size_add);
            array.insert(array.end(), static_cast<std::size_t>(size_add), D_null());
            goto resized;
          }
        resized:
          ptr = std::addressof(array.mut(static_cast<std::size_t>(rindex)));
          break;
        }
      case Reference_modifier::type_object_key:
        {
          const auto &cand = modifier.as<Reference_modifier::S_object_key>();
          if(ptr->which() == Value::type_null) {
            ptr->set(D_object());
          }
          if(ptr->which() != Value::type_object) {
            ASTERIA_THROW_RUNTIME_ERROR("Cannot write to key `", cand.key, "` of `", *ptr, "` which is not an object.");
          }
          auto &object = ptr->as<D_object>();
          auto rpair = object.try_emplace(cand.key, D_null());
          if(rpair.second == false) {
            ASTERIA_DEBUG_LOG("Key inserted: key = ", cand.key, ", object = ", object);
          }
          ptr = std::addressof(rpair.first->second);
          break;
        }
      default:
        ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", modifier.which(), "` is encountered.");
      }
    }
    *ptr = std::move(value);
  }

void materialize_reference(Reference &ref)
  {
    if(ref.get_root().which() == Reference_root::type_variable) {
      return;
    }
    // Create a variable.
    auto value = read_reference(ref);
    auto var = std::make_shared<Variable>(read_reference(ref), false);
    // Make `ref` a reference to this variable.
    Reference_root::S_variable ref_v = { std::move(var) };
    ref.set_root(std::move(ref_v));
    ref.clear_modifiers();
  }

}
