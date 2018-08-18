// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference_modifier.hpp"

namespace Asteria
{

Reference_modifier::Reference_modifier(Reference_modifier &&) noexcept = default;
Reference_modifier & Reference_modifier::operator=(Reference_modifier &&) noexcept = default;
Reference_modifier::~Reference_modifier() = default;

}
