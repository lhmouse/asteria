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

std::shared_ptr<const Variable> Reference::load() const {
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_direct_reference: {
		const auto &direct_reference = boost::get<Direct_reference>(m_variant);
		if(!direct_reference.variable_opt){
			return nullptr;
		}
		return direct_reference.variable_opt; }
	case type_array_element: {
		const auto &array_element = boost::get<Array_element>(m_variant);
		if(!array_element.variable_opt){
			ASTERIA_THROW_RUNTIME_ERROR("Indirection through a null reference");
		}
		const auto array = array_element.variable_opt->try_get<Array>();
		if(!array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integers, while the operand has type `", get_variable_type_name(array_element.variable_opt), "`");
		}
		auto index = static_cast<std::uint64_t>(array_element.index_bidirectional);
		if(array_element.index_bidirectional < 0){
			index += array->size();
		}
		if(index >= array->size()){
			ASTERIA_DEBUG_LOG("Array index out of range: index = ", static_cast<std::int64_t>(index), ", size = ", array->size());
			return nullptr;
		}
		auto it = std::next(array->begin(), static_cast<std::ptrdiff_t>(index));
		return it->share(); }
	case type_object_member: {
		const auto &object_member = boost::get<Object_member>(m_variant);
		if(!object_member.variable_opt){
			ASTERIA_THROW_RUNTIME_ERROR("Indirection through a null reference");
		}
		const auto object = object_member.variable_opt->try_get<Object>();
		if(!object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by strings, while the operand has type `", get_variable_type_name(object_member.variable_opt), "`");
		}
		auto it = object->find(object_member.key);
		if(it == object->end()){
			ASTERIA_DEBUG_LOG("Object member not found: key = ", object_member.key);
			return nullptr;
		}
		return it->second.share(); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}
Value_ptr<Variable> &Reference::store(Value_ptr<Variable> &&new_value){
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_direct_reference: {
		const auto &direct_reference = boost::get<Direct_reference>(m_variant);
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to write to a temporary reference of type `", get_variable_type_name(direct_reference.variable_opt), "`");
		/*return ???;*/ }
	case type_array_element: {
		const auto &array_element = boost::get<Array_element>(m_variant);
		if(!array_element.variable_opt){
			ASTERIA_THROW_RUNTIME_ERROR("Indirection through a null reference");
		}
		const auto array = array_element.variable_opt->try_get<Array>();
		if(!array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integers, while the operand has type `", get_variable_type_name(array_element.variable_opt), "`");
		}
		if(is_immutable()){
			ASTERIA_THROW_RUNTIME_ERROR("Attempting to modify a variable through an immutable reference");
		}
		auto index = static_cast<std::uint64_t>(array_element.index_bidirectional);
		if(array_element.index_bidirectional < 0){
			index += array->size();
		}
		if(index >= array->size()){
			if(index >= array->max_size()){
				ASTERIA_THROW_RUNTIME_ERROR("Array index is too large, got `", index, "`");
			}
			array->insert(array->end(), static_cast<std::size_t>(index - array->size() + 1), nullptr);
		}
		auto it = std::next(array->begin(), static_cast<std::ptrdiff_t>(index));
		*it = std::move(new_value);
		return *it; }
	case type_object_member: {
		const auto &object_member = boost::get<Object_member>(m_variant);
		if(!object_member.variable_opt){
			ASTERIA_THROW_RUNTIME_ERROR("Indirection through a null reference");
		}
		const auto object = object_member.variable_opt->try_get<Object>();
		if(!object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by strings, while the operand has type `", get_variable_type_name(object_member.variable_opt), "`");
		}
		if(is_immutable()){
			ASTERIA_THROW_RUNTIME_ERROR("Attempting to modify a variable through an immutable reference");
		}
		auto it = object->find(object_member.key);
		if(it == object->end()){
			it = object->emplace(object_member.key, nullptr).first;
		}
		it->second = std::move(new_value);
		return it->second; }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

}
