// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "value.hpp"
#include "utilities.hpp"

namespace Asteria {

Variable::~Variable() = default;

void Variable::do_throw_immutable() const {
	ASTERIA_THROW_RUNTIME_ERROR("This variable having value `", m_value_opt, "` is immutable.");
}

}
