// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_root.hpp"
#include "variable_callback.hpp"
#include "reference.hpp"
#include "../utilities.hpp"

namespace Asteria {

const Value& Reference_Root::dereference_const() const
  {
    switch(this->index()) {
    case index_null:
      {
        return null_value;
      }
    case index_constant:
      {
        return this->m_stor.as<index_constant>().val;
      }
    case index_temporary:
      {
        return this->m_stor.as<index_temporary>().val;
      }
    case index_variable:
      {
        return this->m_stor.as<index_variable>().var->get_value();
      }
    case index_tail_call:
      {
        ASTERIA_THROW_RUNTIME_ERROR("Tail call wrappers cannot be dereferenced directly.");
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

Value& Reference_Root::dereference_mutable() const
  {
    switch(this->index()) {
    case index_null:
      {
        ASTERIA_THROW_RUNTIME_ERROR("The constant `null` cannot be modified.");
      }
    case index_constant:
      {
        ASTERIA_THROW_RUNTIME_ERROR("The constant `", this->m_stor.as<index_constant>().val, "` cannot be modified.");
      }
    case index_temporary:
      {
        ASTERIA_THROW_RUNTIME_ERROR("The temporary value `", this->m_stor.as<index_temporary>().val, "` cannot be modified.");
      }
    case index_variable:
      {
        const auto& var = this->m_stor.as<index_variable>().var;
        if(var->is_immutable()) {
          ASTERIA_THROW_RUNTIME_ERROR("This variable having value `", var->get_value(), "` is immutable and cannot be modified.");
        }
        return var->open_value();
      }
    case index_tail_call:
      {
        ASTERIA_THROW_RUNTIME_ERROR("Tail call wrappers cannot be dereferenced directly.");
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

Variable_Callback& Reference_Root::enumerate_variables(Variable_Callback& callback) const
  {
    switch(this->index()) {
    case index_null:
      {
        return callback;
      }
    case index_constant:
      {
        return this->m_stor.as<index_constant>().val.enumerate_variables(callback);
      }
    case index_temporary:
      {
        return this->m_stor.as<index_temporary>().val.enumerate_variables(callback);
      }
    case index_variable:
      {
        const auto& var = this->m_stor.as<index_variable>().var;
        if(callback(var)) {
          // Descend into this variable recursively when the callback returns `true`.
          var->enumerate_variables(callback);
        }
        return callback;
      }
    case index_tail_call:
      {
        return this->m_stor.as<index_tail_call>().tca->enumerate_variables(callback);
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
