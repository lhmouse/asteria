// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "local_variable.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Local_variable::~Local_variable() = default;

void Local_variable::do_throw_immutable_local_variable() const {
	ASTERIA_THROW_RUNTIME_ERROR("Immutable local variable: ", m_variable_opt);
}

}
