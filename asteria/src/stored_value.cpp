// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "stored_value.hpp"
#include "recycler.hpp"

namespace Asteria {

Stored_value::Stored_value(Stored_value &&) noexcept = default;
Stored_value & Stored_value::operator=(Stored_value &&) noexcept = default;
Stored_value::~Stored_value() = default;

void set_value(Xptr<Value> &value_out, Spparam<Recycler> recycler, Stored_value &&value_opt){
	const auto value = value_opt.get_opt();
	if(value == nullptr){
		return value_out.reset();
	} else if(value_out == nullptr){
		auto sptr = std::make_shared<Value>(recycler, std::move(*value));
		recycler->adopt_value(sptr);
		return value_out.reset(std::move(sptr));
	} else {
		return value_out->set(std::move(*value));
	}
}

}
