// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_root.hpp"
#include "reference.hpp"
#include "variable_callback.hpp"
#include "variable.hpp"
#include "ptc_arguments.hpp"
#include "../utilities.hpp"

namespace asteria {

const Value&
Reference_root::
dereference_const()
const
  {
    switch(this->index()) {
      case index_void:
        ASTERIA_THROW("Attempt to take the result of a function call which returned no value");

      case index_constant:
        return this->m_stor.as<index_constant>().val;

      case index_temporary:
        return this->m_stor.as<index_temporary>().val;

      case index_variable: {
        auto var = unerase_cast(this->m_stor.as<index_variable>().var);
        if(!var) {
          return null_value;
        }
        if(!var->is_initialized()) {
          ASTERIA_THROW("Attempt to read from an uninitialized variable");
        }
        return var->get_value();
      }

      case index_tail_call:
        ASTERIA_THROW("Tail call wrapper not dereferenceable");

      case index_jump_src:
        ASTERIA_THROW("Jump marker not dereferenceable");

      default:
        ASTERIA_TERMINATE("invalid reference root type enumeration (index `$1`)", this->index());
    }
  }

Value&
Reference_root::
dereference_mutable()
const
  {
    switch(this->index()) {
      case index_void:
        ASTERIA_THROW("Attempt to modify the result of a function call which returned no value");

      case index_constant:
        ASTERIA_THROW("Attempt to modify a constant `$1`", this->m_stor.as<index_constant>().val);

      case index_temporary:
        ASTERIA_THROW("Attempt to modify a temporary `$1`", this->m_stor.as<index_temporary>().val);

      case index_variable: {
        auto var = unerase_cast(this->m_stor.as<index_variable>().var);
        if(!var) {
          ASTERIA_THROW("Attempt to dereference a moved-away reference");
        }
        if(!var->is_initialized()) {
          ASTERIA_THROW("Attempt to modify an uninitialized variable");
        }
        if(var->is_immutable()) {
          ASTERIA_THROW("Attempt to modify an immutable variable `$1`", var->get_value());
        }
        return var->open_value();
      }

      case index_tail_call:
        ASTERIA_THROW("Tail call wrapper not dereferenceable");

      case index_jump_src:
        ASTERIA_THROW("Jump marker not dereferenceable");

      default:
        ASTERIA_TERMINATE("invalid reference root type enumeration (index `$1`)", this->index());
    }
  }

Variable_Callback&
Reference_root::
enumerate_variables(Variable_Callback& callback)
const
  {
    switch(this->index()) {
      case index_void:
        return callback;

      case index_constant:
        return this->m_stor.as<index_constant>().val.enumerate_variables(callback);

      case index_temporary:
        return this->m_stor.as<index_temporary>().val.enumerate_variables(callback);

      case index_variable: {
        auto var = unerase_cast(this->m_stor.as<index_variable>().var);
        if(!var) {
          return callback;
        }
        if(!callback.process(var)) {
          return callback;
        }
        return var->enumerate_variables(callback);
      }

      case index_tail_call: {
        auto ptc = unerase_cast(this->m_stor.as<index_tail_call>().tca);
        if(!ptc) {
          return callback;
        }
        return ptc->enumerate_variables(callback);
      }

      case index_jump_src:
        return callback;

      default:
        ASTERIA_TERMINATE("invalid reference root type enumeration (index `$1`)", this->index());
    }
  }

}  // namespace asteria
