// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "initializer.hpp"
#include "variable.hpp"
#include "expression.hpp"
#include "reference.hpp"
#include "misc.hpp"

namespace Asteria {

Initializer::~Initializer(){
	//
}

Value_ptr<Variable> Initializer::create_variable(const std::shared_ptr<Scope> &scope) const {
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_bracketed_init_list: {
		const auto &bracketed_init_list = boost::get<Bracketed_init_list>(m_variant);
		Array array;
		for(const auto &child : bracketed_init_list.initializers){
			array.emplace_back(child->create_variable(scope));
		}
		return make_value<Variable>(std::move(array)); }
	case type_braced_init_list: {
		const auto &braced_init_list = boost::get<Braced_init_list>(m_variant);
		Object object;
		for(const auto &pair : braced_init_list.key_values){
			object.emplace(pair.first, pair.second->create_variable(scope));
		}
		return make_value<Variable>(std::move(object)); }
	case type_assignment: {
		const auto &assignment = boost::get<Assignment>(m_variant);
		const auto result = assignment.expression->evaluate(scope).load();
		if(!result){
			return nullptr;
		}
		return make_value<Variable>(*result); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

}
