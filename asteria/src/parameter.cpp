// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "parameter.hpp"
#include "value.hpp"
#include "stored_reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Parameter::Parameter(Parameter &&) noexcept = default;
Parameter & Parameter::operator=(Parameter &&) noexcept = default;
Parameter::~Parameter() = default;

void prepare_function_arguments(Vector<Vp<Reference>> &arguments_inout, const Vector<Sp<const Parameter>> &params_opt){
	const auto delta_size = static_cast<std::ptrdiff_t>(arguments_inout.size() - params_opt.size());
	if(delta_size < 0){
		arguments_inout.insert(arguments_inout.end(), rocket::fill_iterator<Nullptr>(delta_size), rocket::fill_iterator<Nullptr>(0));
	}
	for(std::size_t i = 0; i < params_opt.size(); ++i){
		const auto &param = params_opt.at(i);
		auto &arg = arguments_inout.at(i);
		if(arg){
			continue;
		}
		const auto &default_arg = param->get_default_argument_opt();
		if(!default_arg){
			continue;
		}
		ASTERIA_DEBUG_LOG("Setting default argument: i = ", i, ", default_arg = ", default_arg);
		Reference::S_constant ref_k = { default_arg };
		set_reference(arg, std::move(ref_k));
	}
}

}
