// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference_modifier.hpp"
#include "value.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference_modifier::~Reference_modifier()
  {
  }

const Value * Reference_modifier::apply_readonly_opt(const Value &parent) const
  {
    switch(Index(this->m_stor.index())) {
      case index_array_index: {
        const auto &alt = this->m_stor.as<S_array_index>();
        switch(rocket::weaken_enum(parent.type())) {
          case Value::type_null: {
            return nullptr;
          }
          case Value::type_array: {
            const auto &arr = parent.check<D_array>();
            std::uint64_t bfill, efill;
            auto rindex = wrap_index(bfill, efill, alt.index, arr.size());
            if(rindex >= arr.size()) {
              ASTERIA_DEBUG_LOG("Array index is out of range: index = ", alt.index, ", size = ", arr.size());
              return nullptr;
            }
            return &(arr.at(static_cast<std::size_t>(rindex)));
          }
          default: {
            ASTERIA_THROW_RUNTIME_ERROR("Index `", alt.index, "` cannot be applied to `", parent, "`.");
          }
        }
      }
      case index_object_key: {
        const auto &alt = this->m_stor.as<S_object_key>();
        switch(rocket::weaken_enum(parent.type())) {
          case Value::type_null: {
            return nullptr;
          }
          case Value::type_object: {
            const auto &obj = parent.check<D_object>();
            auto rit = obj.find(alt.key);
            if(rit == obj.end()) {
              ASTERIA_DEBUG_LOG("Object key was not found: key = ", alt.key, ", parent = ", parent);
              return nullptr;
            }
            return &(rit->second);
          }
          default: {
            ASTERIA_THROW_RUNTIME_ERROR("Key `", alt.key, "` cannot be applied to `", parent, "`.");
          }
        }
      }
      default: {
        ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

Value * Reference_modifier::apply_mutable_opt(Value &parent, bool create_new, Value *erased_out_opt) const
  {
    switch(Index(this->m_stor.index())) {
      case index_array_index: {
        const auto &alt = this->m_stor.as<S_array_index>();
        switch(rocket::weaken_enum(parent.type())) {
          case Value::type_null: {
            if(!create_new) {
              return nullptr;
            }
            parent = D_array();
            // Fallthrough.
          case Value::type_array:
            auto &arr = parent.check<D_array>();
            std::uint64_t bfill, efill;
            auto rindex = wrap_index(bfill, efill, alt.index, arr.size());
            if(rindex >= arr.size()) {
              if(!create_new) {
                ASTERIA_DEBUG_LOG("Array index is out of range: index = ", alt.index, ", size = ", arr.size());
                return nullptr;
              }
              auto rsize_add = bfill | efill;
              if(rsize_add >= arr.max_size() - arr.size()) {
                ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", arr.size(), "` by `", rsize_add, "` would exceed system resource limits.");
              }
              if(bfill != 0) {
                arr.insert(arr.begin(), static_cast<std::size_t>(bfill));
                rindex += bfill;
              }
              if(efill != 0) {
                arr.append(static_cast<std::size_t>(efill));
              }
            }
            if(erased_out_opt) {
              *erased_out_opt = std::move(arr.mut(static_cast<std::size_t>(rindex)));
              arr.erase(arr.begin() + static_cast<std::ptrdiff_t>(rindex));
              return erased_out_opt;
            }
            return &(arr.mut(static_cast<std::size_t>(rindex)));
          }
          default: {
            ASTERIA_THROW_RUNTIME_ERROR("Index `", alt.index, "` cannot be applied to `", parent, "`.");
          }
        }
      }
      case index_object_key: {
        const auto &alt = this->m_stor.as<S_object_key>();
        switch(rocket::weaken_enum(parent.type())) {
          case Value::type_null: {
            if(!create_new) {
              return nullptr;
            }
            parent = D_object();
            // Fallthrough.
          case Value::type_object:
            auto &obj = parent.check<D_object>();
            auto rit = create_new ? obj.try_emplace(alt.key).first : obj.find_mut(alt.key);
            if(rit == obj.end()) {
              ASTERIA_DEBUG_LOG("Object key was not found: key = ", alt.key, ", parent = ", parent);
              return nullptr;
            }
            if(erased_out_opt) {
              *erased_out_opt = std::move(rit->second);
              obj.erase(rit);
              return erased_out_opt;
            }
            return &(rit->second);
          }
          default: {
            ASTERIA_THROW_RUNTIME_ERROR("Key `", alt.key, "` cannot be applied to `", parent, "`.");
          }
        }
      }
      default: {
        ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

}
