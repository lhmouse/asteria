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

Reference_modifier::Reference_modifier(const Reference_modifier &) noexcept
  = default;
Reference_modifier & Reference_modifier::operator=(const Reference_modifier &) noexcept
  = default;
Reference_modifier::Reference_modifier(Reference_modifier &&) noexcept
  = default;
Reference_modifier & Reference_modifier::operator=(Reference_modifier &&) noexcept
  = default;

const Value * Reference_modifier::apply_readonly_opt(const Value &parent) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_array_index:
      {
        const auto &alt = this->m_stor.as<S_array_index>();
        if(parent.type() == Value::type_null) {
          return nullptr;
        }
        const auto qarr = parent.opt<D_array>();
        if(!qarr) {
          ASTERIA_THROW_RUNTIME_ERROR("Index `", alt.index, "` cannot be applied to `", parent, "` because it is not an array.");
        }
        const auto ssize = static_cast<Sint64>(qarr->size());
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
        auto rit = qarr->begin() + static_cast<Diff>(rindex);
        return &(rit[0]);
      }
    case index_object_key:
      {
        const auto &alt = this->m_stor.as<S_object_key>();
        if(parent.type() == Value::type_null) {
          return nullptr;
        }
        const auto qobj = parent.opt<D_object>();
        if(!qobj) {
          ASTERIA_THROW_RUNTIME_ERROR("Key `", alt.key, "` cannot be applied to `", parent, "` because it is not an object.");
        }
        auto rit = qobj->find(alt.key);
        if(rit == qobj->end()) {
          ASTERIA_DEBUG_LOG("Object key was not found: key = ", alt.key, ", parent = ", parent);
          return nullptr;
        }
        return &(rit->second);
      }
    default:
      ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

Value * Reference_modifier::apply_mutable_opt(Value &parent, bool create_new, Value *erased_out_opt) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_array_index:
      {
        const auto &alt = this->m_stor.as<S_array_index>();
        if(parent.type() == Value::type_null) {
          if(!create_new) {
            return nullptr;
          }
          parent = D_array();
        }
        const auto qarr = parent.opt<D_array>();
        if(!qarr) {
          ASTERIA_THROW_RUNTIME_ERROR("Index `", alt.index, "` cannot be applied to `", parent, "` because it is not an array.");
        }
        const auto ssize = static_cast<Sint64>(qarr->size());
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
          if(size_add >= qarr->max_size() - qarr->size()) {
            ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", qarr->size(), "` by `", size_add, "` would exceed system resource limits.");
          }
          rit = qarr->insert(qarr->begin(), static_cast<Size>(size_add));
          goto ma;
        }
        if(rindex >= ssize) {
          if(!create_new) {
            ASTERIA_DEBUG_LOG("Array index fell after the back: index = ", alt.index, ", parent = ", parent);
            return nullptr;
          }
          const auto size_add = static_cast<Uint64>(rindex) + 1 - qarr->size();
          if(size_add >= qarr->max_size() - qarr->size()) {
            ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", qarr->size(), "` by `", size_add, "` would exceed system resource limits.");
          }
          rit = qarr->insert(qarr->end(), static_cast<Size>(size_add)) + static_cast<Diff>(size_add - 1);
          goto ma;
        }
        rit = qarr->mut_begin() + static_cast<Diff>(rindex);
  ma:
        if(erased_out_opt) {
          *erased_out_opt = std::move(rit[0]);
          qarr->erase(rit);
          return erased_out_opt;
        }
        return &(rit[0]);
      }
    case index_object_key:
      {
        const auto &alt = this->m_stor.as<S_object_key>();
        if(parent.type() == Value::type_null) {
          if(!create_new) {
            return nullptr;
          }
          parent = D_object();
        }
        const auto qobj = parent.opt<D_object>();
        if(!qobj) {
          ASTERIA_THROW_RUNTIME_ERROR("Key `", alt.key, "` cannot be applied to `", parent, "` because it is not an object.");
        }
        D_object::iterator rit;
        if(!create_new) {
          rit = qobj->find_mut(alt.key);
          if(rit == qobj->end()) {
            ASTERIA_DEBUG_LOG("Object key was not found: key = ", alt.key, ", parent = ", parent);
            return nullptr;
          }
          goto mo;
        }
        rit = qobj->try_emplace(alt.key).first;
  mo:
        if(erased_out_opt) {
          *erased_out_opt = std::move(rit->second);
          qobj->erase(rit);
          return erased_out_opt;
        }
        return &(rit->second);
      }
    default:
      ASTERIA_TERMINATE("An unknown reference modifier type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

}
