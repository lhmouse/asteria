// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "stored_value.hpp"

namespace Asteria {

Stored_value::Stored_value(const Stored_value &) = default;
Stored_value &Stored_value::operator=(const Stored_value &) = default;
Stored_value::Stored_value(Stored_value &&) = default;
Stored_value &Stored_value::operator=(Stored_value &&) = default;
Stored_value::~Stored_value() = default;

}
