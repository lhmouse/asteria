// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_modifier.hpp"
#include "value.hpp"
#include "../utilities.hpp"

namespace Asteria {

Reference_Modifier::~Reference_Modifier()
  {
  }

const Value * Reference_Modifier::apply_const_opt(const Value &parent) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_array_index:
      {
        const auto &alt = this->m_stor.as<S_array_index>();
        switch(rocket::weaken_enum(parent.type())) {
        case type_null:
          {
            return nullptr;
          }
        case type_array:
          {
            const auto &arr = parent.check<D_array>();
            auto wrap = wrap_index(alt.index, arr.size());
            if(wrap.index >= arr.size()) {
              ASTERIA_DEBUG_LOG("Array index is out of range: index = ", alt.index, ", size = ", arr.size());
              return nullptr;
            }
            return &(arr.at(static_cast<std::size_t>(wrap.index)));
          }
        default:
          ASTERIA_THROW_RUNTIME_ERROR("Index `", alt.index, "` cannot be applied to `", parent, "`.");
        }
      }
    case index_object_key:
      {
        const auto &alt = this->m_stor.as<S_object_key>();
        switch(rocket::weaken_enum(parent.type())) {
        case type_null:
          {
            return nullptr;
          }
        case type_object:
          {
            const auto &obj = parent.check<D_object>();
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

Value * Reference_Modifier::apply_mutable_opt(Value &parent, bool create_new) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_array_index:
      {
        const auto &alt = this->m_stor.as<S_array_index>();
        switch(rocket::weaken_enum(parent.type())) {
        case type_null:
          {
            if(!create_new) {
              return nullptr;
            }
            parent = D_array();
            // Fallthrough.
        case type_array:
            auto &arr = parent.check<D_array>();
            auto wrap = wrap_index(alt.index, arr.size());
            if(wrap.index >= arr.size()) {
              if(!create_new) {
                ASTERIA_DEBUG_LOG("Array index is out of range: index = ", alt.index, ", size = ", arr.size());
                return nullptr;
              }
              const auto size_add = wrap.front_fill + wrap.back_fill;
              if(size_add >= arr.max_size() - arr.size()) {
                ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", arr.size(), "` by `", size_add, "` would exceed system resource limits.");
              }
              arr.insert(arr.begin(), static_cast<std::size_t>(wrap.front_fill));
              wrap.index += static_cast<std::size_t>(wrap.front_fill);
              arr.insert(arr.end(), static_cast<std::size_t>(wrap.back_fill));
            }
            return &(arr.mut(static_cast<std::size_t>(wrap.index)));
          }
        default:
          ASTERIA_THROW_RUNTIME_ERROR("Index `", alt.index, "` cannot be applied to `", parent, "`.");
        }
      }
    case index_object_key:
      {
        const auto &alt = this->m_stor.as<S_object_key>();
        switch(rocket::weaken_enum(parent.type())) {
        case type_null:
          {
            if(!create_new) {
              return nullptr;
            }
            parent = D_object();
            // Fallthrough.
        case type_object:
            auto &obj = parent.check<D_object>();
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

Value Reference_Modifier::apply_and_erase(Value &parent) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_array_index:
      {
        const auto &alt = this->m_stor.as<S_array_index>();
        switch(rocket::weaken_enum(parent.type())) {
        case type_null:
          {
            return D_null();
          }
        case type_array:
          {
            auto &arr = parent.check<D_array>();
            auto wrap = wrap_index(alt.index, arr.size());
            if(wrap.index >= arr.size()) {
              ASTERIA_DEBUG_LOG("Array index is out of range: index = ", alt.index, ", size = ", arr.size());
              return D_null();
            }
            auto erased = std::move(arr.mut(static_cast<std::size_t>(wrap.index)));
            arr.erase(arr.begin() + static_cast<std::ptrdiff_t>(wrap.index));
            return erased;
          }
        default:
          ASTERIA_THROW_RUNTIME_ERROR("Index `", alt.index, "` cannot be applied to `", parent, "`.");
        }
      }
    case index_object_key:
      {
        const auto &alt = this->m_stor.as<S_object_key>();
        switch(rocket::weaken_enum(parent.type())) {
        case type_null:
          {
            return D_null();
          }
        case type_object:
          {
            auto &obj = parent.check<D_object>();
            auto rit = obj.find_mut(alt.key);
            if(rit == obj.end()) {
              ASTERIA_DEBUG_LOG("Object key was not found: key = ", alt.key, ", parent = ", parent);
              return D_null();
            }
            auto erased = std::move(rit->second);
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

}
