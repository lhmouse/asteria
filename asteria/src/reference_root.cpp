// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference_root.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference_root::Reference_root(const Reference_root &) noexcept
  = default;
Reference_root & Reference_root::operator=(const Reference_root &) noexcept
  = default;
Reference_root::Reference_root(Reference_root &&) noexcept
  = default;
Reference_root & Reference_root::operator=(Reference_root &&) noexcept
  = default;
Reference_root::~Reference_root()
  = default;

const Value & dereference_root_readonly_partial(const Reference_root &root) noexcept
  {
    switch(root.index()) {
    case Reference_root::index_constant:
      {
        const auto &cand = root.as<Reference_root::S_constant>();
        return cand.src;
      }
    case Reference_root::index_temp_value:
      {
        const auto &cand = root.as<Reference_root::S_temp_value>();
        return cand.value;
      }
    case Reference_root::index_variable:
      {
        const auto &cand = root.as<Reference_root::S_variable>();
        return cand.var->get_value();
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", root.index(), "` has been encountered.");
    }
  }
Value & dereference_root_mutable_partial(const Reference_root &root)
  {
    switch(root.index()) {
    case Reference_root::index_constant:
      {
        const auto &cand = root.as<Reference_root::S_constant>();
        ASTERIA_THROW_RUNTIME_ERROR("The constant `", cand.src, "` cannot be modified.");
      }
    case Reference_root::index_temp_value:
      {
        const auto &cand = root.as<Reference_root::S_temp_value>();
        ASTERIA_THROW_RUNTIME_ERROR("The temporary value `", cand.value, "` cannot be modified.");
      }
    case Reference_root::index_variable:
      {
        const auto &cand = root.as<Reference_root::S_variable>();
        if(cand.var->is_immutable()) {
          ASTERIA_THROW_RUNTIME_ERROR("The variable having value `", cand.var->get_value(), "` is immutable and cannot be modified.");
        }
        return cand.var->get_value();
      }
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", root.index(), "` has been encountered.");
    }
  }

}
