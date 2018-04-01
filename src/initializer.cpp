// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "initializer.hpp"
#include "stored_value.hpp"
#include "expression.hpp"
#include "reference.hpp"
#include "recycler.hpp"
#include "utilities.hpp"

namespace Asteria {

Initializer::Initializer(Initializer &&) = default;
Initializer &Initializer::operator=(Initializer &&) = default;
Initializer::~Initializer() = default;

Initializer::Type get_initializer_type(Spref<const Initializer> initializer_opt) noexcept {
	return initializer_opt ? initializer_opt->get_type() : Initializer::type_none;
}
void set_variable_using_initializer_recursive(Xptr<Variable> &variable_out_opt, Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Initializer> initializer_opt){
	const auto type = get_initializer_type(initializer_opt);
	switch(type){
	case Initializer::type_none: {
		variable_out_opt = nullptr;
		return; }
	case Initializer::type_assignment_init: {
		const auto &params = initializer_opt->get<Initializer::Assignment_init>();
		auto result = evaluate_expression(recycler, scope, params.expression);
		variable_out_opt = extract_variable_from_reference(std::move(result));
		return; }
	case Initializer::type_bracketed_init_list: {
		const auto &params = initializer_opt->get<Initializer::Bracketed_init_list>();
		Array array;
		array.reserve(params.initializers.size());
		for(const auto &elem : params.initializers){
			set_variable_using_initializer_recursive(variable_out_opt, recycler, scope, elem);
			array.emplace_back(std::move(variable_out_opt));
		}
		return recycler->set_variable(variable_out_opt, std::move(array)); }
	case Initializer::type_braced_init_list: {
		const auto &params = initializer_opt->get<Initializer::Braced_init_list>();
		Object object;
		object.reserve(params.key_values.size());
		for(const auto &pair : params.key_values){
			set_variable_using_initializer_recursive(variable_out_opt, recycler, scope, pair.second);
			object.emplace(pair.first, std::move(variable_out_opt));
		}
		return recycler->set_variable(variable_out_opt, std::move(object)); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

}
