// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "initializer.hpp"

namespace Asteria {

Initializer::Initializer(Initializer &&) = default;
Initializer &Initializer::operator=(Initializer &&) = default;
Initializer::~Initializer() = default;

Initializer::Type get_initializer_type(Spref<const Initializer> initializer_opt) noexcept {
	return initializer_opt ? initializer_opt->get_type() : Initializer::type_null;
}

}
