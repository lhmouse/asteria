// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_modifier.hpp"
#include "value.hpp"
#include "../utilities.hpp"

namespace Asteria {

const Value* Reference_Modifier::apply_const_opt(const Value& parent) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_array_index:
      {
        const auto& alt = this->m_stor.as<index_array_index>();
        switch(rocket::weaken_enum(parent.dtype())) {
        case gtype_null:
          {
            return nullptr;
          }
        case gtype_array:
          {
            const auto& arr = parent.check<G_array>();
            auto w = wrap_index(alt.index, arr.size());
            auto nadd = w.nprepend | w.nappend;
            if(nadd != 0) {
              ASTERIA_DEBUG_LOG("Array subscript is out of range: index = ", alt.index, ", size = ", arr.size());
              return nullptr;
            }
            return &(arr.at(w.rindex));
          }
        default:
          ASTERIA_THROW_RUNTIME_ERROR("Index `", alt.index, "` cannot be applied to `", parent, "`.");
        }
      }
    case index_object_key:
      {
        const auto& alt = this->m_stor.as<index_object_key>();
        switch(rocket::weaken_enum(parent.dtype())) {
        case gtype_null:
          {
            return nullptr;
          }
        case gtype_object:
          {
            const auto& obj = parent.check<G_object>();
            auto rit = obj.find(alt.key);
            if(rit == obj.end()) {
              ASTERIA_DEBUG_LOG("Object key was not found: key = ", alt.key, ", parent = ", parent);
              return nullptr;
            }
            return &(rit->second);
          }
        default:
          ASTERIA_THROW_RUNTIME_ERROR("Key `", alt.key, "` cannot be applied to `", parent, "`.");
        }
      }
    default:
      ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

Value* Reference_Modifier::apply_mutable_opt(Value& parent, bool create_new) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_array_index:
      {
        const auto& alt = this->m_stor.as<index_array_index>();
        switch(rocket::weaken_enum(parent.dtype())) {
        case gtype_null:
          {
            if(!create_new) {
              return nullptr;
            }
            parent = G_array();
          }
          // Fallthrough.
        case gtype_array:
          {
            auto& arr = parent.check<G_array>();
            auto w = wrap_index(alt.index, arr.size());
            auto nadd = w.nprepend | w.nappend;
            if(nadd != 0) {
              if(!create_new) {
                ASTERIA_DEBUG_LOG("Array subscript is out of range: index = ", alt.index, ", size = ", arr.size());
                return nullptr;
              }
              if(nadd > arr.max_size() - arr.size()) {
                ASTERIA_THROW_RUNTIME_ERROR("Extending an `array` of length `", arr.size(), "` by `", nadd, "` would exceed the system limit.");
              }
              arr.insert(arr.begin(), static_cast<std::size_t>(w.nprepend));
              arr.insert(arr.end(), static_cast<std::size_t>(w.nappend));
            }
            return &(arr.mut(w.rindex));
          }
        default:
          ASTERIA_THROW_RUNTIME_ERROR("Index `", alt.index, "` cannot be applied to `", parent, "`.");
        }
      }
    case index_object_key:
      {
        const auto& alt = this->m_stor.as<index_object_key>();
        switch(rocket::weaken_enum(parent.dtype())) {
        case gtype_null:
          {
            if(!create_new) {
              return nullptr;
            }
            parent = G_object();
          }
          // Fallthrough.
        case gtype_object:
          {
            auto& obj = parent.check<G_object>();
            auto rit = create_new ? obj.try_emplace(alt.key).first : obj.find_mut(alt.key);
            if(rit == obj.end()) {
              ASTERIA_DEBUG_LOG("Object key was not found: key = ", alt.key, ", parent = ", parent);
              return nullptr;
            }
            return &(rit->second);
          }
        default:
          ASTERIA_THROW_RUNTIME_ERROR("Key `", alt.key, "` cannot be applied to `", parent, "`.");
        }
      }
    default:
      ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

Value Reference_Modifier::apply_and_erase(Value& parent) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_array_index:
      {
        const auto& alt = this->m_stor.as<index_array_index>();
        switch(rocket::weaken_enum(parent.dtype())) {
        case gtype_null:
          {
            return G_null();
          }
        case gtype_array:
          {
            auto& arr = parent.check<G_array>();
            auto w = wrap_index(alt.index, arr.size());
            auto nadd = w.nprepend | w.nappend;
            if(nadd != 0) {
              ASTERIA_DEBUG_LOG("Array subscript is out of range: index = ", alt.index, ", size = ", arr.size());
              return G_null();
            }
            auto erased = rocket::move(arr.mut(w.rindex));
            arr.erase(w.rindex, 1);
            return rocket::move(erased);
          }
        default:
          ASTERIA_THROW_RUNTIME_ERROR("Index `", alt.index, "` cannot be applied to `", parent, "`.");
        }
      }
    case index_object_key:
      {
        const auto& alt = this->m_stor.as<index_object_key>();
        switch(rocket::weaken_enum(parent.dtype())) {
        case gtype_null:
          {
            return G_null();
          }
        case gtype_object:
          {
            auto& obj = parent.check<G_object>();
            auto rit = obj.find_mut(alt.key);
            if(rit == obj.end()) {
              ASTERIA_DEBUG_LOG("Object key was not found: key = ", alt.key, ", parent = ", parent);
              return G_null();
            }
            auto erased = rocket::move(rit->second);
            obj.erase(rit);
            return erased;
          }
        default:
          ASTERIA_THROW_RUNTIME_ERROR("Key `", alt.key, "` cannot be applied to `", parent, "`.");
        }
      }
    default:
      ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

}  // namespace Asteria
