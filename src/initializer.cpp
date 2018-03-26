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

Value_ptr<Variable> Initializer::create_variable() const {
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_bracketed_init_list: {
		const auto &queue = boost::get<Bracketed_init_list>(m_variant);
		Array array;
		for(const auto &ptr : queue){
			array.emplace_back(ptr->create_variable());
		}
		return make_value<Variable>(std::move(array)); }
	case type_braced_init_list: {
		const auto &map = boost::get<Braced_init_list>(m_variant);
		Object object;
		for(const auto &pair : map){
			object.emplace(pair.first, pair.second->create_variable());
		}
		return make_value<Variable>(std::move(object)); }
	case type_expression: {
		const auto &ptr = boost::get<Expression_ptr>(m_variant);
		return ptr->evaluate().load(); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

}
