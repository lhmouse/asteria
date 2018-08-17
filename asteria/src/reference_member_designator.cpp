// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference_member_designator.hpp"

namespace Asteria
{

Reference_member_designator::Reference_member_designator(Reference_member_designator &&) noexcept = default;
Reference_member_designator & Reference_member_designator::operator=(Reference_member_designator &&) noexcept = default;
Reference_member_designator::~Reference_member_designator() = default;

}
