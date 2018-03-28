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
		const auto array = params.variable->get_opt<Array>();
		if(!array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integers, while the operand had type `", get_variable_type_name(params.variable), "`");
		}
		auto normalized_index = (params.index_bidirectional >= 0) ? params.index_bidirectional
		                                                          : static_cast<std::int64_t>(static_cast<std::uint64_t>(params.index_bidirectional) + array->size());
		auto size_current = static_cast<std::int64_t>(array->size());
		if(normalized_index < 0){
			ASTERIA_DEBUG_LOG("Array index was before the front: index = ", params.index_bidirectional, ", size = ", array->size());
			return nullptr;
		}
		if(normalized_index >= size_current){
			ASTERIA_DEBUG_LOG("Array index was after the back: index = ", params.index_bidirectional, ", size = ", array->size());
			return nullptr;
		}
		auto &variable = array->at(static_cast<std::size_t>(normalized_index));
		return variable.share(); }
	case type_lvalue_object_member: {
		const auto &params = boost::get<Lvalue_object_member>(m_variant);
		const auto object = params.variable->get_opt<Object>();
		if(!object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by strings, while the operand had type `", get_variable_type_name(params.variable), "`");
		}
		auto it = object->find(params.key);
		if(it == object->end()){
			ASTERIA_DEBUG_LOG("Object member was not found: key = ", params.key);
			return nullptr;
		}
		auto &variable = it->second;
		return variable.share(); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}
Value_ptr<Variable> Reference::extract_opt(){
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_rvalue_generic: {
		auto &params = boost::get<Rvalue_generic>(m_variant);
		return Value_ptr<Variable>(std::move(params.value_opt)); }
	case type_lvalue_generic:
	case type_lvalue_array_element:
	case type_lvalue_object_member: {
		const auto ptr = load_opt();
		if(!ptr){
			return nullptr;
		}
		return create_variable_opt(*ptr); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}
void Reference::store(boost::optional<Variable> &&value_new_opt) const {
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_rvalue_generic: {
		const auto &params = boost::get<Rvalue_generic>(m_variant);
		ASTERIA_THROW_RUNTIME_ERROR("Attempted to modify a temporary variable having type ", get_variable_type_name(params.value_opt), "`");
		/*return ???;*/ }
	case type_lvalue_generic: {
		const auto &params = boost::get<Lvalue_generic>(m_variant);
		auto &variable = *(params.variable_ptr);
		return set_variable(variable, std::move(value_new_opt)); }
	case type_lvalue_array_element: {
		const auto &params = boost::get<Lvalue_array_element>(m_variant);
		const auto array = params.variable->get_opt<Array>();
		if(!array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integers, while the operand had type `", get_variable_type_name(params.variable), "`");
		}
		auto normalized_index = (params.index_bidirectional >= 0) ? params.index_bidirectional
		                                                          : static_cast<std::int64_t>(static_cast<std::uint64_t>(params.index_bidirectional) + array->size());
		auto size_current = static_cast<std::int64_t>(array->size());
		if(normalized_index < 0){
			ASTERIA_DEBUG_LOG("Creating array elements automatically in the front: index = ", params.index_bidirectional, ", size = ", array->size());
			do {
				array->emplace_front();
			} while(++normalized_index < 0);
		}
		if(normalized_index >= size_current){
			ASTERIA_DEBUG_LOG("Creating array elements automatically in the back: index = ", params.index_bidirectional, ", size = ", array->size());
			do {
				array->emplace_back();
			} while(normalized_index >= ++size_current);
		}
		auto &variable = array->at(static_cast<std::size_t>(normalized_index));
		return set_variable(variable, std::move(value_new_opt)); }
	case type_lvalue_object_member: {
		const auto &params = boost::get<Lvalue_object_member>(m_variant);
		const auto object = params.variable->get_opt<Object>();
		if(!object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by strings, while the operand had type `", get_variable_type_name(params.variable), "`");
		}
		auto it = object->find(params.key);
		if(it == object->end()){
			ASTERIA_DEBUG_LOG("Creating object member automatically: key = ", params.key);
			it = object->emplace(params.key, nullptr).first;
		}
		auto &variable = it->second;
		return set_variable(variable, std::move(value_new_opt)); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}
void Reference::store(Variable &&value_new_opt) const {
	store(boost::optional<Variable>(std::move(value_new_opt)));
}

}
