// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "stored_reference.hpp"
#include "stored_value.hpp"
#include "local_variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference::~Reference() = default;

Reference::Type get_reference_type(Spcref<const Reference> reference_opt) noexcept {
	return reference_opt ? reference_opt->get_type() : Reference::type_null;
}

void dump_reference(std::ostream &os, Spcref<const Reference> reference_opt, unsigned indent_next, unsigned indent_increment){
	const auto type = get_reference_type(reference_opt);
	switch(type){
	case Reference::type_null: {
		os <<"null ";
		return dump_variable(os, nullptr, indent_next, indent_increment); }
	case Reference::type_constant: {
		const auto &value = reference_opt->get<Reference::S_constant>();
		os <<"constant ";
		return dump_variable(os, value.source_opt, indent_next, indent_increment); }
	case Reference::type_temporary_value: {
		const auto &value = reference_opt->get<Reference::S_temporary_value>();
		os <<"temporary value ";
		return dump_variable(os, value.variable_opt, indent_next, indent_increment); }
	case Reference::type_local_variable: {
		const auto &value = reference_opt->get<Reference::S_local_variable>();
		if(value.local_variable->is_immutable()){
			os <<"immutable ";
		}
		os <<"local variable ";
		return dump_variable(os, value.local_variable->get_variable_opt(), indent_next, indent_increment); }
	case Reference::type_array_element: {
		const auto &value = reference_opt->get<Reference::S_array_element>();
		os <<"the element at index [" <<value.index <<"] of ";
		return dump_reference(os, value.parent_opt, indent_next, indent_increment); }
	case Reference::type_object_member: {
		const auto &value = reference_opt->get<Reference::S_object_member>();
		os <<"the value having key \"" <<value.key <<"\"] in ";
		return dump_reference(os, value.parent_opt, indent_next, indent_increment); }
	default:
		ASTERIA_DEBUG_LOG("Unknown reference type enumeration: type = ", type);
		std::terminate();
	}
}

std::ostream &operator<<(std::ostream &os, Spcref<const Reference> reference_opt){
	dump_reference(os, reference_opt);
	return os;
}
std::ostream &operator<<(std::ostream &os, const Xptr<Reference> &reference_opt){
	dump_reference(os, reference_opt);
	return os;
}

void copy_reference(Xptr<Reference> &reference_out, Spcref<const Reference> source_opt){
	const auto type = get_reference_type(source_opt);
	switch(type){
	case Reference::type_null: {
		return set_reference(reference_out, nullptr); }
	case Reference::type_constant: {
		const auto &source = source_opt->get<Reference::S_constant>();
		return set_reference(reference_out, source); }
	case Reference::type_temporary_value:
		ASTERIA_THROW_RUNTIME_ERROR("References holding temporary values cannot be copied");
	case Reference::type_local_variable: {
		const auto &source = source_opt->get<Reference::S_local_variable>();
		return set_reference(reference_out, source); }
	case Reference::type_array_element: {
		const auto &source = source_opt->get<Reference::S_array_element>();
		copy_reference(reference_out, source.parent_opt);
		Reference::S_array_element array_element = { std::move(reference_out), source.index };
		return set_reference(reference_out, std::move(array_element)); }
	case Reference::type_object_member: {
		const auto &source = source_opt->get<Reference::S_object_member>();
		copy_reference(reference_out, source.parent_opt);
		Reference::S_object_member object_member = { std::move(reference_out), source.key };
		return set_reference(reference_out, std::move(object_member)); }
	default:
		ASTERIA_DEBUG_LOG("Unknown reference type enumeration: type = ", type);
		std::terminate();
	}
}
void move_reference(Xptr<Reference> &reference_out, Xptr<Reference> &&source_opt){
	if(source_opt == nullptr){
		return reference_out.reset();
	} else {
		return reference_out.reset(source_opt.release());
	}
}

