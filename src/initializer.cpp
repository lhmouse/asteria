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

Value_ptr<Variable> Initializer::evaluate_opt(const Shared_ptr<Recycler> &recycler, const Shared_ptr<Scope> &scope) const {
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_bracketed_init_list: {
		const auto &params = boost::get<Bracketed_init_list>(m_variant);
		Array array;
		array.reserve(params.initializers.size());
		for(const auto &child : params.initializers){
			array.emplace_back(child->evaluate_opt(recycler, scope));
		}
		return recycler->create_variable_opt(std::move(array)); }
	case type_braced_init_list: {
		const auto &params = boost::get<Braced_init_list>(m_variant);
		Object object;
		object.reserve(params.key_values.size());
		for(const auto &pair : params.key_values){
			object.emplace(pair.first, pair.second->evaluate_opt(recycler, scope));
		}
		return recycler->create_variable_opt(std::move(object)); }
	case type_expression_init: {
		const auto &params = boost::get<Expression_init>(m_variant);
		auto result = params.expression->evaluate(recycler, scope);
		return result.extract_opt(recycler); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

}
