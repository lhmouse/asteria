// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_root.hpp"
#include "abstract_variable_callback.hpp"
#include "global_context.hpp"
#include "../utilities.hpp"

namespace Asteria {

Reference_root::~Reference_root()
  {
  }

const Value & Reference_root::dereference_const() const
  {
    switch(Index(this->m_stor.index())) {
      case index_null: {
        return Value::get_null();
      }
      case index_constant: {
        const auto &alt = this->m_stor.as<S_constant>();
        return alt.source;
      }
      case index_temporary: {
        const auto &alt = this->m_stor.as<S_temporary>();
        return alt.value;
      }
      case index_variable: {
        const auto &alt = this->m_stor.as<S_variable>();
        return alt.var->get_value();
      }
      default: {
        ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

Value & Reference_root::dereference_mutable() const
  {
    switch(Index(this->m_stor.index())) {
      case index_null: {
        ASTERIA_THROW_RUNTIME_ERROR("The null reference cannot be modified.");
      }
      case index_constant: {
        const auto &alt = this->m_stor.as<S_constant>();
        ASTERIA_THROW_RUNTIME_ERROR("The constant `", alt.source, "` cannot be modified.");
      }
      case index_temporary: {
        const auto &alt = this->m_stor.as<S_temporary>();
        ASTERIA_THROW_RUNTIME_ERROR("The temporary value `", alt.value, "` cannot be modified.");
      }
      case index_variable: {
        const auto &alt = this->m_stor.as<S_variable>();
        return alt.var->get_mutable_value();
      }
      default: {
        ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

void Reference_root::enumerate_variables(const Abstract_variable_callback &callback) const
  {
    switch(Index(this->m_stor.index())) {
      case index_null: {
        return;
      }
      case index_constant: {
        const auto &alt = this->m_stor.as<S_constant>();
        alt.source.enumerate_variables(callback);
        return;
      }
      case index_temporary: {
        const auto &alt = this->m_stor.as<S_temporary>();
        alt.value.enumerate_variables(callback);
        return;
      }
      case index_variable: {
        const auto &alt = this->m_stor.as<S_variable>();
        if(callback.accept(alt.var)) {
          // Descend into this variable recursively when the callback returns `true`.
          alt.var->enumerate_variables(callback);
        }
        return;
      }
      default: {
        ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

void Reference_root::dispose_variable(Global_context &global) noexcept
  {
    switch(Index(this->m_stor.index())) {
      case index_null: {
        return;
      }
      case index_constant:
      case index_temporary: {
        this->m_stor = S_null();
        return;
      }
      case index_variable: {
        const auto &alt = this->m_stor.as<S_variable>();
        if((alt.var->use_count() <= 2) && global.untrack_variable(alt.var)) {
          // Wipe out its contents only if it has been detached successfully.
          ASTERIA_DEBUG_LOG("Disposing variable: ", alt.var->get_value());
          alt.var->reset(D_null(), true);
        }
        this->m_stor = S_null();
        return;
      }
      default: {
        ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

}
