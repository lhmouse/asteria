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
	case type_null_reference: {
		return nullptr; }
	case type_direct_reference: {
		const auto &variable = boost::get<Direct_reference>(m_variant);
		return variable; }
	case type_array_element: {
		const auto &pair = boost::get<Array_element>(m_variant);
		const auto array = pair.first->try_get<Array>();
		if(!array){
			ASTERIA_THROW("Only arrays can be indexed by integers, while the operand has type `", pair.first->get_type_name(), "`");
		}
		auto index = static_cast<std::uint64_t>(pair.second);
		if(pair.second < 0){
			index += array->size();
		}
		if(index >= array->size()){
			ASTERIA_DEBUG_LOG("Array index out of range: index = ", static_cast<std::int64_t>(index), ", size = ", array->size());
			return nullptr;
		}
		return array->at(index).share(); }
	case type_object_member: {
		const auto &pair = boost::get<Object_member>(m_variant);
		const auto object = pair.first->try_get<Object>();
		if(!object){
			ASTERIA_THROW("Only objects can be indexed by strings, while the operand has type `", pair.first->get_type_name(), "`");
		}
		const auto it = object->find(pair.second);
		if(it == object->end()){
			ASTERIA_DEBUG_LOG("Object member not found: id = ", pair.second);
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
	case type_null_reference: {
		ASTERIA_THROW("Attempt to write to a null reference"); }
	case type_direct_reference: {
		const auto &variable = boost::get<Direct_reference>(m_variant);
		ASTERIA_THROW("Attempt to write to a temporary variable of type `", variable->get_type_name(), "`");
		/*return variable = std::move(new_value);*/ }
	case type_array_element: {
		const auto &pair = boost::get<Array_element>(m_variant);
		const auto array = pair.first->try_get<Array>();
		if(!array){
			ASTERIA_THROW("Only arrays can be indexed by integers, while the operand has type `", pair.first->get_type_name(), "`");
		}
		auto index = static_cast<std::uint64_t>(pair.second);
		if(pair.second < 0){
			index += array->size();
		}
		if(index >= array->max_size()){
			ASTERIA_THROW("Array index is too large, got `", index, "`");
		}
		while(index >= array->size()){
			array->emplace_back(make_value<Variable>(nullptr));
		}
		return array->at(index) = std::move(new_value); }
	case type_object_member: {
		const auto &pair = boost::get<Object_member>(m_variant);
		const auto object = pair.first->try_get<Object>();
		if(!object){
			ASTERIA_THROW("Only objects can be indexed by strings, while the operand has type `", pair.first->get_type_name(), "`");
		}
		return (*object)[pair.second] = std::move(new_value); }
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
