// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "stored_reference.hpp"
#include "variable.hpp"
#include "stored_value.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference::~Reference() = default;

Reference::Type get_reference_type(Spref<const Reference> reference_opt) noexcept {
	return reference_opt ? reference_opt->get_type() : Reference::type_null;
}

void dump_reference(std::ostream &os, Spref<const Reference> reference_opt, unsigned indent_next, unsigned indent_increment){
	const auto type = get_reference_type(reference_opt);
	switch(type){
	case Reference::type_null: {
		os <<"null";
		return; }
	case Reference::type_constant: {
		const auto &value = reference_opt->get<Reference::S_constant>();
		os <<"constant `";
		dump_variable(os, value.source_opt, indent_next, indent_increment);
		os <<"`";
		return; }
	case Reference::type_temporary_value: {
		const auto &value = reference_opt->get<Reference::S_temporary_value>();
		os <<"temporary value `";
		dump_variable(os, value.variable_opt, indent_next, indent_increment);
		os <<"`";
		return; }
	case Reference::type_local_variable: {
		const auto &value = reference_opt->get<Reference::S_local_variable>();
		if(value.local_variable->immutable){
			os <<"constant ";
		}
		os <<"local variable `";
		dump_variable(os, value.local_variable->variable_opt, indent_next, indent_increment);
		os <<"`";
		return; }
	case Reference::type_array_element: {
		const auto &value = reference_opt->get<Reference::S_array_element>();
		os <<"the element at index [" <<value.index <<"] of `";
		dump_reference(os, value.parent_opt, indent_next, indent_increment);
		os <<"`";
		return; }
	case Reference::type_object_member: {
		const auto &value = reference_opt->get<Reference::S_object_member>();
		os <<"the value having key \"" <<value.key <<"\"] in `";
		dump_reference(os, value.parent_opt, indent_next, indent_increment);
		os <<"`";
		return; }
	default:
		ASTERIA_DEBUG_LOG("Unknown reference type enumeration: type = ", type);
		std::terminate();
	}
}

std::ostream &operator<<(std::ostream &os, Spref<const Reference> reference_opt){
	dump_reference(os, reference_opt);
	return os;
}
std::ostream &operator<<(std::ostream &os, const Xptr<Reference> &reference_opt){
	dump_reference(os, reference_opt);
	return os;
}

