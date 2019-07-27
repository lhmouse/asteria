// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_modifier.hpp"
#include "value.hpp"
#include "../utilities.hpp"

namespace Asteria {

const Value* Reference_Modifier::apply_const_opt(const Value& parent) const
  {
    switch(this->index()) {
    case index_array_index:
      {
        const auto& altr = this->m_stor.as<index_array_index>();
        if(parent.is_null()) {
          // Elements of a `null` are also `null`s.
          return nullptr;
        }
        if(!parent.is_array()) {
          ASTERIA_THROW_RUNTIME_ERROR("`", parent, "` is not an `array` and cannot be indexed by `integer`s.");
        }
        const auto& array = parent.as_array();
        auto w = wrap_index(altr.index, array.size());
        auto nadd = w.nprepend | w.nappend;
        if(nadd != 0) {
          ASTERIA_DEBUG_LOG("Array subscript was out of range: index = ", altr.index, ", size = ", array.size());
          return nullptr;
        }
        return std::addressof(array.at(w.rindex));
      }
    case index_object_key:
      {
        const auto& altr = this->m_stor.as<index_object_key>();
        if(parent.is_null()) {
          // Members of a `null` are also `null`s.
          return nullptr;
        }
        if(!parent.is_object()) {
          ASTERIA_THROW_RUNTIME_ERROR("`", parent, "` is not an `object` and cannot be indexed by `string`s.");
        }
        const auto& object = parent.as_object();
        auto rit = object.find(altr.key);
        if(rit == object.end()) {
          ASTERIA_DEBUG_LOG("Object member was not found: key = ", altr.key, ", parent = ", parent);
          return nullptr;
        }
        return std::addressof(rit->second);
      }
    default:
      ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

Value* Reference_Modifier::apply_mutable_opt(Value& parent, bool create_new) const
  {
    switch(this->index()) {
    case index_array_index:
      {
        const auto& altr = this->m_stor.as<index_array_index>();
        if(parent.is_null()) {
          // Create elements as needed.
          if(!create_new) {
            return nullptr;
          }
          parent = G_array();
        }
        if(!parent.is_array()) {
          ASTERIA_THROW_RUNTIME_ERROR("`", parent, "` is not an `array` and cannot be indexed by `integer`s.");
        }
        auto& array = parent.mut_array();
        auto w = wrap_index(altr.index, array.size());
        auto nadd = w.nprepend | w.nappend;
        if(nadd != 0) {
          if(!create_new) {
            ASTERIA_DEBUG_LOG("Array subscript was out of range: index = ", altr.index, ", size = ", array.size());
            return nullptr;
          }
          if(nadd > array.max_size() - array.size()) {
            ASTERIA_THROW_RUNTIME_ERROR("Extending an `array` of length `", array.size(), "` by `", nadd, "` would exceed the system limit.");
          }
          array.insert(array.begin(), static_cast<std::size_t>(w.nprepend));
          array.insert(array.end(), static_cast<std::size_t>(w.nappend));
        }
        return std::addressof(array.mut(w.rindex));
      }
    case index_object_key:
      {
        const auto& altr = this->m_stor.as<index_object_key>();
        if(parent.is_null()) {
          // Create members as needed.
          if(!create_new) {
            return nullptr;
          }
          parent = G_object();
        }
        if(!parent.is_object()) {
          ASTERIA_THROW_RUNTIME_ERROR("`", parent, "` is not an `object` and cannot be indexed by `string`s.");
        }
        auto& object = parent.mut_object();
        G_object::iterator rit;
        if(!create_new) {
          rit = object.find_mut(altr.key);
          if(rit == object.end()) {
            ASTERIA_DEBUG_LOG("Object member was not found: key = ", altr.key, ", parent = ", parent);
            return nullptr;
          }
        }
        else {
          rit = object.try_emplace(altr.key).first;
        }
        return std::addressof(rit->second);
      }
    default:
      ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

Value Reference_Modifier::apply_and_erase(Value& parent) const
  {
    switch(this->index()) {
    case index_array_index:
      {
        const auto& altr = this->m_stor.as<index_array_index>();
        if(parent.is_null()) {
          // There is nothing to erase.
          return G_null();
        }
        if(!parent.is_array()) {
          ASTERIA_THROW_RUNTIME_ERROR("`", parent, "` is not an `array` and cannot be indexed by `integer`s.");
        }
        auto& array = parent.mut_array();
        auto w = wrap_index(altr.index, array.size());
        auto nadd = w.nprepend | w.nappend;
        if(nadd != 0) {
          ASTERIA_DEBUG_LOG("Array subscript was out of range: index = ", altr.index, ", size = ", array.size());
          return G_null();
        }
        auto elem = rocket::move(array.mut(w.rindex));
        array.erase(w.rindex, 1);
        return elem;
      }
    case index_object_key:
      {
        const auto& altr = this->m_stor.as<index_object_key>();
        if(parent.is_null()) {
          // There is nothing to erase.
          return G_null();
        }
        if(!parent.is_object()) {
          ASTERIA_THROW_RUNTIME_ERROR("`", parent, "` is not an `object` and cannot be indexed by `string`s.");
        }
        auto& object = parent.mut_object();
        auto rit = object.find_mut(altr.key);
        if(rit == object.end()) {
          ASTERIA_DEBUG_LOG("Object member was not found: key = ", altr.key, ", parent = ", parent);
          return nullptr;
        }
        auto elem = rocket::move(rit->second);
        object.erase(rit);
        return elem;
      }
    default:
      ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
