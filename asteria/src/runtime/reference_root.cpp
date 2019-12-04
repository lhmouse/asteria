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
        ASTERIA_THROW("tail call wrapper not dereferenceable");
      }
    default:
      ASTERIA_TERMINATE("invalid reference root type enumeration (index `", this->index(), "`)");
    }
  }

Value& Reference_Root::dereference_mutable() const
  {
    switch(this->index()) {
    case index_null:
      {
        ASTERIA_THROW("attempt to modify a constant `null`");
      }
    case index_constant:
      {
        ASTERIA_THROW("attempt to modify a constant `", this->m_stor.as<index_constant>().val, "`");
      }
    case index_temporary:
      {
        ASTERIA_THROW("attempt to modify a temporary `", this->m_stor.as<index_temporary>().val, "`");
      }
    case index_variable:
      {
        const auto& var = this->m_stor.as<index_variable>().var;
        if(var->is_immutable()) {
          ASTERIA_THROW("attempt to modify an immutable variable `", var->get_value(), "`");
        }
        return var->open_value();
      }
    case index_tail_call:
      {
        ASTERIA_THROW("tail call wrapper not dereferenceable");
      }
    default:
      ASTERIA_TERMINATE("invalid reference root type enumeration (index `", this->index(), "`)");
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
          var->enumerate_variables(callback);
        }
        return callback;
      }
    case index_tail_call:
      {
        return this->m_stor.as<index_tail_call>().tca->enumerate_variables(callback);
      }
    default:
      ASTERIA_TERMINATE("invalid reference root type enumeration (index `", this->index(), "`)");
    }
  }

}  // namespace Asteria