void copy_reference(Xptr<Reference> &reference_out, Spref<const Reference> source_opt){
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

namespace {
	struct Dereference_result {
		Sptr<const Variable> rptr_opt;   // How to read a value through this reference?
		Sptr<const Variable> prptr_opt;  // Where is my parent?
		bool rvalue;                     // Is this reference an rvalue that must not be written into?
		Xptr<Variable> *wref_opt;        // How to write a value through this reference?
	};

	Dereference_result do_dereference_unsafe(Spref<Reference> reference_opt, bool create_as_needed){
		const auto type = get_reference_type(reference_opt);
		switch(type){
		case Reference::type_null: {
			// Return a null result.
			Dereference_result res = { nullptr, nullptr, true, nullptr };
			return std::move(res); }
		case Reference::type_constant: {
			auto &params = reference_opt->get<Reference::S_constant>();
			// The variable is read-only.
			Dereference_result res = { params.source_opt, nullptr, true, nullptr };
			return std::move(res); }
		case Reference::type_temporary_value: {
			auto &params = reference_opt->get<Reference::S_temporary_value>();
			// The variable has to be 'writeable' because it can be moved from.
			Dereference_result res = { params.variable_opt, nullptr, true, &(params.variable_opt) };
			return std::move(res); }
		case Reference::type_local_variable: {
			auto &params = reference_opt->get<Reference::S_local_variable>();
			// Local variables can be either read from or written into.
			const auto &local_var = params.local_variable;
			Dereference_result res = { local_var->variable_opt, nullptr, false, local_var->immutable ? nullptr : &(local_var->variable_opt) };
			return std::move(res); }
		case Reference::type_array_element: {
			auto &params = reference_opt->get<Reference::S_array_element>();
			// Get the parent, which has to be an array.
			const auto parent_result = do_dereference_unsafe(params.parent_opt, create_as_needed);
			if(get_variable_type(parent_result.rptr_opt) != Variable::type_array){
				ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integer, while the operand has type `", get_variable_type_name(parent_result.rptr_opt), "`");
			}
			// Throw an error in case of possibility of modification, even when there is no need to create a new element.
			if(create_as_needed && (parent_result.wref_opt == nullptr)){
				ASTERIA_THROW_RUNTIME_ERROR("Attempting to modify a constant array");
			}
			auto &array = std::const_pointer_cast<Variable>(parent_result.rptr_opt)->get<D_array>();
			// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
			auto normalized_index = (params.index >= 0) ? params.index : static_cast<std::int64_t>(static_cast<std::uint64_t>(params.index) + array.size());
			if(normalized_index < 0){
				if(!create_as_needed){
					ASTERIA_DEBUG_LOG("Array subscript falls before the front: index = ", params.index, ", size = ", array.size());
					Dereference_result res = { nullptr, parent_result.rptr_opt, parent_result.rvalue, nullptr };
					return std::move(res);
				}
				// Prepend `null`s until the subscript designates the beginning.
				ASTERIA_DEBUG_LOG("Creating array elements automatically in the front: index = ", params.index, ", size = ", array.size());
				const auto count_to_prepend = 0 - static_cast<std::uint64_t>(normalized_index);
				if(count_to_prepend > array.max_size() - array.size()){
					ASTERIA_THROW_RUNTIME_ERROR("Cannot allocate such a large array: count_to_prepend = ", count_to_prepend);
				}
				array.resize(array.size() + static_cast<std::size_t>(count_to_prepend));
				std::move_backward(array.begin(), std::prev(array.end(), static_cast<std::ptrdiff_t>(count_to_prepend)), array.end());
				normalized_index = 0;
			} else if(normalized_index >= static_cast<std::int64_t>(array.size())){
				if(!create_as_needed){
					ASTERIA_DEBUG_LOG("Array subscript falls after the back: index = ", params.index, ", size = ", array.size());
					Dereference_result res = { nullptr, parent_result.rptr_opt, parent_result.rvalue, nullptr };
					return std::move(res);
				}
				// Append `null`s until the subscript designates the end.
				ASTERIA_DEBUG_LOG("Creating array elements automatically in the back: index = ", params.index, ", size = ", array.size());
				const auto count_to_append = static_cast<std::uint64_t>(normalized_index) - array.size() + 1;
				if(count_to_append > array.max_size() - array.size()){
					ASTERIA_THROW_RUNTIME_ERROR("Cannot allocate such a large array: count_to_append = ", count_to_append);
				}
				array.resize(array.size() + static_cast<std::size_t>(count_to_append));
			}
			auto it = std::next(array.begin(), static_cast<std::ptrdiff_t>(normalized_index));
			Dereference_result res = { *it, parent_result.rptr_opt, parent_result.rvalue, &*it };
			return std::move(res); }
		case Reference::type_object_member: {
			auto &params = reference_opt->get<Reference::S_object_member>();
			// Get the parent, which has to be an object.
			const auto parent_result = do_dereference_unsafe(params.parent_opt, create_as_needed);
			if(get_variable_type(parent_result.rptr_opt) != Variable::type_object){
				ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by string, while the operand has type `", get_variable_type_name(parent_result.rptr_opt), "`");
			}
			// Throw an error in case of possibility of modification, even when there is no need to create a new member.
			if(create_as_needed && (parent_result.wref_opt == nullptr)){
				ASTERIA_THROW_RUNTIME_ERROR("Attempting to modify a constant object");
			}
			auto &object = std::const_pointer_cast<Variable>(parent_result.rptr_opt)->get<D_object>();
			// Find the element.
			auto it = object.find(params.key);
			if(it == object.end()){
				if(!create_as_needed){
					ASTERIA_DEBUG_LOG("Object member not found: key = ", params.key);
					Dereference_result res = { nullptr, parent_result.rptr_opt, parent_result.rvalue, nullptr };
					return std::move(res);
				}
				ASTERIA_DEBUG_LOG("Creating object member automatically: key = ", params.key);
				it = object.emplace(params.key, nullptr).first;
			}
			Dereference_result res = { it->second, parent_result.rptr_opt, parent_result.rvalue, &(it->second) };
			return std::move(res); }
		default:
			ASTERIA_DEBUG_LOG("Unknown reference type enumeration: type = ", type);
			std::terminate();
		}
	}
}

Sptr<const Variable> read_reference_opt(Spref<const Reference> reference_opt, Sptr<const Variable> *parent_out_opt){
	auto result = do_dereference_unsafe(std::const_pointer_cast<Reference>(reference_opt), false);
	if(parent_out_opt){
		*parent_out_opt = std::move(result.prptr_opt);
	}
	return std::move(result.rptr_opt);
}
std::reference_wrapper<Xptr<Variable>> drill_reference(Spref<Reference> reference_opt){
	if(reference_opt == nullptr){
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to write through a null reference");
	}
	auto result = do_dereference_unsafe(reference_opt, true);
	if(result.rvalue){
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to write through a reference holding a temporary value of type `", get_variable_type_name(result.rptr_opt), "`");
	}
	if(result.wref_opt == nullptr){
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to write through a reference holding a constant value of type `", get_variable_type_name(result.rptr_opt), "`");
	}
	return std::ref(*(result.wref_opt));
}

void extract_variable_from_reference(Xptr<Variable> &variable_out, Spref<Recycler> recycler, Xptr<Reference> &&reference_opt){
	auto result = do_dereference_unsafe(reference_opt, false);
	if(result.rptr_opt && result.rvalue && result.wref_opt){
		return move_variable(variable_out, recycler, std::move(*(result.wref_opt)));
	} else {
		return copy_variable(variable_out, recycler, result.rptr_opt);
	}
}

}
