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
			ASTERIA_THROW("Indirection through a null reference");
		}
		const auto array = array_element.variable_opt->try_get<Array>();
		if(!array){
			ASTERIA_THROW("Only arrays can be indexed by integers, while the operand has type `", array_element.variable_opt->get_type_name(), "`");
		}
		auto index = static_cast<std::uint64_t>(array_element.index_bidirectional);
		if(array_element.index_bidirectional < 0){
			index += array->size();
		}
		if(index >= array->size()){
			ASTERIA_DEBUG_LOG("Array index out of range: index = ", static_cast<std::int64_t>(index), ", size = ", array->size());
			return nullptr;
		}
		return array->at(index).share(); }
	case type_object_member: {
		const auto &object_member = boost::get<Object_member>(m_variant);
		if(!object_member.variable_opt){
			ASTERIA_THROW("Indirection through a null reference");
		}
		const auto object = object_member.variable_opt->try_get<Object>();
		if(!object){
			ASTERIA_THROW("Only objects can be indexed by strings, while the operand has type `", object_member.variable_opt->get_type_name(), "`");
		}
		const auto it = object->find(object_member.key);
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
	if(is_immutable()){
		ASTERIA_THROW("Attempting to modify a variable through an immutable reference");
	}
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_direct_reference: {
		const auto &direct_reference = boost::get<Direct_reference>(m_variant);
		ASTERIA_THROW("Attempting to write to a temporary variable of type `", direct_reference.variable_opt->get_type_name(), "`");
		/*return ???;*/ }
	case type_array_element: {
		const auto &array_element = boost::get<Array_element>(m_variant);
		if(!array_element.variable_opt){
			ASTERIA_THROW("Indirection through a null reference");
		}
		const auto array = array_element.variable_opt->try_get<Array>();
		if(!array){
			ASTERIA_THROW("Only arrays can be indexed by integers, while the operand has type `", array_element.variable_opt->get_type_name(), "`");
		}
		auto index = static_cast<std::uint64_t>(array_element.index_bidirectional);
		if(array_element.index_bidirectional < 0){
			index += array->size();
		}
		if(index >= array->max_size()){
			ASTERIA_THROW("Array index is too large, got `", index, "`");
		}
		if(index >= array->size()){
			array->insert(array->end(), static_cast<std::size_t>(index - array->size() + 1), nullptr);
		}
		return array->at(index) = std::move(new_value); }
	case type_object_member: {
		const auto &object_member = boost::get<Object_member>(m_variant);
		if(!object_member.variable_opt){
			ASTERIA_THROW("Indirection through a null reference");
		}
		const auto object = object_member.variable_opt->try_get<Object>();
		if(!object){
			ASTERIA_THROW("Only objects can be indexed by strings, while the operand has type `", object_member.variable_opt->get_type_name(), "`");
		}
		return (*object)[object_member.key] = std::move(new_value); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

Value_ptr<Variable> Reference::copy() const {
	const auto ptr = load();
	if(!ptr){
		return nullptr;
	}
	return make_value<Variable>(*ptr);
}

}
