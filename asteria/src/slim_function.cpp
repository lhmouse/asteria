// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "slim_function.hpp"
#include "utilities.hpp"

namespace Asteria {

Slim_function::~Slim_function() = default;

String Slim_function::describe() const {
	return ASTERIA_FORMAT_STRING("slim function wrapper for '", m_description, "' @ `", reinterpret_cast<void *>(reinterpret_cast<std::intptr_t>(m_target)), "`");
}
void Slim_function::invoke(Xptr<Reference> &result_out, Spparam<Recycler> recycler, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt) const {
	return (*m_target)(result_out, recycler, std::move(this_opt), std::move(arguments_opt));
}

}
