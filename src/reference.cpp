// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "variable.hpp"
#include "misc.hpp"

namespace Asteria {

Reference::~Reference(){
	//
}

std::shared_ptr<Variable> Reference::load_opt() const {
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_rvalue_generic: {
		const auto &params = boost::get<Rvalue_generic>(m_variant);
		return params.value_opt; }
	case type_lvalue_generic: {
		const auto &params = boost::get<Lvalue_generic>(m_variant);
		auto &variable = *(params.variable_ptr);
		return variable.share(); }
	case type_lvalue_array_element: {
		const auto &params = boost::get<Lvalue_array_element>(m_variant);
		const auto array = params.variable->try_get<Array>();
		if(!array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integers, while the operand has type `", get_variable_type_name(params.variable), "`");
		}
		auto index = static_cast<std::uint64_t>(params.index_bidirectional);
		if(params.index_bidirectional < 0){
			index += array->size();
		}
		if(index >= array->size()){
			ASTERIA_DEBUG_LOG("Array index out of range: index = ", static_cast<std::int64_t>(index), ", size = ", array->size());
			return nullptr;
		}
		auto &variable = (*array)[index];
		return variable.share(); }
	case type_lvalue_object_member: {
		const auto &params = boost::get<Lvalue_object_member>(m_variant);
		const auto object = params.variable->try_get<Object>();
		if(!object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by strings, while the operand has type `", get_variable_type_name(params.variable), "`");
		}
		auto it = object->find(params.key);
		if(it == object->end()){
			ASTERIA_DEBUG_LOG("Object member not found: key = ", params.key);
			return nullptr;
		}
		auto &variable = it->second;
		return variable.share(); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}
void Reference::store_opt(boost::optional<Variable> &&value_new_opt) const {
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_rvalue_generic: {
		const auto &params = boost::get<Rvalue_generic>(m_variant);
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to modify a temporary variable having type ", get_variable_type_name(params.value_opt), "`");
		/*return ???;*/ }
	case type_lvalue_generic: {
		const auto &params = boost::get<Lvalue_generic>(m_variant);
		auto &variable = *(params.variable_ptr);
		return set_variable(variable, std::move(value_new_opt)); }
	case type_lvalue_array_element: {
		const auto &params = boost::get<Lvalue_array_element>(m_variant);
		const auto array = params.variable->try_get<Array>();
		if(!array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integers, while the operand has type `", get_variable_type_name(params.variable), "`");
		}
		auto index = static_cast<std::uint64_t>(params.index_bidirectional);
		if(params.index_bidirectional < 0){
			index += array->size();
		}
		if(index >= array->size()){
			if(index >= array->max_size()){
				ASTERIA_THROW_RUNTIME_ERROR("Array index `", index, "` is too large");
			}
			array->insert(array->end(), static_cast<std::size_t>(index - array->size() + 1), nullptr);
		}
		auto &variable = (*array)[index];
		return set_variable(variable, std::move(value_new_opt)); }
	case type_lvalue_object_member: {
		const auto &params = boost::get<Lvalue_object_member>(m_variant);
		const auto object = params.variable->try_get<Object>();
		if(!object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by strings, while the operand has type `", get_variable_type_name(params.variable), "`");
		}
		auto it = object->find(params.key);
		if(it == object->end()){
			it = object->emplace(params.key, nullptr).first;
		}
		auto &variable = it->second;
		return set_variable(variable, std::move(value_new_opt)); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}
Value_ptr<Variable> Reference::extract_result_opt(){
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_rvalue_generic: {
		auto &params = boost::get<Rvalue_generic>(m_variant);
		return Value_ptr<Variable>(std::move(params.value_opt)); }
	case type_lvalue_generic:
	case type_lvalue_array_element:
	case type_lvalue_object_member: {
		const auto result = load_opt();
		return result ? make_value<Variable>(*result) : nullptr; }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

}

template class std::function<Asteria::Reference (boost::container::vector<Asteria::Reference> &&)>;
