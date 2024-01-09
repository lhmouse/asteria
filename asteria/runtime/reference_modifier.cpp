// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "reference_modifier.hpp"
#include "runtime_error.hpp"
#include "../value.hpp"
#include "../utils.hpp"
namespace asteria {

const Value*
Reference_Modifier::
apply_read_opt(const Value& parent) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
      case index_array_index: {
        // Get the element at the given index.
        const auto& altr = this->m_stor.as<S_array_index>();
        if(parent.is_null())
          return nullptr;
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Integer subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        const auto& arr = parent.as_array();
        auto w = wrap_array_index(arr.ssize(), altr.index);

        if(w.nprepend | w.nappend)
          return nullptr;

        return arr.data() + w.rindex;
      }

      case index_object_key: {
        // Get the value with the given key.
        const auto& altr = this->m_stor.as<S_object_key>();
        if(parent.is_null())
          return nullptr;
        else if(!parent.is_object())
          throw Runtime_Error(xtc_format,
                   "String subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        const auto& obj = parent.as_object();

        return obj.ptr(altr.key);
      }

      case index_array_head: {
        // Get the first element.
        if(parent.is_null())
          return nullptr;
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Head operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        const auto& arr = parent.as_array();

        if(arr.empty())
          return nullptr;

        return &(arr.front());
      }

      case index_array_tail: {
        // Get the last element.
        if(parent.is_null())
          return nullptr;
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Tail operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        const auto& arr = parent.as_array();

        if(arr.empty())
          return nullptr;

        return &(arr.back());
      }

      case index_array_random: {
        // Get a random element.
        const auto& altr = this->m_stor.as<S_array_random>();
        if(parent.is_null())
          return nullptr;
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Random operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        const auto& arr = parent.as_array();

        if(arr.empty())
          return nullptr;

        size_t off = ::rocket::probe_origin(arr.size(), altr.seed);
        return arr.data() + off;
      }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
    }
  }

Value*
Reference_Modifier::
apply_write_opt(Value& parent) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
      case index_array_index: {
        // Get the element at the given index.
        const auto& altr = this->m_stor.as<S_array_index>();
        if(parent.is_null())
          return nullptr;
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Integer subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();
        auto w = wrap_array_index(arr.ssize(), altr.index);

        if(w.nprepend | w.nappend)
          return nullptr;

        return arr.mut_data() + w.rindex;
      }

      case index_object_key: {
        // Get the value with the given key.
        const auto& altr = this->m_stor.as<S_object_key>();
        if(parent.is_null())
          return nullptr;
        else if(!parent.is_object())
          throw Runtime_Error(xtc_format,
                   "String subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& obj = parent.mut_object();

        return obj.mut_ptr(altr.key);
      }

      case index_array_head: {
        // Get the first element.
        if(parent.is_null())
          return nullptr;
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Head operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();

        if(arr.empty())
          return nullptr;

        return &(arr.mut_front());
      }

      case index_array_tail: {
        // Get the last element.
        if(parent.is_null())
          return nullptr;
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Tail operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();

        if(arr.empty())
          return nullptr;

        return &(arr.mut_back());
      }

      case index_array_random: {
        // Get a random element.
        const auto& altr = this->m_stor.as<S_array_random>();
        if(parent.is_null())
          return nullptr;
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Random operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();

        if(arr.empty())
          return nullptr;

        size_t off = ::rocket::probe_origin(arr.size(), altr.seed);
        return arr.mut_data() + off;
      }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
    }
  }

Value&
Reference_Modifier::
apply_open(Value& parent) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
      case index_array_index: {
        // Get the element at the given index.
        const auto& altr = this->m_stor.as<S_array_index>();
        if(parent.is_null())
          parent = V_array();
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Integer subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();
        auto w = wrap_array_index(arr.ssize(), altr.index);

        if(w.nprepend)
          arr.insert(arr.begin(), ::rocket::clamp_cast<size_t>(w.nprepend, 0U, SIZE_MAX));
        else if(w.nappend)
          arr.append(::rocket::clamp_cast<size_t>(w.nappend, 0U, SIZE_MAX));

        return *(arr.mut_data() + w.rindex);
      }

      case index_object_key: {
        // Get the value with the given key.
        const auto& altr = this->m_stor.as<S_object_key>();
        if(parent.is_null())
          parent = V_object();
        else if(!parent.is_object())
          throw Runtime_Error(xtc_format,
                   "String subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& obj = parent.mut_object();

        auto r = obj.try_emplace(altr.key);
        return r.first->second;
      }

      case index_array_head: {
        // Get the first element.
        if(parent.is_null())
          parent = V_array();
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Head operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();

        if(arr.empty())
          arr.insert(arr.begin(), nullopt);
        else
          arr.insert(arr.begin(), arr.front());

        return arr.mut_front();
      }

      case index_array_tail: {
        // Get the last element.
        if(parent.is_null())
          parent = V_array();
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Tail operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();

        if(arr.empty())
          arr.emplace_back();
        else
          arr.push_back(arr.back());

        return arr.mut_back();
      }

      case index_array_random: {
        // Get a random element.
        const auto& altr = this->m_stor.as<S_array_random>();
        if(parent.is_null())
          parent = V_array();
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Random operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();

        if(arr.empty())
          throw Runtime_Error(xtc_format,
                   "Cannot write to random element of an empty array");

        size_t off = ::rocket::probe_origin(arr.size(), altr.seed);
        return *(arr.mut_data() + off);
      }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
    }
  }

Value
Reference_Modifier::
apply_unset(Value& parent) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
      case index_array_index: {
        // Get the element at the given index.
        const auto& altr = this->m_stor.as<S_array_index>();
        if(parent.is_null())
          return nullopt;
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Integer subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();
        auto w = wrap_array_index(arr.ssize(), altr.index);

        if(w.nprepend | w.nappend)
          return nullopt;

        auto it = arr.mut_begin() + static_cast<ptrdiff_t>(w.rindex);
        auto val = move(*it);
        arr.erase(it);
        return val;
      }

      case index_object_key: {
        // Get the value with the given key.
        const auto& altr = this->m_stor.as<S_object_key>();
        if(parent.is_null())
          return nullopt;
        else if(!parent.is_object())
          throw Runtime_Error(xtc_format,
                   "String subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& obj = parent.mut_object();

        auto it = obj.mut_find(altr.key);
        if(it == obj.end())
          return nullopt;

        auto val = move(it->second);
        obj.erase(it);
        return val;
      }

      case index_array_head: {
        // Get the first element.
        if(parent.is_null())
          return nullopt;
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Head operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();

        if(arr.empty())
          return nullopt;

        auto it = arr.mut_begin();
        auto val = move(*it);
        arr.erase(it);
        return val;
      }

      case index_array_tail: {
        // Get the last element.
        if(parent.is_null())
          return nullopt;
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Tail operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();

        if(arr.empty())
          return nullopt;

        auto it = arr.mut_end() - 1;
        auto val = move(*it);
        arr.erase(it);
        return val;
      }

      case index_array_random: {
        // Get a random element.
        const auto& altr = this->m_stor.as<S_array_random>();
        if(parent.is_null())
          return nullopt;
        else if(!parent.is_array())
          throw Runtime_Error(xtc_format,
                   "Random operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();

        if(arr.empty())
          return nullopt;

        size_t off = ::rocket::probe_origin(arr.size(), altr.seed);
        auto it = arr.mut_begin() + static_cast<ptrdiff_t>(off);
        auto val = move(*it);
        arr.erase(it);
        return val;
      }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
    }
  }

}  // namespace asteria
