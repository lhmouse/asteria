// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "parameter.hpp"
#include "variable.hpp"
#include "stored_reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Parameter::Parameter(Parameter &&) noexcept = default;
Parameter & Parameter::operator=(Parameter &&) noexcept = default;
Parameter::~Parameter() = default;

void prepare_function_arguments(Xptr_vector<Reference> &arguments_inout, Sparg<const Parameter_vector> parameters_opt){
	if(parameters_opt){
		const auto delta_size = static_cast<std::ptrdiff_t>(arguments_inout.size() - parameters_opt->size());
		if(delta_size < 0){
			arguments_inout.insert(arguments_inout.end(), rocket::nullptr_filler(delta_size), rocket::nullptr_filler(0));
		}
		for(std::size_t i = 0; i < parameters_opt->size(); ++i){
			const auto &param = parameters_opt->at(i);
			auto &arg = arguments_inout.at(i);
			if(arg){
				continue;
			}
			const auto &default_arg = param.get_default_argument_opt();
			if(!default_arg){
				continue;
			}
			ASTERIA_DEBUG_LOG("Setting default argument: i = ", i, ", default_arg = ", sptr_fmt(default_arg));
			Reference::S_constant ref_k = { default_arg };
			set_reference(arg, std::move(ref_k));
		}
	}
}

}
