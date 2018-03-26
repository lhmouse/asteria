// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "initializer.hpp"
#include "variable.hpp"
#include "expression.hpp"
#include "misc.hpp"

namespace Asteria {

Initializer::~Initializer(){
	//
}

Value_ptr<Variable> Initializer::evaluate() const {
	const auto category = get_category();

	switch(category){
	case category_bracketed_init_list: {
		const auto &list = boost::get<Expression_list>(m_variant);
		Array array;
		for(const auto &ptr : list){
			array.emplace_back(ptr->evaluate());
		}
		return make_value<Variable>(std::move(array)); }
	case category_braced_init_list: {
		const auto &list = boost::get<Key_value_list>(m_variant);
		Object object;
		for(const auto &pair : list){
			object.emplace(pair.first, pair.second->evaluate());
		}
		return make_value<Variable>(std::move(object)); }
	case category_expression: {
		const auto &ptr = boost::get<Value_ptr<Expression>>(m_variant);
		return ptr->evaluate(); }
	default:
		ASTERIA_DEBUG_LOG("Unknown category enumeration: category = ", category);
		std::terminate();
	}
}

}
