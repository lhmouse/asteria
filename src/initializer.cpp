// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "initializer.hpp"
#include "variable.hpp"
#include "stored_value.hpp"
#include "expression.hpp"
#include "recycler.hpp"
#include "reference.hpp"
#include "misc.hpp"

namespace Asteria {

Initializer::Initializer(Initializer &&) = default;
Initializer &Initializer::operator=(Initializer &&) = default;
Initializer::~Initializer() = default;

Value_ptr<Variable> Initializer::evaluate_opt(const Shared_ptr<Recycler> &recycler, const Shared_ptr<Scope> &scope) const {
	const auto type = get_type();
	switch(type){
	case type_bracketed_init_list: {
		const auto &params = get<Bracketed_init_list>();
		Array array;
		array.reserve(params.initializers.size());
		for(const auto &child : params.initializers){
			array.emplace_back(child->evaluate_opt(recycler, scope));
		}
		return recycler->create_variable_opt(std::move(array)); }

	case type_braced_init_list: {
		const auto &params = get<Braced_init_list>();
		Object object;
		object.reserve(params.key_values.size());
		for(const auto &pair : params.key_values){
			object.emplace(pair.first, pair.second->evaluate_opt(recycler, scope));
		}
		return recycler->create_variable_opt(std::move(object)); }

	case type_expression_init: {
		const auto &params = get<Expression_init>();
		auto result = params.expression->evaluate(recycler, scope);
		return result.extract_opt(recycler); }

	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

}