Sptr<const Variable> read_reference_opt(Spcref<const Reference> reference_opt){
	const auto type = get_reference_type(reference_opt);
	switch(type){
	case Reference::type_null:
		return nullptr;
	case Reference::type_constant: {
		const auto &params = reference_opt->get<Reference::S_constant>();
		return params.source_opt; }
	case Reference::type_temporary_value: {
		const auto &params = reference_opt->get<Reference::S_temporary_value>();
		return params.variable_opt; }
	case Reference::type_local_variable: {
		const auto &params = reference_opt->get<Reference::S_local_variable>();
		return params.local_variable->get_variable_opt(); }
	case Reference::type_array_element: {
		const auto &params = reference_opt->get<Reference::S_array_element>();
		// Get the parent, which has to be an array.
		const auto parent = read_reference_opt(params.parent_opt);
		if(get_variable_type(parent) != Variable::type_array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integer, while the operand has type `", get_variable_type_name(parent), "`");
		}
		const auto &array = parent->get<D_array>();
		// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
		auto normalized_index = (params.index >= 0) ? params.index : static_cast<std::int64_t>(static_cast<std::uint64_t>(params.index) + array.size());
		if(normalized_index < 0){
			ASTERIA_DEBUG_LOG("Array subscript falls before the front: index = ", params.index, ", size = ", array.size());
			return nullptr;
		} else if(normalized_index >= static_cast<std::int64_t>(array.size())){
			ASTERIA_DEBUG_LOG("Array subscript falls after the back: index = ", params.index, ", size = ", array.size());
			return nullptr;
		}
		const auto &variable_opt = array.at(static_cast<std::size_t>(normalized_index));
		return variable_opt; }
	case Reference::type_object_member: {
		const auto &params = reference_opt->get<Reference::S_object_member>();
		// Get the parent, which has to be an object.
		const auto parent = read_reference_opt(params.parent_opt);
		if(get_variable_type(parent) != Variable::type_object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by string, while the operand has type `", get_variable_type_name(parent), "`");
		}
		const auto &object = parent->get<D_object>();
		// Find the element.
		auto it = object.find(params.key);
		if(it == object.end()){
			ASTERIA_DEBUG_LOG("Object member not found: key = ", params.key);
			return nullptr;
		}
		const auto &variable_opt = it->second;
		return variable_opt; }
	default:
		ASTERIA_DEBUG_LOG("Unknown reference type enumeration: type = ", type);
		std::terminate();
	}
}
std::reference_wrapper<Xptr<Variable>> drill_reference(Spcref<const Reference> reference_opt){
	const auto type = get_reference_type(reference_opt);
	switch(type){
	case Reference::type_null:
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to write through a null reference");
	case Reference::type_constant: {
		const auto &params = reference_opt->get<Reference::S_constant>();
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to modify a constant `", params.source_opt, "`");
		/*return;*/ }
	case Reference::type_temporary_value: {
		const auto &params = reference_opt->get<Reference::S_temporary_value>();
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to modify a temporary value `", params.variable_opt, "`");
		/*return;*/ }
	case Reference::type_local_variable: {
		const auto &params = reference_opt->get<Reference::S_local_variable>();
		return params.local_variable->drill_for_variable(); }
	case Reference::type_array_element: {
		const auto &params = reference_opt->get<Reference::S_array_element>();
		// Get the parent, which has to be an array.
		const auto parent = drill_reference(params.parent_opt).get().share();
		if(get_variable_type(parent) != Variable::type_array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integer, while the operand has type `", get_variable_type_name(parent), "`");
		}
		auto &array = parent->get<D_array>();
		// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
		auto normalized_index = (params.index >= 0) ? params.index : static_cast<std::int64_t>(static_cast<std::uint64_t>(params.index) + array.size());
		if(normalized_index < 0){
			// Prepend `null`s until the subscript designates the beginning.
			ASTERIA_DEBUG_LOG("Creating array elements automatically in the front: index = ", params.index, ", size = ", array.size());
			const auto count_to_prepend = 0 - static_cast<std::uint64_t>(normalized_index);
			if(count_to_prepend > array.max_size() - array.size()){
				ASTERIA_THROW_RUNTIME_ERROR("Cannot allocate such a large array: count_to_prepend = ", count_to_prepend);
			}
			array.insert(array.begin(), rocket::nullptr_filler(0), rocket::nullptr_filler(static_cast<std::ptrdiff_t>(count_to_prepend)));
			normalized_index = 0;
		} else if(normalized_index >= static_cast<std::int64_t>(array.size())){
			// Append `null`s until the subscript designates the end.
			ASTERIA_DEBUG_LOG("Creating array elements automatically in the back: index = ", params.index, ", size = ", array.size());
			const auto count_to_append = static_cast<std::uint64_t>(normalized_index) - array.size() + 1;
			if(count_to_append > array.max_size() - array.size()){
				ASTERIA_THROW_RUNTIME_ERROR("Cannot allocate such a large array: count_to_append = ", count_to_append);
			}
			array.insert(array.end(), rocket::nullptr_filler(0), rocket::nullptr_filler(static_cast<std::ptrdiff_t>(count_to_append)));
			normalized_index = static_cast<std::int64_t>(array.size() - 1);
		}
		auto &variable_opt = array.at(static_cast<std::size_t>(normalized_index));
		return std::ref(variable_opt); }
	case Reference::type_object_member: {
		const auto &params = reference_opt->get<Reference::S_object_member>();
		// Get the parent, which has to be an object.
		const auto parent = drill_reference(params.parent_opt).get().share();
		if(get_variable_type(parent) != Variable::type_object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by string, while the operand has type `", get_variable_type_name(parent), "`");
		}
		auto &object = parent->get<D_object>();
		// Find the element.
		auto it = object.find(params.key);
		if(it == object.end()){
			ASTERIA_DEBUG_LOG("Creating object member automatically: key = ", params.key);
			it = object.emplace(params.key, nullptr).first;
		}
		auto &variable_opt = it->second;
		return std::ref(variable_opt); }
	default:
		ASTERIA_DEBUG_LOG("Unknown reference type enumeration: type = ", type);
		std::terminate();
	}
}

