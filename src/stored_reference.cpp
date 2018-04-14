// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "stored_reference.hpp"

namespace Asteria {

Stored_reference::Stored_reference(Stored_reference &&) = default;
Stored_reference &Stored_reference::operator=(Stored_reference &&) = default;
Stored_reference::~Stored_reference() = default;

}
