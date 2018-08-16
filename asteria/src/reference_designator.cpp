// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference_designator.hpp"

namespace Asteria
{

Reference_designator::Reference_designator(Reference_designator &&) noexcept = default;
Reference_designator & Reference_designator::operator=(Reference_designator &&) noexcept = default;
Reference_designator::~Reference_designator() = default;

}
