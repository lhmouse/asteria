// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference_modifier.hpp"
#include "value.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference_modifier::Reference_modifier(const Reference_modifier &) noexcept
  = default;
Reference_modifier & Reference_modifier::operator=(const Reference_modifier &) noexcept
  = default;
Reference_modifier::Reference_modifier(Reference_modifier &&) noexcept
  = default;
Reference_modifier & Reference_modifier::operator=(Reference_modifier &&) noexcept
  = default;
Reference_modifier::~Reference_modifier()
  = default;

const Value * apply_reference_modifier_readonly_partial_opt(const Reference_modifier &modifier, const Value &parent)
  {
    switch(modifier.index()) {
    case Reference_modifier::index_array_index:
      {
        const auto &cand = modifier.as<Reference_modifier::S_array_index>();
        if(parent.type() == Value::type_null) {
          return nullptr;
        }
        if(parent.type() != Value::type_array) {
          ASTERIA_THROW_RUNTIME_ERROR("Index `", cand.index, "` cannot be applied to `", parent, "` because it is not an array.");
        }
        const auto &array = parent.as<D_array>();
        auto rindex = cand.index;
        if(rindex < 0) {
          // Wrap negative indices.
          rindex += static_cast<Signed>(array.size());
        }
        if(rindex < 0) {
          ASTERIA_DEBUG_LOG("Array index fell before the front: index = ", cand.index, ", array = ", array);
          return nullptr;
        }
        if(rindex >= static_cast<Signed>(array.size())) {
          ASTERIA_DEBUG_LOG("Array index fell after the back: index = ", cand.index, ", array = ", array);
          return nullptr;
        }
        const auto rit = array.begin() + static_cast<std::ptrdiff_t>(rindex);
        return &(rit[0]);
      }
    case Reference_modifier::index_object_key:
      {
        const auto &cand = modifier.as<Reference_modifier::S_object_key>();
        if(parent.type() == Value::type_null) {
          return nullptr;
        }
        if(parent.type() != Value::type_object) {
          ASTERIA_THROW_RUNTIME_ERROR("Key `", cand.key, "` cannot be applied to `", parent, "` because it is not an object.");
        }
        const auto &object = parent.as<D_object>();
        const auto rit = object.find(cand.key);
        if(rit == object.end()) {
          ASTERIA_DEBUG_LOG("Object key was not found: key = ", cand.key, ", object = ", object);
          return nullptr;
        }
        return &(rit->second);
      }
    default:
      ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", modifier.index(), "` has been encountered.");
    }
  }
Value * apply_reference_modifier_mutable_partial_opt(const Reference_modifier &modifier, Value &parent, bool creates, Value *erased_out_opt)
  {
    switch(modifier.index()) {
    case Reference_modifier::index_array_index:
      {
        const auto &cand = modifier.as<Reference_modifier::S_array_index>();
        if(parent.type() == Value::type_null) {
          if(!creates) {
            return nullptr;
          }
          parent.set(D_array());
        }
        if(parent.type() != Value::type_array) {
          ASTERIA_THROW_RUNTIME_ERROR("Index `", cand.index, "` cannot be applied to `", parent, "` because it is not an array.");
        }
        auto &array = parent.as<D_array>();
        auto rindex = cand.index;
        if(rindex < 0) {
          // Wrap negative indices.
          rindex += static_cast<Signed>(array.size());
        }
        if(rindex < 0) {
          ASTERIA_DEBUG_LOG("Array index fell before the front: index = ", cand.index, ", array = ", array);
          if(!creates) {
            return nullptr;
          }
          const auto size_add = static_cast<Unsigned>(0) - static_cast<Unsigned>(rindex);
          if(size_add >= array.max_size() - array.size()) {
            ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", array.size(), "` by `", size_add, "` would exceed system resource limits.");
          }
          array.insert(array.begin(), static_cast<std::size_t>(size_add));
          rindex = 0;
        }
        if(rindex >= static_cast<Signed>(array.size())) {
          ASTERIA_DEBUG_LOG("Array index fell after the back: index = ", cand.index, ", array = ", array);
          if(!creates) {
            return nullptr;
          }
          const auto size_add = static_cast<Unsigned>(rindex) + 1 - array.size();
          if(size_add >= array.max_size() - array.size()) {
            ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", array.size(), "` by `", size_add, "` would exceed system resource limits.");
          }
          array.insert(array.end(), static_cast<std::size_t>(size_add));
        }
        const auto rit = array.mut_begin() + static_cast<std::ptrdiff_t>(rindex);
        if(erased_out_opt){
          *erased_out_opt = std::move(rit[0]);
          array.erase(rit);
          return nullptr;
        }
        return &(rit[0]);
      }
    case Reference_modifier::index_object_key:
      {
        const auto &cand = modifier.as<Reference_modifier::S_object_key>();
        if(parent.type() == Value::type_null) {
          if(!creates) {
            return nullptr;
          }
          parent.set(D_object());
        }
        if(parent.type() != Value::type_object) {
          ASTERIA_THROW_RUNTIME_ERROR("Key `", cand.key, "` cannot be applied to `", parent, "` because it is not an object.");
        }
        auto &object = parent.as<D_object>();
        auto rit = D_object::iterator();
        if(!creates) {
          rit = object.find_mut(cand.key);
          if(rit == object.end()) {
            return nullptr;
          }
        } else {
          rit = object.try_emplace(cand.key).first;
        }
        if(erased_out_opt){
          *erased_out_opt = std::move(rit->second);
          object.erase(rit);
          return nullptr;
        }
        return &(rit->second);
      }
    default:
      ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", modifier.index(), "` has been encountered.");
    }
  }

}
