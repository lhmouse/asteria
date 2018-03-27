// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "variable.hpp"
#include "scope.hpp"
#include "misc.hpp"

namespace Asteria {

Reference::~Reference(){
	//
}

std::shared_ptr<const Variable> Reference::load_opt() const {
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_scoped_variable: {
		const auto &params = boost::get<Scoped_variable>(m_variant);
		const auto pptr = params.scope->try_get_local_variable_recursive(params.key);
		if(!pptr){
			ASTERIA_THROW_RUNTIME_ERROR("Undeclared variable `", params.key, "`");
		}
		return pptr->share(); }
	case type_array_element: {
		const auto &params = boost::get<Array_element>(m_variant);
		if(!params.variable_opt){
			ASTERIA_THROW_RUNTIME_ERROR("Indirection through a null reference");
		}
		const auto array = params.variable_opt->try_get<Array>();
		if(!array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integers, while the operand has type `", get_variable_type_name(params.variable_opt), "`");
		}
		auto index = static_cast<std::uint64_t>(params.index_bidirectional);
		if(params.index_bidirectional < 0){
			index += array->size();
		}
		if(index >= array->size()){
			ASTERIA_DEBUG_LOG("Array index out of range: index = ", static_cast<std::int64_t>(index), ", size = ", array->size());
			return nullptr;
		}
		return (*array)[index].share(); }
	case type_object_member: {
		const auto &params = boost::get<Object_member>(m_variant);
		if(!params.variable_opt){
			ASTERIA_THROW_RUNTIME_ERROR("Indirection through a null reference");
		}
		const auto object = params.variable_opt->try_get<Object>();
		if(!object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by strings, while the operand has type `", get_variable_type_name(params.variable_opt), "`");
		}
		const auto it = object->find(params.key);
		if(it == object->end()){
			ASTERIA_DEBUG_LOG("Object member not found: key = ", params.key);
			return nullptr;
		}
		return it->second.share(); }
	case type_pure_rvalue: {
		const auto &params = boost::get<Pure_rvalue>(m_variant);
		return params.variable_opt; }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}
Value_ptr<Variable> &Reference::store(Value_ptr<Variable> &&new_value_opt){
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_scoped_variable: {
		const auto &params = boost::get<Scoped_variable>(m_variant);
		const auto pptr = params.scope->try_get_local_variable_recursive(params.key);
		if(!pptr){
			ASTERIA_THROW_RUNTIME_ERROR("Undeclared variable `", params.key, "`");
		}
		return *pptr = std::move(new_value_opt); }
	case type_array_element: {
		const auto &params = boost::get<Array_element>(m_variant);
		if(!params.variable_opt){
			ASTERIA_THROW_RUNTIME_ERROR("Indirection through a null reference");
		}
		const auto array = params.variable_opt->try_get<Array>();
		if(!array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integers, while the operand has type `", get_variable_type_name(params.variable_opt), "`");
		}
		auto index = static_cast<std::uint64_t>(params.index_bidirectional);
		if(params.index_bidirectional < 0){
			index += array->size();
		}
		if(index >= array->size()){
			if(index >= array->max_size()){
				ASTERIA_THROW_RUNTIME_ERROR("Array index is too large, got `", index, "`");
			}
			array->insert(array->end(), static_cast<std::size_t>(index - array->size() + 1), nullptr);
		}
		return (*array)[index] = std::move(new_value_opt); }
	case type_object_member: {
		const auto &params = boost::get<Object_member>(m_variant);
		if(!params.variable_opt){
			ASTERIA_THROW_RUNTIME_ERROR("Indirection through a null reference");
		}
		const auto object = params.variable_opt->try_get<Object>();
		if(!object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by strings, while the operand has type `", get_variable_type_name(params.variable_opt), "`");
		}
		return (*object)[params.key] = std::move(new_value_opt); }
	case type_pure_rvalue: {
		const auto &params = boost::get<Pure_rvalue>(m_variant);
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to modify a temporary variable having type ", get_variable_type_name(params.variable_opt), "`");
		/*return params.variable_opt;*/ }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

Value_ptr<Variable> Reference::steal_opt(){
	const auto prvalue_params = boost::get<Pure_rvalue>(&m_variant);
	if(prvalue_params){
		// Move the rvalue.
		return Value_ptr<Variable>(std::move(prvalue_params->variable_opt));
	} else {
		// Copy the lvalue.
		const auto ptr = load_opt();
		if(!ptr){
			return nullptr;
		}
		return make_value<Variable>(*ptr);
	}
}

}
