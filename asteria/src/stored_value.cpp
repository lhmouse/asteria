// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "stored_value.hpp"
#include "recycler.hpp"

namespace Asteria {

Stored_value::Stored_value(Stored_value &&) noexcept = default;
Stored_value & Stored_value::operator=(Stored_value &&) noexcept = default;
Stored_value::~Stored_value() = default;

void set_variable(Xptr<Variable> &variable_out, Spcref<Recycler> recycler, Stored_value &&value_opt){
	const auto value = value_opt.get_opt();
	if(value == nullptr){
		return variable_out.reset();
	} else if(variable_out == nullptr){
		auto sptr = std::make_shared<Variable>(recycler, std::move(*value));
		recycler->adopt_variable(sptr);
		return variable_out.reset(std::move(sptr));
	} else {
		return variable_out->set(std::move(*value));
	}
}

}
