// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_modifier.hpp"
#include "../value.hpp"
#include "../utilities.hpp"

namespace Asteria {

const Value* Reference_Modifier::apply_const_opt(const Value& parent) const
  {
    switch(this->index()) {
      {{
    case index_array_index:
        const auto& altr = this->m_stor.as<index_array_index>();
        if(parent.is_null()) {
          // Elements of a `null` are also `null`s.
          return nullptr;
        }
        if(!parent.is_array()) {
          ASTERIA_THROW("integer subscript applied to non-array (parent `$1`, index `$2`)", parent, altr.index);
        }
        const auto& arr = parent.as_array();
        // Return a pointer to the element at the given index.
        auto w = wrap_index(altr.index, arr.size());
        auto nadd = w.nprepend | w.nappend;
        if(nadd != 0) {
          return nullptr;
        }
        return ::std::addressof(arr.at(w.rindex));
      }{
    case index_object_key:
        const auto& altr = this->m_stor.as<index_object_key>();
        if(parent.is_null()) {
          // Members of a `null` are also `null`s.
          return nullptr;
        }
        if(!parent.is_object()) {
          ASTERIA_THROW("string subscript applied to non-object (parent `$1`, key `$2`)", parent, altr.key);
        }
        const auto& obj = parent.as_object();
        // Return a pointer to the value with the given key.
        auto rit = obj.find(altr.key);
        if(rit == obj.end()) {
          return nullptr;
        }
        return ::std::addressof(rit->second);
      }{
    case index_array_tail:
        // We have to verify that the parent value is actually an `array` or `null`.
        if(parent.is_null()) {
          // Elements of a `null` are also `null`s.
          return nullptr;
        }
        if(!parent.is_array()) {
          ASTERIA_THROW("tail subscript applied to non-array (parent `$1`)", parent);
        }
        const auto& arr = parent.as_array();
        // Returns the last element.
        if(arr.empty()) {
          return nullptr;
        }
        return ::std::addressof(arr.back());
      }}
    default:
      ASTERIA_TERMINATE("invalid reference modifier type (index `$1`)", this->index());
    }
  }

Value* Reference_Modifier::apply_mutable_opt(Value& parent, bool create_new) const
  {
    switch(this->index()) {
      {{
    case index_array_index:
        const auto& altr = this->m_stor.as<index_array_index>();
        if(parent.is_null()) {
          // Create elements as needed.
          if(!create_new) {
            return nullptr;
          }
          parent = G_array();
        }
        if(!parent.is_array()) {
          ASTERIA_THROW("integer subscript applied to non-array (parent `$1`, index `$2`)", parent, altr.index);
        }
        auto& arr = parent.open_array();
        // Return a pointer to the element at the given index if the index is valid; create an element if it is out of range.
        auto w = wrap_index(altr.index, arr.size());
        auto nadd = w.nprepend | w.nappend;
        if(nadd != 0) {
          if(!create_new) {
            return nullptr;
          }
          if(nadd > arr.max_size() - arr.size()) {
            ASTERIA_THROW("array length overflow (`$1` + `$2` > `$3`)", arr.size(), nadd, arr.max_size());
          }
          arr.insert(arr.begin(), static_cast<size_t>(w.nprepend));
          arr.insert(arr.end(), static_cast<size_t>(w.nappend));
        }
        return ::std::addressof(arr.mut(w.rindex));
      }{
    case index_object_key:
        const auto& altr = this->m_stor.as<index_object_key>();
        if(parent.is_null()) {
          // Create members as needed.
          if(!create_new) {
            return nullptr;
          }
          parent = G_object();
        }
        if(!parent.is_object()) {
          ASTERIA_THROW("string subscript applied to non-object (parent `$1`, key `$2`)", parent, altr.key);
        }
        auto& obj = parent.open_object();
        // Return a pointer to the value with the given key if it is found; create a value otherwise.
        G_object::iterator rit;
        if(!create_new) {
          rit = obj.find_mut(altr.key);
          if(rit == obj.end()) {
            return nullptr;
          }
        }
        else {
          rit = obj.try_emplace(altr.key).first;
        }
        return ::std::addressof(rit->second);
      }{
    case index_array_tail:
        // We have to verify that the parent value is actually an `array` or `null`.
        if(parent.is_null()) {
          // Create elements as needed.
          if(!create_new) {
            return nullptr;
          }
          parent = G_array();
        }
        if(!parent.is_array()) {
          ASTERIA_THROW("tail subscript applied to non-array (parent `$1`)", parent);
        }
        auto& arr = parent.open_array();
        // Append a new element to the end and return a pointer to it.
        return ::std::addressof(arr.emplace_back());
      }}
    default:
      ASTERIA_TERMINATE("invalid reference modifier type (index `$1`)", this->index());
    }
  }

Value Reference_Modifier::apply_and_erase(Value& parent) const
  {
    switch(this->index()) {
      {{
    case index_array_index:
        const auto& altr = this->m_stor.as<index_array_index>();
        if(parent.is_null()) {
          // There is nothing to erase.
          return nullptr;
        }
        if(!parent.is_array()) {
          ASTERIA_THROW("integer subscript applied to non-array (parent `$1`, index `$2`)", parent, altr.index);
        }
        auto& arr = parent.open_array();
        // Erase the element at the given index and return it.
        auto w = wrap_index(altr.index, arr.size());
        auto nadd = w.nprepend | w.nappend;
        if(nadd != 0) {
          return nullptr;
        }
        auto elem = ::rocket::move(arr.mut(w.rindex));
        arr.erase(w.rindex, 1);
        return elem;
      }{
    case index_object_key:
        const auto& altr = this->m_stor.as<index_object_key>();
        if(parent.is_null()) {
          // There is nothing to erase.
          return nullptr;
        }
        if(!parent.is_object()) {
          ASTERIA_THROW("string subscript applied to non-object (parent `$1`, key `$2`)", parent, altr.key);
        }
        auto& obj = parent.open_object();
        // Erase the value with the given key and return it.
        auto rit = obj.find_mut(altr.key);
        if(rit == obj.end()) {
          return nullptr;
        }
        auto elem = ::rocket::move(rit->second);
        obj.erase(rit);
        return elem;
      }{
    case index_array_tail:
        // We have to verify that the parent value is actually an `array` or `null`.
        if(parent.is_null()) {
          // There is nothing to erase.
          return nullptr;
        }
        if(!parent.is_array()) {
          ASTERIA_THROW("tail subscript applied to non-array (parent `$1`)", parent);
        }
        auto& arr = parent.open_array();
        // Erase the last element and return it.
        if(arr.empty()) {
          return nullptr;
        }
        auto elem = ::rocket::move(arr.mut_back());
        arr.pop_back();
        return elem;
      }}
    default:
      ASTERIA_TERMINATE("invalid reference modifier type (index `$1`)", this->index());
    }
  }

}  // namespace Asteria
