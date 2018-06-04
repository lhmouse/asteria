// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "stored_reference.hpp"

namespace Asteria {

Stored_reference::Stored_reference(Stored_reference &&) noexcept = default;
Stored_reference & Stored_reference::operator=(Stored_reference &&) noexcept = default;
Stored_reference::~Stored_reference() = default;

void set_reference(Vp<Reference> &reference_out, Stored_reference &&value_opt){
	const auto value = value_opt.get_opt();
	if(value == nullptr){
		return reference_out.reset();
	} else if(reference_out == nullptr){
		auto sp = std::make_shared<Reference>(std::move(*value));
		return reference_out.reset(std::move(sp));
	} else {
		return reference_out->set(std::move(*value));
	}
}

}
