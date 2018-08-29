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
    case Reference_modifier::index_array_index:
      {
        const auto &alt = this->m_stor.as<Reference_modifier::S_array_index>();
        if(parent.type() == Value::type_null) {
          return nullptr;
        }
        const auto qarr = parent.opt<D_array>();
        if(!qarr) {
          ASTERIA_THROW_RUNTIME_ERROR("Index `", alt.index, "` cannot be applied to `", parent, "` because it is not an array.");
        }
        auto rindex = alt.index;
        if(rindex < 0) {
          // Wrap negative indices.
          rindex += static_cast<Signed>(qarr->size());
        }
        if(rindex < 0) {
          ASTERIA_DEBUG_LOG("Array index fell before the front: index = ", alt.index, ", parent = ", parent);
          return nullptr;
        }
        if(rindex >= static_cast<Signed>(qarr->size())) {
          ASTERIA_DEBUG_LOG("Array index fell after the back: index = ", alt.index, ", parent = ", parent);
          return nullptr;
        }
        const auto rit = qarr->begin() + static_cast<std::ptrdiff_t>(rindex);
        return &(rit[0]);
      }
    case Reference_modifier::index_object_key:
      {
        const auto &alt = this->m_stor.as<Reference_modifier::S_object_key>();
        if(parent.type() == Value::type_null) {
          return nullptr;
        }
        const auto qobj = parent.opt<D_object>();
        if(!qobj) {
          ASTERIA_THROW_RUNTIME_ERROR("Key `", alt.key, "` cannot be applied to `", parent, "` because it is not an object.");
        }
        const auto rit = qobj->find(alt.key);
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

Value * Reference_modifier::apply_mutable_opt(Value &parent, bool creates, Value *erased_out_opt) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case Reference_modifier::index_array_index:
      {
        const auto &alt = this->m_stor.as<Reference_modifier::S_array_index>();
        if(parent.type() == Value::type_null) {
          if(!creates) {
            return nullptr;
          }
          parent.set(D_array());
        }
        const auto qarr = parent.opt<D_array>();
        if(!qarr) {
          ASTERIA_THROW_RUNTIME_ERROR("Index `", alt.index, "` cannot be applied to `", parent, "` because it is not an array.");
        }
        auto rindex = alt.index;
        if(rindex < 0) {
          // Wrap negative indices.
          rindex += static_cast<Signed>(qarr->size());
        }
        if(rindex < 0) {
          ASTERIA_DEBUG_LOG("Array index fell before the front: index = ", alt.index, ", parent = ", parent);
          if(!creates) {
            return nullptr;
          }
          const auto size_add = static_cast<Unsigned>(0) - static_cast<Unsigned>(rindex);
          if(size_add >= qarr->max_size() - qarr->size()) {
            ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", qarr->size(), "` by `", size_add, "` would exceed system resource limits.");
          }
          qarr->insert(qarr->begin(), static_cast<std::size_t>(size_add));
          rindex = 0;
        }
        if(rindex >= static_cast<Signed>(qarr->size())) {
          ASTERIA_DEBUG_LOG("Array index fell after the back: index = ", alt.index, ", parent = ", parent);
          if(!creates) {
            return nullptr;
          }
          const auto size_add = static_cast<Unsigned>(rindex) + 1 - qarr->size();
          if(size_add >= qarr->max_size() - qarr->size()) {
            ASTERIA_THROW_RUNTIME_ERROR("Extending the array of size `", qarr->size(), "` by `", size_add, "` would exceed system resource limits.");
          }
          qarr->insert(qarr->end(), static_cast<std::size_t>(size_add));
        }
        const auto rit = qarr->mut_begin() + static_cast<std::ptrdiff_t>(rindex);
        if(erased_out_opt){
          *erased_out_opt = std::move(rit[0]);
          qarr->erase(rit);
          return erased_out_opt;
        }
        return &(rit[0]);
      }
    case Reference_modifier::index_object_key:
      {
        const auto &alt = this->m_stor.as<Reference_modifier::S_object_key>();
        if(parent.type() == Value::type_null) {
          if(!creates) {
            return nullptr;
          }
          parent.set(D_object());
        }
        const auto qobj = parent.opt<D_object>();
        if(!qobj) {
          ASTERIA_THROW_RUNTIME_ERROR("Key `", alt.key, "` cannot be applied to `", parent, "` because it is not an object.");
        }
        auto rit = D_object::iterator();
        if(!creates) {
          rit = qobj->find_mut(alt.key);
          if(rit == qobj->end()) {
            ASTERIA_DEBUG_LOG("Object key was not found: key = ", alt.key, ", parent = ", parent);
            return nullptr;
          }
        } else {
          const auto result = qobj->try_emplace(alt.key);
          if(result.second) {
            ASTERIA_DEBUG_LOG("New object key has been created: key = ", alt.key, ", parent = ", parent);
          }
          rit = result.first;
        }
        if(erased_out_opt){
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
