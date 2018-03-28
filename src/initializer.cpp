// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "initializer.hpp"
#include "variable.hpp"
#include "nullable_value.hpp"
#include "expression.hpp"
#include "recycler.hpp"
#include "reference.hpp"
#include "misc.hpp"

namespace Asteria {

Initializer::~Initializer(){
	//
}

Value_ptr<Variable> Initializer::evaluate_opt(const Shared_ptr<Recycler> &recycler, const Shared_ptr<Scope> &scope) const {
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_bracketed_init_list: {
		const auto &bracketed_init_list = boost::get<Bracketed_init_list>(m_variant);
		Array array;
		for(const auto &child : bracketed_init_list.initializers){
			array.emplace_back(child->evaluate_opt(recycler, scope));
		}
		return recycler->create_variable_opt(std::move(array)); }
	case type_braced_init_list: {
		const auto &braced_init_list = boost::get<Braced_init_list>(m_variant);
		Object object;
		for(const auto &pair : braced_init_list.key_values){
			object.emplace(pair.first, pair.second->evaluate_opt(recycler, scope));
		}
		return recycler->create_variable_opt(std::move(object)); }
	case type_assignment: {
		const auto &assignment = boost::get<Assignment>(m_variant);
		auto result = assignment.expression->evaluate(recycler, scope);
		return result.extract_opt(recycler); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

}
