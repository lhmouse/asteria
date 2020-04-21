// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_modifier.hpp"
#include "../value.hpp"
#include "../utilities.hpp"

namespace Asteria {

const Value*
Reference_modifier::
apply_const_opt(const Value& parent)
const
  {
    switch(this->index()) {
      case index_array_index: {
        const auto& altr = this->m_stor.as<index_array_index>();

        // Elements of a `null` are also `null`s.
        if(parent.is_null())
          return nullptr;

        if(!parent.is_array())
          ASTERIA_THROW("integer subscript applied to non-array (parent `$1`, index `$2`)", parent, altr.index);
        const auto& arr = parent.as_array();

        // Return a pointer to the element at the given index.
        auto w = wrap_index(altr.index, arr.size());
        size_t nadd = w.nprepend | w.nappend;
        if(nadd != 0)
          return nullptr;

        return ::std::addressof(arr.at(w.rindex));
      }

      case index_object_key: {
        const auto& altr = this->m_stor.as<index_object_key>();

        // Members of a `null` are also `null`s.
        if(parent.is_null())
          return nullptr;

        if(!parent.is_object())
          ASTERIA_THROW("string subscript applied to non-object (parent `$1`, key `$2`)", parent, altr.key);
        const auto& obj = parent.as_object();

        // Return a pointer to the value with the given key.
        auto q = obj.find(altr.key);
        if(q == obj.end())
          return nullptr;

        return ::std::addressof(q->second);
      }

      case index_array_head: {
        // Elements of a `null` are also `null`s.
        if(parent.is_null())
          return nullptr;

        if(!parent.is_array())
          ASTERIA_THROW("head subscript applied to non-array (parent `$1`)", parent);
        const auto& arr = parent.as_array();

        // Returns the first element.
        if(arr.empty())
          return nullptr;

        return ::std::addressof(arr.front());
      }

      case index_array_tail: {
        // Elements of a `null` are also `null`s.
        if(parent.is_null())
          return nullptr;

        if(!parent.is_array())
          ASTERIA_THROW("tail subscript applied to non-array (parent `$1`)", parent);
        const auto& arr = parent.as_array();

        // Returns the last element.
        if(arr.empty())
          return nullptr;

        return ::std::addressof(arr.back());
      }

      default:
        ASTERIA_TERMINATE("invalid reference modifier type (index `$1`)", this->index());
    }
  }

Value*
Reference_modifier::
apply_mutable_opt(Value& parent)
const
  {
    switch(this->index()) {
      case index_array_index: {
        const auto& altr = this->m_stor.as<index_array_index>();

        // Elements of a `null` are also `null`s.
        if(parent.is_null())
          return nullptr;

        if(!parent.is_array())
          ASTERIA_THROW("integer subscript applied to non-array (parent `$1`, index `$2`)", parent, altr.index);
        auto& arr = parent.open_array();

        // Return a pointer to the element at the given index.
        auto w = wrap_index(altr.index, arr.size());
        size_t nadd = w.nprepend | w.nappend;
        if(nadd != 0)
          return nullptr;

        return ::std::addressof(arr.mut(w.rindex));
      }

      case index_object_key: {
        const auto& altr = this->m_stor.as<index_object_key>();

        // Members of a `null` are also `null`s.
        if(parent.is_null())
          return nullptr;

        if(!parent.is_object())
          ASTERIA_THROW("string subscript applied to non-object (parent `$1`, key `$2`)", parent, altr.key);
        auto& obj = parent.open_object();

        // Return a pointer to the value with the given key.
        auto q = obj.find_mut(altr.key);
        if(q == obj.end())
          return nullptr;

        return ::std::addressof(q->second);
      }

      case index_array_head: {
        // Elements of a `null` are also `null`s.
        if(parent.is_null())
          return nullptr;

        if(!parent.is_array())
          ASTERIA_THROW("head subscript applied to non-array (parent `$1`)", parent);
        auto& arr = parent.open_array();

        // Returns the first element.
        if(arr.empty())
          return nullptr;

        return ::std::addressof(arr.mut_front());
      }

      case index_array_tail: {
        // Elements of a `null` are also `null`s.
        if(parent.is_null())
          return nullptr;

        if(!parent.is_array())
          ASTERIA_THROW("tail subscript applied to non-array (parent `$1`)", parent);
        auto& arr = parent.open_array();

        // Returns the last element.
        if(arr.empty())
          return nullptr;

        return ::std::addressof(arr.mut_back());
      }

      default:
        ASTERIA_TERMINATE("invalid reference modifier type (index `$1`)", this->index());
    }
  }

Value&
Reference_modifier::
apply_and_create(Value& parent)
const
  {
    switch(this->index()) {
      case index_array_index: {
        const auto& altr = this->m_stor.as<index_array_index>();

        // Elements of a `null` are also `null`s.
        if(parent.is_null())
          parent = V_array();

        if(!parent.is_array())
          ASTERIA_THROW("integer subscript applied to non-array (parent `$1`, index `$2`)", parent, altr.index);
        auto& arr = parent.open_array();

        // Return a pointer to the element at the given index if the index is valid.
        auto w = wrap_index(altr.index, arr.size());
        size_t nadd = w.nprepend | w.nappend;
        if(nadd != 0) {
          // Create elements as needed.
          if(nadd > arr.max_size() - arr.size())
            ASTERIA_THROW("array length overflow (`$1` + `$2` > `$3`)", arr.size(), nadd, arr.max_size());

          arr.insert(arr.begin(), static_cast<size_t>(w.nprepend));
          arr.insert(arr.end(), static_cast<size_t>(w.nappend));
        }

        return arr.mut(w.rindex);
      }

      case index_object_key: {
        const auto& altr = this->m_stor.as<index_object_key>();

        // Members of a `null` are also `null`s.
        if(parent.is_null())
          parent = V_object();

        if(!parent.is_object())
          ASTERIA_THROW("string subscript applied to non-object (parent `$1`, key `$2`)", parent, altr.key);
        auto& obj = parent.open_object();

        // Create a value as needed.
        auto q = obj.try_emplace(altr.key).first;

        return q->second;
      }

      case index_array_head: {
        // Prepend an element to the array.
        if(parent.is_null())
          parent = V_array();

        if(!parent.is_array())
          ASTERIA_THROW("head subscript applied to non-array (parent `$1`)", parent);
        auto& arr = parent.open_array();

        return *(arr.insert(arr.begin(), V_null()));
      }

      case index_array_tail: {
        // Append an element to the array.
        if(parent.is_null())
          parent = V_array();

        if(!parent.is_array())
          ASTERIA_THROW("tail subscript applied to non-array (parent `$1`)", parent);
        auto& arr = parent.open_array();

        return arr.emplace_back(V_null());
      }

      default:
        ASTERIA_TERMINATE("invalid reference modifier type (index `$1`)", this->index());
    }
  }

Value
Reference_modifier::
apply_and_erase(Value& parent)
const
  {
    switch(this->index()) {
      case index_array_index: {
        const auto& altr = this->m_stor.as<index_array_index>();

        // Elements of a `null` are also `null`s.
        if(parent.is_null())
          return V_null();

        if(!parent.is_array())
          ASTERIA_THROW("integer subscript applied to non-array (parent `$1`, index `$2`)", parent, altr.index);
        auto& arr = parent.open_array();

        // Erase the element at the given index and return it.
        auto w = wrap_index(altr.index, arr.size());
        size_t nadd = w.nprepend | w.nappend;
        if(nadd != 0)
          return V_null();

        auto val = ::std::move(arr.mut(w.rindex));
        arr.erase(w.rindex, 1);
        return val;
      }

      case index_object_key: {
        const auto& altr = this->m_stor.as<index_object_key>();

        // Members of a `null` are also `null`s.
        if(parent.is_null())
          return V_null();

        if(!parent.is_object())
          ASTERIA_THROW("string subscript applied to non-object (parent `$1`, key `$2`)", parent, altr.key);
        auto& obj = parent.open_object();

        // Erase the value with the given key and return it.
        auto q = obj.find_mut(altr.key);
        if(q == obj.end())
          return V_null();

        auto val = ::std::move(q->second);
        obj.erase(q);
        return val;
      }

      case index_array_head: {
        // Elements of a `null` are also `null`s.
        if(parent.is_null())
          return V_null();

        if(!parent.is_array())
          ASTERIA_THROW("head subscript applied to non-array (parent `$1`)", parent);
        auto& arr = parent.open_array();

        // Erase the first element and return it.
        if(arr.empty())
          return V_null();

        auto val = ::std::move(arr.mut_front());
        arr.erase(arr.begin());
        return val;
      }

      case index_array_tail: {
        // Elements of a `null` are also `null`s.
        if(parent.is_null())
          return V_null();

        if(!parent.is_array())
          ASTERIA_THROW("tail subscript applied to non-array (parent `$1`)", parent);
        auto& arr = parent.open_array();

        // Erase the last element and return it.
        if(arr.empty())
          return V_null();

        auto val = ::std::move(arr.mut_back());
        arr.pop_back();
        return val;
      }

      default:
        ASTERIA_TERMINATE("invalid reference modifier type (index `$1`)", this->index());
    }
  }

}  // namespace Asteria
