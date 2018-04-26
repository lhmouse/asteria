// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "function_parameter.hpp"
#include "reference.hpp"
#include "stored_reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Function_parameter::Function_parameter(Function_parameter &&) noexcept = default;
Function_parameter &Function_parameter::operator=(Function_parameter &&) = default;
Function_parameter::~Function_parameter() = default;

void prepare_function_arguments(Xptr_vector<Reference> &arguments_inout, const std::vector<Function_parameter> &parameters){
	const auto delta_size = static_cast<std::ptrdiff_t>(arguments_inout.size() - parameters.size());
	if(delta_size < 0){
		arguments_inout.insert(arguments_inout.end(), rocket::nullptr_filler(delta_size), rocket::nullptr_filler(0));
	}
	for(std::size_t i = 0; i < parameters.size(); ++i){
		const auto &param = parameters.at(i);
		auto &arg = arguments_inout.at(i);
		if(arg){
			continue;
		}
		const auto &default_arg = param.get_default_argument_opt();
		if(!default_arg){
			continue;
		}
		ASTERIA_DEBUG_LOG("Setting default argument: i = ", i, ", default_arg = ", default_arg);
		Reference::S_constant ref_c = { default_arg };
		set_reference(arg, std::move(ref_c));
	}
}

}