namespace {
	std::tuple<Sptr<const Variable>, Xptr<Variable> *> do_try_extract_variable(Spcref<Reference> reference_opt){
		const auto type = get_reference_type(reference_opt);
		switch(type){
		case Reference::type_null:
			return std::forward_as_tuple(nullptr, nullptr);
		case Reference::type_constant: {
			auto &params = reference_opt->get<Reference::S_constant>();
			return std::forward_as_tuple(params.source_opt, nullptr); }
		case Reference::type_temporary_value: {
			auto &params = reference_opt->get<Reference::S_temporary_value>();
			return std::forward_as_tuple(params.variable_opt, &(params.variable_opt)); }
		case Reference::type_local_variable: {
			auto &params = reference_opt->get<Reference::S_local_variable>();
			return std::forward_as_tuple(params.local_variable->get_variable_opt(), nullptr); }
		case Reference::type_array_element: {
			auto &params = reference_opt->get<Reference::S_array_element>();
			// Get the parent, which has to be an array.
			Sptr<const Variable> parent;
			Xptr<Variable> *parent_wptr;
			std::tie(parent, parent_wptr) = do_try_extract_variable(params.parent_opt);
			if(get_variable_type(parent) != Variable::type_array){
				ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integer, while the operand has type `", get_variable_type_name(parent), "`");
			}
			const auto &array = parent->get<D_array>();
			// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
			auto normalized_index = (params.index >= 0) ? params.index : static_cast<std::int64_t>(static_cast<std::uint64_t>(params.index) + array.size());
			if(normalized_index < 0){
				ASTERIA_DEBUG_LOG("Array subscript falls before the front: index = ", params.index, ", size = ", array.size());
				return std::forward_as_tuple(nullptr, nullptr);
			} else if(normalized_index >= static_cast<std::int64_t>(array.size())){
				ASTERIA_DEBUG_LOG("Array subscript falls after the back: index = ", params.index, ", size = ", array.size());
				return std::forward_as_tuple(nullptr, nullptr);
			}
			const auto &variable_opt = array.at(static_cast<std::size_t>(normalized_index));
			return std::forward_as_tuple(variable_opt, parent_wptr ? const_cast<Xptr<Variable> *>(&variable_opt) : nullptr); }
		case Reference::type_object_member: {
			auto &params = reference_opt->get<Reference::S_object_member>();
			// Get the parent, which has to be an object.
			Sptr<const Variable> parent;
			Xptr<Variable> *parent_wptr;
			std::tie(parent, parent_wptr) = do_try_extract_variable(params.parent_opt);
			if(get_variable_type(parent) != Variable::type_object){
				ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by string, while the operand has type `", get_variable_type_name(parent), "`");
			}
			const auto &object = parent->get<D_object>();
			// Find the element.
			auto it = object.find(params.key);
			if(it == object.end()){
				ASTERIA_DEBUG_LOG("Object member not found: key = ", params.key);
				return std::forward_as_tuple(nullptr, nullptr);
			}
			const auto &variable_opt = it->second;
			return std::forward_as_tuple(variable_opt, parent_wptr ? const_cast<Xptr<Variable> *>(&variable_opt) : nullptr); }
		default:
			ASTERIA_DEBUG_LOG("Unknown reference type enumeration: type = ", type);
			std::terminate();
		}
	}
}

void extract_variable_from_reference(Xptr<Variable> &variable_out, Spcref<Recycler> recycler, Xptr<Reference> &&reference_opt){
	Sptr<const Variable> rref;
	Xptr<Variable> *wptr;
	std::tie(rref, wptr) = do_try_extract_variable(reference_opt);
	if(wptr){
		return move_variable(variable_out, recycler, std::move(*wptr));
	}
	return copy_variable(variable_out, recycler, rref);
}
void materialize_reference(Xptr<Reference> &reference_inout_opt, Spcref<Recycler> recycler, bool immutable){
	Xptr<Variable> *wptr;
	std::tie(std::ignore, wptr) = do_try_extract_variable(reference_inout_opt);
	if(wptr){
		Xptr<Variable> variable;
		move_variable(variable, recycler, std::move(*wptr));
		auto local_var = std::make_shared<Local_variable>(std::move(variable), immutable);
		Reference::S_local_variable ref_l = { std::move(local_var) };
		return set_reference(reference_inout_opt, std::move(ref_l));
	}
}

}
