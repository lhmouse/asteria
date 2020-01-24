// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_root.hpp"
#include "reference.hpp"
#include "variable_callback.hpp"
#include "tail_call_arguments.hpp"
#include "../utilities.hpp"

namespace Asteria {

const Value& Reference_Root::dereference_const() const
  {
    switch(this->index()) {
    case index_void: {
        ASTERIA_THROW("attempt to take the result of a function call which returned no value");
      }
    case index_constant: {
        return this->m_stor.as<index_constant>().val;
      }
    case index_temporary: {
        return this->m_stor.as<index_temporary>().val;
      }
    case index_variable: {
        const auto& var = this->m_stor.as<index_variable>().var;
        if(!var->is_initialized()) {
          ASTERIA_THROW("attempt to read from an uninitialized variable");
        }
        return var->get_value();
      }
    case index_tail_call: {
        ASTERIA_THROW("tail call wrapper not dereferenceable");
      }
    default:
      ASTERIA_TERMINATE("invalid reference root type enumeration (index `$1`)", this->index());
    }
  }

Value& Reference_Root::dereference_mutable() const
  {
    switch(this->index()) {
    case index_void: {
        ASTERIA_THROW("attempt to modify the result of a function call which returned no value");
      }
    case index_constant: {
        ASTERIA_THROW("attempt to modify a constant `$1`", this->m_stor.as<index_constant>().val);
      }
    case index_temporary: {
        ASTERIA_THROW("attempt to modify a temporary `$1`", this->m_stor.as<index_temporary>().val);
      }
    case index_variable: {
        const auto& var = this->m_stor.as<index_variable>().var;
        if(!var->is_initialized()) {
          ASTERIA_THROW("attempt to modify an uninitialized variable");
        }
        if(var->is_immutable()) {
          ASTERIA_THROW("attempt to modify an immutable variable `$1`", var->get_value());
        }
        return var->open_value();
      }
    case index_tail_call: {
        ASTERIA_THROW("tail call wrapper not dereferenceable");
      }
    default:
      ASTERIA_TERMINATE("invalid reference root type enumeration (index `$1`)", this->index());
    }
  }

Variable_Callback& Reference_Root::enumerate_variables(Variable_Callback& callback) const
  {
    switch(this->index()) {
    case index_void: {
        return callback;
      }
    case index_constant: {
        return this->m_stor.as<index_constant>().val.enumerate_variables(callback);
      }
    case index_temporary: {
        return this->m_stor.as<index_temporary>().val.enumerate_variables(callback);
      }
    case index_variable: {
        const auto& var = this->m_stor.as<index_variable>().var;
        if(!callback.process(var)) {
          return callback;
        }
        return var->enumerate_variables(callback);
      }
    case index_tail_call: {
        return ::rocket::static_pointer_cast<Tail_Call_Arguments>(
                              this->m_stor.as<index_tail_call>().tca)->enumerate_variables(callback);
      }
    default:
      ASTERIA_TERMINATE("invalid reference root type enumeration (index `$1`)", this->index());
    }
  }

}  // namespace Asteria
