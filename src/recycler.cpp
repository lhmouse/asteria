// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "recycler.hpp"
#include "variable.hpp"
#include "nullable_value.hpp"
#include "misc.hpp"

namespace Asteria {

Recycler::~Recycler(){
	clear_variables();
}

template<typename ValueT>
void Recycler::do_set_variable_explicit(Value_ptr<Variable> &variable, ValueT &&value){
	if(!variable){
		auto ref_ptr = std::make_shared<Variable>(std::forward<ValueT>(value));
		m_weak_variables.emplace_back(ref_ptr);
		variable = Value_ptr<Variable>(std::move(ref_ptr));
	} else {
		*variable = std::forward<ValueT>(value);
	}
}

Value_ptr<Variable> Recycler::create_variable_opt(Nullable_value &&value_opt){
	Value_ptr<Variable> variable;
	set_variable(variable, std::move(value_opt));
	return variable;
}
void Recycler::set_variable(Value_ptr<Variable> &variable, Nullable_value &&value_opt){
	const auto type = value_opt.get_type();
	switch(type){
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
	case -2: { // Variable
#pragma GCC diagnostic pop
		auto &value = value_opt.get<Variable>();
		return do_set_variable_explicit(variable, std::move(value)); }
	case Variable::type_null: {
		variable = nullptr;
		break; }
	case Variable::type_boolean: {
		auto &value = value_opt.get<Boolean>();
		return do_set_variable_explicit(variable, std::move(value)); }
	case Variable::type_integer: {
		auto &value = value_opt.get<Integer>();
		return do_set_variable_explicit(variable, std::move(value)); }
	case Variable::type_double: {
		auto &value = value_opt.get<Double>();
		return do_set_variable_explicit(variable, std::move(value)); }
	case Variable::type_string: {
		auto &value = value_opt.get<String>();
		return do_set_variable_explicit(variable, std::move(value)); }
	case Variable::type_opaque: {
		auto &value = value_opt.get<Opaque>();
		return do_set_variable_explicit(variable, std::move(value)); }
	case Variable::type_array: {
		auto &value = value_opt.get<Array>();
		return do_set_variable_explicit(variable, std::move(value)); }
	case Variable::type_object: {
		auto &value = value_opt.get<Object>();
		return do_set_variable_explicit(variable, std::move(value)); }
	case Variable::type_function: {
		auto &value = value_opt.get<Function>();
		return do_set_variable_explicit(variable, std::move(value)); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}
void Recycler::clear_variables() noexcept {
	for(auto &weak_variable : m_weak_variables){
		dispose_variable_recursive(weak_variable.lock().get());
	}
	m_weak_variables.clear();
}

}
