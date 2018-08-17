// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference_root.hpp"
#include "utilities.hpp"

namespace Asteria
{

Reference_root::Reference_root(Reference_root &&) noexcept = default;
Reference_root & Reference_root::operator=(Reference_root &&) noexcept = default;
Reference_root::~Reference_root() = default;

const char * get_reference_category_name(Reference_root::Type type) noexcept
  {
    switch(type) {
    case Reference_root::type_constant:
      return "constant";
    case Reference_root::type_rvalue:
      return "rvalue";
    case Reference_root::type_lvalue:
      return "lvalue";
    default:
      ASTERIA_TERMINATE("An unknown reference root type enumeration `", type, "` is encountered. This is probably a bug. Please report.");
    }
  }
const char * get_reference_category_name_of(const Reference_root &root) noexcept
  {
    return get_reference_category_name(root.which());
  }

}
