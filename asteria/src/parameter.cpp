// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "parameter.hpp"
#include "value.hpp"
#include "stored_reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Parameter::Parameter(const Parameter &) noexcept = default;
Parameter & Parameter::operator=(const Parameter &) noexcept = default;
Parameter::Parameter(Parameter &&) noexcept = default;
Parameter & Parameter::operator=(Parameter &&) noexcept = default;
Parameter::~Parameter() = default;

void prepare_function_arguments(Vector<Vp<Reference>> &args_inout, const Vector<Parameter> &params){
	const auto delta_size = static_cast<std::ptrdiff_t>(args_inout.size() - params.size());
	if(delta_size < 0){
		args_inout.insert(args_inout.end(), rocket::fill_iterator<Nullptr>(delta_size), rocket::fill_iterator<Nullptr>(0));
	}
	for(std::size_t i = 0; i < params.size(); ++i){
		const auto &param = params.at(i);
		auto &arg = args_inout.at(i);
		if(arg){
			continue;
		}
		const auto &def_arg = param.get_default_argument_opt();
		if(!def_arg){
			continue;
		}
		ASTERIA_DEBUG_LOG("Setting default argument: i = ", i, ", def_arg = ", def_arg);
		Reference::S_constant ref_k = { def_arg };
		set_reference(arg, std::move(ref_k));
	}
}

}
