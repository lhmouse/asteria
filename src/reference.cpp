// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "variable.hpp"
#include "stored_value.hpp"
#include "recycler.hpp"
#include "misc.hpp"

namespace Asteria {

Reference::Reference(Reference &&) = default;
Reference &Reference::operator=(Reference &&) = default;
Reference::~Reference() = default;

std::tuple<Shared_ptr<Variable>, Value_ptr<Variable> *> Reference::do_dereference_once_opt(bool create_if_not_exist) const {
	const auto type = static_cast<Type>(m_variant.which());
	switch(type){
	case type_rvalue_generic: {
		const auto &params = boost::get<Rvalue_generic>(m_variant);
		return std::forward_as_tuple(params.value_opt, nullptr); }
	case type_lvalue_generic: {
		const auto &params = boost::get<Lvalue_generic>(m_variant);
		auto &variable = params.named_variable->variable;
		return std::forward_as_tuple(variable, &variable); }
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
			if(!create_if_not_exist){
				ASTERIA_DEBUG_LOG("Array index was before the front: index = ", params.index_bidirectional, ", size = ", size_current);
				return std::forward_as_tuple(nullptr, nullptr);
			}
			ASTERIA_DEBUG_LOG("Creating array elements automatically in the front: index = ", params.index_bidirectional, ", size = ", size_current);
			const auto count_to_prepend = 0 - static_cast<std::uint64_t>(normalized_index);
			if(count_to_prepend > array->max_size() - array->size()){
				ASTERIA_THROW_RUNTIME_ERROR("The array is too large and cannot be allocated: count_to_prepend = ", count_to_prepend);
			}
			array->insert(array->begin(), static_cast<std::size_t>(count_to_prepend), nullptr);
			normalized_index += static_cast<std::int64_t>(count_to_prepend);
			size_current += static_cast<std::int64_t>(count_to_prepend);
			ASTERIA_DEBUG_LOG("Resized array successfully: normalized_index = ", normalized_index, ", size_current = ", size_current);
		} else if(normalized_index >= size_current){
			if(!create_if_not_exist){
				ASTERIA_DEBUG_LOG("Array index was after the back: index = ", params.index_bidirectional, ", size = ", size_current);
				return std::forward_as_tuple(nullptr, nullptr);
			}
			ASTERIA_DEBUG_LOG("Creating array elements automatically in the back: index = ", params.index_bidirectional, ", size = ", size_current);
			const auto count_to_append = static_cast<std::uint64_t>(normalized_index - size_current) + 1;
			if(count_to_append > array->max_size() - array->size()){
				ASTERIA_THROW_RUNTIME_ERROR("The array is too large and cannot be allocated: count_to_append = ", count_to_append);
			}
			array->insert(array->end(), static_cast<std::size_t>(count_to_append), nullptr);
			normalized_index += static_cast<std::int64_t>(0);
			size_current += static_cast<std::int64_t>(count_to_append);
			ASTERIA_DEBUG_LOG("Resized array successfully: normalized_index = ", normalized_index, ", size_current = ", size_current);
		}
		auto &variable = array->at(static_cast<std::size_t>(normalized_index));
		return std::forward_as_tuple(variable, &variable); }
	case type_lvalue_object_member: {
		const auto &params = boost::get<Lvalue_object_member>(m_variant);
		const auto object = params.variable->get_opt<Object>();
		if(!object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by strings, while the operand had type `", get_variable_type_name(params.variable), "`");
		}
		auto it = object->find(params.key);
		if(it == object->end()){
			if(!create_if_not_exist){
				ASTERIA_DEBUG_LOG("Object member was not found: key = ", params.key);
				return std::forward_as_tuple(nullptr, nullptr);
			}
			ASTERIA_DEBUG_LOG("Creating object member automatically: key = ", params.key);
			it = object->emplace(params.key, nullptr).first;
			ASTERIA_DEBUG_LOG("Created object member successfuly: key = ", params.key);
		}
		auto &variable = it->second;
		return std::forward_as_tuple(variable, &variable); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

Shared_ptr<Variable> Reference::load_opt() const {
	Shared_ptr<Variable> ref_ptr;
	std::tie(ref_ptr, std::ignore) = do_dereference_once_opt(false);
	return ref_ptr;
}
void Reference::store(const Shared_ptr<Recycler> &recycler, Stored_value &&value_opt) const {
	Shared_ptr<Variable> ref_ptr;
	Value_ptr<Variable> *pvar;
	std::tie(ref_ptr, pvar) = do_dereference_once_opt(true);
	if(!pvar){
		ASTERIA_THROW_RUNTIME_ERROR("This variable having type ", get_variable_type_name(ref_ptr), "` was unwriteable. It was probably a temporary value.");
	}
	recycler->set_variable(*pvar, std::move(value_opt));
}
Value_ptr<Variable> Reference::extract_opt(const Shared_ptr<Recycler> &recycler){
	const auto type = static_cast<Type>(m_variant.which());
	if(type == type_rvalue_generic){
		auto &params = boost::get<Rvalue_generic>(m_variant);
		return Value_ptr<Variable>(std::move(params.value_opt));
	} else {
		Shared_ptr<Variable> ref_ptr;
		std::tie(ref_ptr, std::ignore) = do_dereference_once_opt(false);
		if(!ref_ptr){
			return nullptr;
		}
		return recycler->create_variable_opt(*ref_ptr);
	}
}

}
