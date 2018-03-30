// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "variable.hpp"
#include "stored_value.hpp"
#include "recycler.hpp"
#include "misc.hpp"

namespace Asteria {

struct Reference::Dereference_once_result {
	Sp<Variable> rvar_opt;  // How to read a value through this reference?
	Value_ptr<Variable> *wptr_opt;  // How to write a value through this reference?
	bool immutable;                 // Is this reference read-only?
};

Reference::Reference(Reference &&) = default;
Reference &Reference::operator=(Reference &&) = default;
Reference::~Reference() = default;

Reference::Dereference_once_result Reference::do_dereference_once_opt(bool create_if_not_exist) const {
	const auto type = get_type();
	switch(type){
	case type_rvalue_generic: {
		const auto &params = get<Rvalue_generic>();
		Dereference_once_result res = { params.xvar_opt, nullptr, true };
		return std::move(res); }

	case type_lvalue_generic: {
		const auto &params = get<Lvalue_generic>();
		auto &variable = params.named_var->variable;
		Dereference_once_result res = { variable, &variable, params.named_var->immutable };
		return std::move(res); }

	case type_lvalue_array_element: {
		const auto &params = get<Lvalue_array_element>();
		const auto array = params.rvar->get_opt<Array>();
		if(!array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integers, while the operand had type `", get_variable_type_name(params.rvar), "`");
		}
		auto normalized_index = (params.index_bidirectional >= 0) ? params.index_bidirectional
		                                                          : static_cast<std::int64_t>(static_cast<std::uint64_t>(params.index_bidirectional) + array->size());
		auto size_current = static_cast<std::int64_t>(array->size());
		if(normalized_index < 0){
			if(!create_if_not_exist || params.immutable){
				ASTERIA_DEBUG_LOG("Array index was before the front: index = ", params.index_bidirectional, ", size = ", size_current);
				Dereference_once_result res = { nullptr, nullptr, params.immutable };
				return std::move(res);
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
			if(!create_if_not_exist || params.immutable){
				ASTERIA_DEBUG_LOG("Array index was after the back: index = ", params.index_bidirectional, ", size = ", size_current);
				Dereference_once_result res = { nullptr, nullptr, params.immutable };
				return std::move(res);
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
		Dereference_once_result res = { variable, &variable, params.immutable };
		return std::move(res); }

	case type_lvalue_object_member: {
		const auto &params = get<Lvalue_object_member>();
		const auto object = params.rvar->get_opt<Object>();
		if(!object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by strings, while the operand had type `", get_variable_type_name(params.rvar), "`");
		}
		auto it = object->find(params.key);
		if(it == object->end()){
			if(!create_if_not_exist || params.immutable){
				ASTERIA_DEBUG_LOG("Object member was not found: key = ", params.key);
				Dereference_once_result res = { nullptr, nullptr, params.immutable };
				return std::move(res);
			}
			ASTERIA_DEBUG_LOG("Creating object member automatically: key = ", params.key);
			it = object->emplace(params.key, nullptr).first;
			ASTERIA_DEBUG_LOG("Created object member successfuly: key = ", params.key);
		}
		auto &variable = it->second;
		Dereference_once_result res = { variable, &variable, params.immutable };
		return std::move(res); }

	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

Sp<Variable> Reference::load_opt() const {
	auto result = do_dereference_once_opt(false);
	return std::move(result.rvar_opt);
}
void Reference::store(Stored_value &&value_opt) const {
	if(!m_recycler){
		ASTERIA_THROW_RUNTIME_ERROR("No recycler has been associated with this reference. This is probably a bug, please report.");
	}
	auto result = do_dereference_once_opt(true);
	if(!(result.wptr_opt)){
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to write to a temporary variable having type `", get_variable_type_name(result.rvar_opt), "`");
	}
	if(result.immutable){
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to write to a constant having type `", get_variable_type_name(result.rvar_opt), "`");
	}
	m_recycler->set_variable(*(result.wptr_opt), std::move(value_opt));
}
Value_ptr<Variable> Reference::extract_opt(){
	if(!m_recycler){
		ASTERIA_THROW_RUNTIME_ERROR("No recycler has been associated with this reference. This is probably a bug, please report.");
	}
	Value_ptr<Variable> variable;
	if(get_type() == type_rvalue_generic){
		auto &params = get<Rvalue_generic>();
		variable = Value_ptr<Variable>(std::move(params.xvar_opt));
	} else {
		auto result = do_dereference_once_opt(false);
		m_recycler->copy_variable(variable, result.rvar_opt);
	}
	return variable;
}

}

template class std::function<Asteria::Reference (boost::container::vector<Asteria::Reference> &&)>;
