// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "initializer.hpp"
#include "variable.hpp"
#include "stored_value.hpp"
#include "expression.hpp"
#include "reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Initializer::Initializer(Initializer &&) = default;
Initializer &Initializer::operator=(Initializer &&) = default;
Initializer::~Initializer() = default;

Initializer::Type get_initializer_type(Spref<const Initializer> initializer_opt) noexcept {
	return initializer_opt ? initializer_opt->get_type() : Initializer::type_null;
}

Xptr<Variable> initialize_variable_recursive(Xptr<Variable> &variable_out, Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Initializer> initializer_opt){
	const auto type = get_initializer_type(initializer_opt);
	switch(type){
	case Initializer::type_null: {
		return set_variable(variable_out, recycler, nullptr); }
	case Initializer::type_assignment_init: {
		const auto &params = initializer_opt->get<Initializer::S_assignment_init>();
		auto result = evaluate_expression_recursive_opt(recycler, scope, params.expression);
		return set_variable_using_reference(variable_out, recycler, std::move(result)); }
	case Initializer::type_bracketed_init_list: {
		const auto &params = initializer_opt->get<Initializer::S_bracketed_init_list>();
		D_array array;
		array.reserve(params.initializers.size());
		for(const auto &elem : params.initializers){
			initialize_variable_recursive(variable_out, recycler, scope, elem);
			array.emplace_back(std::move(variable_out));
		}
		return set_variable(variable_out, recycler, std::move(array)); }
	case Initializer::type_braced_init_list: {
		const auto &params = initializer_opt->get<Initializer::S_braced_init_list>();
		D_object object;
		object.reserve(params.key_values.size());
		for(const auto &pair : params.key_values){
			initialize_variable_recursive(variable_out, recycler, scope, pair.second);
			object.emplace(pair.first, std::move(variable_out));
		}
		return set_variable(variable_out, recycler, std::move(object)); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

}
