// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "slim_function.hpp"
#include "utilities.hpp"

namespace Asteria {

Slim_function::~Slim_function() = default;

D_string Slim_function::describe() const {
	return ASTERIA_FORMAT_STRING("slim function wrapper for '", m_description, "' @ `", reinterpret_cast<void *>(reinterpret_cast<std::intptr_t>(m_target)), "`");
}
void Slim_function::invoke(Vp<Reference> &result_out, Spr<Recycler> recycler_out, Vp<Reference> &&this_opt, Vector<Vp<Reference>> &&args) const {
	return (*m_target)(result_out, recycler_out, std::move(this_opt), std::move(args));
}

}
