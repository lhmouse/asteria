// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_root.hpp"
#include "abstract_variable_callback.hpp"
#include "reference.hpp"
#include "../utilities.hpp"

namespace Asteria {

const Value& Reference_Root::dereference_const() const
  {
    switch(this->index()) {
    case index_null:
      {
        return Value::null();
      }
    case index_constant:
      {
        return this->m_stor.as<index_constant>().source;
      }
    case index_temporary:
      {
        return this->m_stor.as<index_temporary>().value;
      }
    case index_variable:
      {
        const auto& var = this->m_stor.as<index_variable>().var_opt;
        if(!var) {
          return Value::null();
        }
        return var->get_value();
      }
    case index_tail_call:
      {
        ASTERIA_THROW_RUNTIME_ERROR("Tail call wrappers cannot be dereferenced directly.");
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->index(), "` has been encountered.");
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
        ASTERIA_THROW_RUNTIME_ERROR("The constant `", this->m_stor.as<index_constant>().source, "` cannot be modified.");
      }
    case index_temporary:
      {
        ASTERIA_THROW_RUNTIME_ERROR("The temporary value `", this->m_stor.as<index_temporary>().value, "` cannot be modified.");
      }
    case index_variable:
      {
        const auto& var = this->m_stor.as<index_variable>().var_opt;
        if(!var) {
          ASTERIA_THROW_RUNTIME_ERROR("The reference cannot be written after being moved. This is likely a bug. Please report.");
        }
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
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->index(), "` has been encountered.");
    }
  }

Rcptr<Abstract_Function> Reference_Root::unpack_tail_call_opt(Cow_Bivector<Source_Location, Cow_String>& backtrace, Reference& self, Cow_Vector<Reference>& args)
  {
    if(this->index() != index_tail_call) {
      // This is not a tail call wrapper.
      return nullptr;
    }
    auto& altr = this->m_stor.as<index_tail_call>();
    // Unpack a backtrace frame.
    backtrace.emplace_back(rocket::move(altr.sloc), rocket::move(altr.func));
    // Unpack arguments.
    self = rocket::move(altr.args.mut_back());
    altr.args.pop_back();
    args = rocket::move(altr.args);
    // Return a pointer to the wrapped function.
    return altr.target;
  }

void Reference_Root::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    switch(this->index()) {
    case index_null:
      {
        return;
      }
    case index_constant:
      {
        return this->m_stor.as<index_constant>().source.enumerate_variables(callback);
      }
    case index_temporary:
      {
        return this->m_stor.as<index_temporary>().value.enumerate_variables(callback);
      }
    case index_variable:
      {
        const auto& var = this->m_stor.as<index_variable>().var_opt;
        if(!var || !callback(var)) {
          return;
        }
        // Descend into this variable recursively when the callback returns `true`.
        return var->enumerate_variables(callback);
      }
    case index_tail_call:
      {
        return rocket::for_each(this->m_stor.as<index_tail_call>().args, [&](const Reference& arg) { arg.enumerate_variables(callback);  });
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->index(), "` has been encountered.");
    }
  }

}  // namespace Asteria
