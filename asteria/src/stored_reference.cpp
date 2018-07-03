// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "stored_reference.hpp"

namespace Asteria {

Stored_reference::Stored_reference(Stored_reference &&) noexcept = default;
Stored_reference & Stored_reference::operator=(Stored_reference &&) noexcept = default;
Stored_reference::~Stored_reference() = default;

void set_reference(Vp<Reference> &ref_out, Stored_reference &&value_opt){
	const auto value = value_opt.get_opt();
	if(value == nullptr){
		ref_out.reset();
	} else if(ref_out == nullptr){
		auto sp = std::make_shared<Reference>(std::move(*value));
		ref_out.reset(std::move(sp));
	} else {
		ref_out->set(std::move(*value));
	}
}

}
