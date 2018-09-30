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
            const auto ssize = static_cast<Sint64>(arr.size());
            auto rindex = alt.index;
            if(rindex < 0) {
              // Wrap negative indices.
              rindex += ssize;
            }
            if(rindex < 0) {
              ASTERIA_DEBUG_LOG("Array index fell before the front: index = ", alt.index, ", parent = ", parent);
              return nullptr;
            }
            if(rindex >= ssize) {
              ASTERIA_DEBUG_LOG("Array index fell after the back: index = ", alt.index, ", parent = ", parent);
              return nullptr;
            }
            auto rit = arr.begin() + static_cast<Diff>(rindex);
            return &(rit[0]);
          }
//          case Value::type_opaque: {
//            const auto &opq = parent.check<D_opaque>();
//            if(!opq) {
//              ASTERIA_DEBUG_LOG("Null opaque pointer encountered.");
//              return nullptr;
//            }
//            const auto qmem = opq->open_member(alt.index, false);
//            return qmem;
//          }
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
          case Value::type_opaque: {
            const auto &opq = parent.check<D_opaque>();
            if(!opq) {
              ASTERIA_DEBUG_LOG("Null opaque pointer encountered.");
              return nullptr;
            }
            const auto qmem = opq->open_member(alt.key, false);
            return qmem;
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
            const auto ssize = static_cast<Sint64>(arr.size());
            auto rindex = alt.index;
            if(rindex < 0) {
              // Wrap negative indices.
              rindex += ssize;
            }
            D_array::iterator rit;
            if(rindex < 0) {
              if(!create_new) {
                ASTERIA_DEBUG_LOG("Array index fell before the front: index = ", alt.index, ", parent = ", parent);
                return nullptr;
              }
              const auto size_add = 0 - static_cast<Uint64>(rindex);
              if(size_add >= arr.max_size() - arr.size()) {
                ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", arr.size(), "` by `", size_add, "` would exceed system resource limits.");
              }
              rit = arr.insert(arr.begin(), static_cast<Size>(size_add));
              goto ma;
            }
            if(rindex >= ssize) {
              if(!create_new) {
                ASTERIA_DEBUG_LOG("Array index fell after the back: index = ", alt.index, ", parent = ", parent);
                return nullptr;
              }
              const auto size_add = static_cast<Uint64>(rindex) + 1 - arr.size();
              if(size_add >= arr.max_size() - arr.size()) {
                ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", arr.size(), "` by `", size_add, "` would exceed system resource limits.");
              }
              rit = arr.insert(arr.end(), static_cast<Size>(size_add)) + static_cast<Diff>(size_add - 1);
              goto ma;
            }
            rit = arr.mut_begin() + static_cast<Diff>(rindex);
  ma:
            if(erased_out_opt) {
              *erased_out_opt = std::move(rit[0]);
              arr.erase(rit);
              return erased_out_opt;
            }
            return &(rit[0]);
          }
//          case Value::type_opaque: {
//            const auto &opq = parent.check<D_opaque>();
//            if(!opq) {
//              if(!create_new) {
//                ASTERIA_DEBUG_LOG("Null opaque pointer encountered.");
//                return nullptr;
//              }
//              ASTERIA_THROW_RUNTIME_ERROR("An attempt to create a member through a null opaque pointer was made.");
//            }
//            if(erased_out_opt) {
//              *erased_out_opt = opq->unset_member(alt.index);
//              return erased_out_opt;
//            }
//            const auto qmem = opq->open_member(alt.index, create_new);
//            if(create_new && !qmem) {
//              ASTERIA_THROW_RUNTIME_ERROR("`open_member()` (of `", typeid(*opq).name(), "`) returned a null pointer when `create_new` was `true`.");
//            }
//            return qmem;
//          }
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
            D_object::iterator rit;
            if(!create_new) {
              rit = obj.find_mut(alt.key);
              if(rit == obj.end()) {
                ASTERIA_DEBUG_LOG("Object key was not found: key = ", alt.key, ", parent = ", parent);
                return nullptr;
              }
              goto mo;
            }
            rit = obj.try_emplace(alt.key).first;
  mo:
            if(erased_out_opt) {
              *erased_out_opt = std::move(rit->second);
              obj.erase(rit);
              return erased_out_opt;
            }
            return &(rit->second);
          }
          case Value::type_opaque: {
            const auto &opq = parent.check<D_opaque>();
            if(!opq) {
              if(!create_new) {
                ASTERIA_DEBUG_LOG("Null opaque pointer encountered.");
                return nullptr;
              }
              ASTERIA_THROW_RUNTIME_ERROR("An attempt to create a member through a null opaque pointer was made.");
            }
            if(erased_out_opt) {
              *erased_out_opt = opq->unset_member(alt.key);
              return erased_out_opt;
            }
            const auto qmem = opq->open_member(alt.key, create_new);
            if(create_new && !qmem) {
              ASTERIA_THROW_RUNTIME_ERROR("`open_member()` (of `", typeid(*opq).name(), "`) returned a null pointer when `create_new` was `true`.");
            }
            return qmem;
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
