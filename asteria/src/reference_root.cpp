// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference_root.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference_root::~Reference_root()
  {
  }

Reference_root::Reference_root(const Reference_root &) noexcept
  = default;
Reference_root & Reference_root::operator=(const Reference_root &) noexcept
  = default;
Reference_root::Reference_root(Reference_root &&) noexcept
  = default;
Reference_root & Reference_root::operator=(Reference_root &&) noexcept
  = default;

const Value & Reference_root::dereference_readonly() const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_constant:
      {
        const auto &alt = this->m_stor.as<Reference_root::S_constant>();
        return alt.src;
      }
    case index_temporary:
      {
        const auto &alt = this->m_stor.as<Reference_root::S_temporary>();
        return alt.value;
      }
    case index_variable:
      {
        const auto &alt = this->m_stor.as<Reference_root::S_variable>();
        return alt.var->get_value();
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

Value & Reference_root::dereference_mutable() const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_constant:
      {
        const auto &alt = this->m_stor.as<Reference_root::S_constant>();
        ASTERIA_THROW_RUNTIME_ERROR("The constant `", alt.src, "` cannot be modified.");
      }
    case index_temporary:
      {
        const auto &alt = this->m_stor.as<Reference_root::S_temporary>();
        ASTERIA_THROW_RUNTIME_ERROR("The temporary value `", alt.value, "` cannot be modified.");
      }
    case index_variable:
      {
        const auto &alt = this->m_stor.as<Reference_root::S_variable>();
        if(alt.var->is_immutable()) {
          ASTERIA_THROW_RUNTIME_ERROR("The variable having value `", alt.var->get_value(), "` is immutable and cannot be modified.");
        }
        return alt.var->get_value();
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

}
