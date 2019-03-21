// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_root.hpp"
#include "abstract_variable_callback.hpp"
#include "../utilities.hpp"

namespace Asteria {

const Value& Reference_Root::dereference_const() const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_null:
      {
        return Value::get_null();
      }
    case index_constant:
      {
        const auto& alt = this->m_stor.as<S_constant>();
        return alt.source;
      }
    case index_temporary:
      {
        const auto& alt = this->m_stor.as<S_temporary>();
        return alt.value;
      }
    case index_variable:
      {
        const auto& alt = this->m_stor.as<S_variable>();
        if(!alt.var_opt) {
          return Value::get_null();
        }
        return alt.var_opt->get_value();
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

Value& Reference_Root::dereference_mutable() const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_null:
      {
        ASTERIA_THROW_RUNTIME_ERROR("The constant `null` cannot be modified.");
      }
    case index_constant:
      {
        const auto& alt = this->m_stor.as<S_constant>();
        ASTERIA_THROW_RUNTIME_ERROR("The constant `", alt.source, "` cannot be modified.");
      }
    case index_temporary:
      {
        const auto& alt = this->m_stor.as<S_temporary>();
        ASTERIA_THROW_RUNTIME_ERROR("The temporary value `", alt.value, "` cannot be modified.");
      }
    case index_variable:
      {
        const auto& alt = this->m_stor.as<S_variable>();
        if(!alt.var_opt) {
          ASTERIA_THROW_RUNTIME_ERROR("The reference cannot be written after being moved. This is likely a bug. Please report.");
        }
        if(alt.var_opt->is_immutable()) {
          ASTERIA_THROW_RUNTIME_ERROR("This variable having value `", alt.var_opt->get_value(), "` is immutable and cannot be modified.");
        }
        return alt.var_opt->open_value();
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

void Reference_Root::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_null:
      {
        return;
      }
    case index_constant:
      {
        const auto& alt = this->m_stor.as<S_constant>();
        alt.source.enumerate_variables(callback);
        return;
      }
    case index_temporary:
      {
        const auto& alt = this->m_stor.as<S_temporary>();
        alt.value.enumerate_variables(callback);
        return;
      }
    case index_variable:
      {
        const auto& alt = this->m_stor.as<S_variable>();
        if(!alt.var_opt) {
          return;
        }
        if(!callback(alt.var_opt)) {
          return;
        }
        // Descend into this variable recursively when the callback returns `true`.
        alt.var_opt->enumerate_variables(callback);
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

}  // namespace Asteria
