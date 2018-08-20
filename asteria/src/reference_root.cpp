// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference_root.hpp"

namespace Asteria
{

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

}
