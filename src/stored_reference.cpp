// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "stored_reference.hpp"

namespace Asteria {

Stored_reference::Stored_reference(Stored_reference &&) = default;
Stored_reference &Stored_reference::operator=(Stored_reference &&) = default;
Stored_reference::~Stored_reference() = default;

void set_reference(Xptr<Reference> &reference_out, Stored_reference &&value_opt){
	const auto value = value_opt.get_opt();
	if(value == nullptr){
		return reference_out.reset();
	} else if(reference_out == nullptr){
		auto sptr = std::make_shared<Reference>(std::move(*value));
		return reference_out.reset(std::move(sptr));
	} else {
		return reference_out->set(std::move(*value));
	}
}

}
