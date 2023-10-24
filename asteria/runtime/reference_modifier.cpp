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
    switch(this->index()) {
      case index_array_index: {
        // Get the element at the given index.
        const auto& altr = this->m_stor.as<S_array_index>();
        if(parent.is_null()) {
          // Elements of null values are also null values.
          return nullptr;
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Integer subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        const auto& arr = parent.as_array();
        auto w = wrap_array_index(arr.ssize(), altr.index);
        if(w.nprepend | w.nappend)
          return nullptr;

        return &(arr[w.rindex]);
      }

      case index_object_key: {
        // Get the value with the given key.
        const auto& altr = this->m_stor.as<S_object_key>();
        if(parent.is_null()) {
          // Members of null values are also null values.
          return nullptr;
        }
        else if(!parent.is_object())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "String subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        const auto& obj = parent.as_object();
        return obj.ptr(altr.key);
      }

      case index_array_head: {
        // Get the first element.
        if(parent.is_null()) {
          // Elements of null values are also null values.
          return nullptr;
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Head operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        const auto& arr = parent.as_array();
        if(arr.empty())
          return nullptr;

        return &(arr.front());
      }

      case index_array_tail: {
        // Get the last element.
        if(parent.is_null()) {
          // Elements of null values are also null values.
          return nullptr;
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
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
        if(parent.is_null()) {
          // Elements of null values are also null values.
          return nullptr;
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Random operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        const auto& arr = parent.as_array();
        if(arr.empty())
          return nullptr;

        size_t r = ::rocket::probe_origin(arr.size(), altr.seed);
        return arr.data() + r;
      }

      default:
        ASTERIA_TERMINATE(("Invalid reference modifier type `$1`"), this->index());
    }
  }

Value*
Reference_Modifier::
apply_write_opt(Value& parent) const
  {
    switch(this->index()) {
      case index_array_index: {
        // Get the element at the given index.
        const auto& altr = this->m_stor.as<S_array_index>();
        if(parent.is_null()) {
          // Elements of null values are also null values.
          return nullptr;
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Integer subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();
        auto w = wrap_array_index(arr.ssize(), altr.index);
        if(w.nprepend | w.nappend)
          return nullptr;

        return &(arr.mut(w.rindex));
      }

      case index_object_key: {
        // Get the value with the given key.
        const auto& altr = this->m_stor.as<S_object_key>();
        if(parent.is_null()) {
          // Members of null values are also null values.
          return nullptr;
        }
        else if(!parent.is_object())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "String subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& obj = parent.mut_object();
        return obj.mut_ptr(altr.key);
      }

      case index_array_head: {
        // Get the first element.
        if(parent.is_null()) {
          // Elements of null values are also null values.
          return nullptr;
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Head operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();
        if(arr.empty())
          return nullptr;

        return &(arr.mut_front());
      }

      case index_array_tail: {
        // Get the last element.
        if(parent.is_null()) {
          // Elements of null values are also null values.
          return nullptr;
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
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
        if(parent.is_null()) {
          // Elements of null values are also null values.
          return nullptr;
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Random operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();
        if(arr.empty())
          return nullptr;

        size_t r = ::rocket::probe_origin(arr.size(), altr.seed);
        return arr.mut_data() + r;
      }

      default:
        ASTERIA_TERMINATE(("Invalid reference modifier type `$1`"), this->index());
    }
  }

Value&
Reference_Modifier::
apply_open(Value& parent) const
  {
    switch(this->index()) {
      case index_array_index: {
        // Get the element at the given index.
        const auto& altr = this->m_stor.as<S_array_index>();
        if(parent.is_null()) {
          // Empty arrays are created if null values are encountered.
          parent = V_array();
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Integer subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();
        auto w = wrap_array_index(arr.ssize(), altr.index);
        if(w.nprepend)
          arr.insert(arr.begin(), w.nprepend);
        else if(w.nappend)
          arr.append(w.nappend);

        return arr.mut(w.rindex);
      }

      case index_object_key: {
        // Get the value with the given key.
        const auto& altr = this->m_stor.as<S_object_key>();
        if(parent.is_null()) {
          // Empty objects are created if null values are encountered.
          parent = V_object();
        }
        else if(!parent.is_object())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "String subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& obj = parent.mut_object();
        return obj.try_emplace(altr.key).first->second;
      }

      case index_array_head: {
        // Get the first element.
        if(parent.is_null()) {
          // Empty arrays are created if null values are encountered.
          parent = V_array();
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
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
        if(parent.is_null()) {
          // Empty arrays are created if null values are encountered.
          parent = V_array();
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
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
        if(parent.is_null()) {
          // Empty arrays are created if null values are encountered.
          parent = V_array();
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Random operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();
        if(arr.empty())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Cannot write to random element of an empty array");

        size_t r = ::rocket::probe_origin(arr.size(), altr.seed);
        return arr.mut(r);
      }

      default:
        ASTERIA_TERMINATE(("Invalid reference modifier type `$1`"), this->index());
    }
  }

Value
Reference_Modifier::
apply_unset(Value& parent) const
  {
    switch(this->index()) {
      case index_array_index: {
        // Get the element at the given index.
        const auto& altr = this->m_stor.as<S_array_index>();
        if(parent.is_null()) {
          // Elements of null values are also null values.
          return nullopt;
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Integer subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();
        auto w = wrap_array_index(arr.ssize(), altr.index);
        if(w.nprepend | w.nappend)
          return nullopt;

        auto val = arr.mut(w.rindex);
        arr.erase(w.rindex, 1);
        return val;
      }

      case index_object_key: {
        // Get the value with the given key.
        const auto& altr = this->m_stor.as<S_object_key>();
        if(parent.is_null()) {
          // Members of null values are also null values.
          return nullopt;
        }
        else if(!parent.is_object())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "String subscript not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& obj = parent.mut_object();
        auto it = obj.mut_find(altr.key);
        if(it == obj.end())
          return nullopt;

        auto val = ::std::move(it->second);
        obj.erase(it);
        return val;
      }

      case index_array_head: {
        // Get the first element.
        if(parent.is_null()) {
          // Elements of null values are also null values.
          return nullopt;
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Head operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();
        if(arr.empty())
          return nullopt;

        auto val = ::std::move(arr.mut_front());
        arr.erase(arr.begin());
        return val;
      }

      case index_array_tail: {
        // Get the last element.
        if(parent.is_null()) {
          // Elements of null values are also null values.
          return nullopt;
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Tail operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();
        if(arr.empty())
          return nullopt;

        auto val = ::std::move(arr.mut_back());
        arr.pop_back();
        return val;
      }

      case index_array_random: {
        // Get a random element.
        const auto& altr = this->m_stor.as<S_array_random>();
        if(parent.is_null()) {
          // Elements of null values are also null values.
          return nullopt;
        }
        else if(!parent.is_array())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Random operator not applicable (parent type `$1`)",
                   describe_type(parent.type()));

        auto& arr = parent.mut_array();
        if(arr.empty())
          return nullopt;

        size_t r = ::rocket::probe_origin(arr.size(), altr.seed);
        auto val = ::std::move(arr.mut(r));
        arr.erase(r, 1);
        return val;
      }

      default:
        ASTERIA_TERMINATE(("Invalid reference modifier type `$1`"), this->index());
    }
  }

}  // namespace asteria
