// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "local_variable.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Local_variable::~Local_variable() = default;

void Local_variable::do_throw_local_constant() const {
	ASTERIA_THROW_RUNTIME_ERROR("The local constant `", m_variable_opt, "` cannot be modified.");
}

}
