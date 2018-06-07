// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "stored_value.hpp"
#include "recycler.hpp"

namespace Asteria {

Stored_value::Stored_value(Stored_value &&) noexcept = default;
Stored_value & Stored_value::operator=(Stored_value &&) noexcept = default;
Stored_value::~Stored_value() = default;

void set_value(Vp<Value> &value_out, Spr<Recycler> recycler_inout, Stored_value &&value_opt){
	const auto value = value_opt.get_opt();
	if(value == nullptr){
		return value_out.reset();
	} else if(value_out == nullptr){
		auto sp = std::make_shared<Value>(recycler_inout, std::move(*value));
		recycler_inout->adopt_value(sp);
		return value_out.reset(std::move(sp));
	} else {
		return value_out->set(std::move(*value));
	}
}

}
