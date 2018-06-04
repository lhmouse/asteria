// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "stored_reference.hpp"
#include "stored_value.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference::~Reference() = default;

Reference::Type get_reference_type(Spparam<const Reference> reference_opt) noexcept {
	return reference_opt ? reference_opt->get_type() : Reference::type_null;
}

void dump_reference(std::ostream &os, Spparam<const Reference> reference_opt, unsigned indent_next, unsigned indent_increment){
	const auto type = get_reference_type(reference_opt);
	switch(type){
	case Reference::type_null:
		os <<"null ";
		return dump_value(os, nullptr, indent_next, indent_increment);

	case Reference::type_constant: {
		const auto &candidate = reference_opt->get<Reference::S_constant>();
		os <<"constant ";
		return dump_value(os, candidate.source_opt, indent_next, indent_increment); }

	case Reference::type_temporary_value: {
		const auto &candidate = reference_opt->get<Reference::S_temporary_value>();
		os <<"temporary candidate ";
		return dump_value(os, candidate.value_opt, indent_next, indent_increment); }

	case Reference::type_variable: {
		const auto &candidate = reference_opt->get<Reference::S_variable>();
		os <<(candidate.variable->is_constant() ? "named constant " : "named variable ");
		return dump_value(os, candidate.variable->get_value_opt(), indent_next, indent_increment); }

	case Reference::type_array_element: {
		const auto &candidate = reference_opt->get<Reference::S_array_element>();
		os <<"the element at index [" <<candidate.index <<"] of ";
		return dump_reference(os, candidate.parent_opt, indent_next, indent_increment); }

	case Reference::type_object_member: {
		const auto &candidate = reference_opt->get<Reference::S_object_member>();
		os <<"the candidate having key \"" <<candidate.key <<"\" in ";
		return dump_reference(os, candidate.parent_opt, indent_next, indent_increment); }

	default:
		ASTERIA_DEBUG_LOG("Unknown reference type enumeration: type = ", type);
		std::terminate();
	}
}
std::ostream & operator<<(std::ostream &os, const Sptr_formatter<Reference> &reference_fmt){
	dump_reference(os, reference_fmt.get());
	return os;
}

void copy_reference(Xptr<Reference> &reference_out, Spparam<const Reference> source_opt){
	const auto type = get_reference_type(source_opt);
	switch(type){
	case Reference::type_null:
		return set_reference(reference_out, nullptr);

	case Reference::type_constant: {
		const auto &candidate = source_opt->get<Reference::S_constant>();
		return set_reference(reference_out, candidate); }

	case Reference::type_temporary_value:
		ASTERIA_THROW_RUNTIME_ERROR("References holding temporary values cannot be copied.");

	case Reference::type_variable: {
		const auto &candidate = source_opt->get<Reference::S_variable>();
		return set_reference(reference_out, candidate); }

	case Reference::type_array_element: {
		const auto &candidate = source_opt->get<Reference::S_array_element>();
		copy_reference(reference_out, candidate.parent_opt);
		Reference::S_array_element array_element = { std::move(reference_out), candidate.index };
		return set_reference(reference_out, std::move(array_element)); }

	case Reference::type_object_member: {
		const auto &candidate = source_opt->get<Reference::S_object_member>();
		copy_reference(reference_out, candidate.parent_opt);
		Reference::S_object_member object_member = { std::move(reference_out), candidate.key };
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

Sptr<const Value> read_reference_opt(Spparam<const Reference> reference_opt){
	const auto type = get_reference_type(reference_opt);
	switch(type){
	case Reference::type_null:
		return nullptr;

	case Reference::type_constant: {
		const auto &candidate = reference_opt->get<Reference::S_constant>();
		return candidate.source_opt; }

	case Reference::type_temporary_value: {
		const auto &candidate = reference_opt->get<Reference::S_temporary_value>();
		return candidate.value_opt; }

	case Reference::type_variable: {
		const auto &candidate = reference_opt->get<Reference::S_variable>();
		return candidate.variable->get_value_opt(); }

	case Reference::type_array_element: {
		const auto &candidate = reference_opt->get<Reference::S_array_element>();
		// Get the parent, which has to be an array.
		const auto parent = read_reference_opt(candidate.parent_opt);
		if(get_value_type(parent) != Value::type_array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integer, while the operand has type `", get_value_type_name(parent), "`.");
		}
		const auto &array = parent->get<D_array>();
		// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
		auto normalized_index = (candidate.index >= 0) ? candidate.index : D_integer(Unsigned_integer(candidate.index) + array.size());
		if(normalized_index < 0){
			ASTERIA_DEBUG_LOG("D_array subscript falls before the front: index = ", candidate.index, ", size = ", array.size());
			return nullptr;
		} else if(normalized_index >= D_integer(array.size())){
			ASTERIA_DEBUG_LOG("D_array subscript falls after the back: index = ", candidate.index, ", size = ", array.size());
			return nullptr;
		}
		const auto &value_opt = array.at(static_cast<std::size_t>(normalized_index));
		return value_opt; }

	case Reference::type_object_member: {
		const auto &candidate = reference_opt->get<Reference::S_object_member>();
		// Get the parent, which has to be an object.
		const auto parent = read_reference_opt(candidate.parent_opt);
		if(get_value_type(parent) != Value::type_object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by string, while the operand has type `", get_value_type_name(parent), "`.");
		}
		const auto &object = parent->get<D_object>();
		// Find the element.
		auto it = object.find(candidate.key);
		if(it == object.end()){
			ASTERIA_DEBUG_LOG("D_object member not found: key = ", candidate.key);
			return nullptr;
		}
		const auto &value_opt = it->second;
		return value_opt; }

	default:
		ASTERIA_DEBUG_LOG("Unknown reference type enumeration: type = ", type);
		std::terminate();
	}
}
std::reference_wrapper<Xptr<Value>> drill_reference(Spparam<const Reference> reference_opt){
	const auto type = get_reference_type(reference_opt);
	switch(type){
	case Reference::type_null:
		ASTERIA_THROW_RUNTIME_ERROR("Writing through a null reference is not allowed.");

	case Reference::type_constant: {
		const auto &candidate = reference_opt->get<Reference::S_constant>();
		ASTERIA_THROW_RUNTIME_ERROR("The constant `", sptr_fmt(candidate.source_opt), "` cannot be modified."); }

	case Reference::type_temporary_value: {
		const auto &candidate = reference_opt->get<Reference::S_temporary_value>();
		ASTERIA_THROW_RUNTIME_ERROR("Modifying the temporary value `", sptr_fmt(candidate.value_opt), "` is likely to be an error hence is not allowed."); }

	case Reference::type_variable: {
		const auto &candidate = reference_opt->get<Reference::S_variable>();
		return candidate.variable->drill_for_value(); }

	case Reference::type_array_element: {
		const auto &candidate = reference_opt->get<Reference::S_array_element>();
		// Get the parent, which has to be an array.
		const auto parent = drill_reference(candidate.parent_opt).get().share();
		if(get_value_type(parent) != Value::type_array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integer, while the operand has type `", get_value_type_name(parent), "`");
		}
		auto &array = parent->get<D_array>();
		// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
		auto normalized_index = (candidate.index >= 0) ? candidate.index : D_integer(Unsigned_integer(candidate.index) + array.size());
		if(normalized_index < 0){
			// Prepend `null`s until the subscript designates the beginning.
			ASTERIA_DEBUG_LOG("Creating array elements automatically in the front: index = ", candidate.index, ", size = ", array.size());
			const auto count_to_prepend = 0 - Unsigned_integer(normalized_index);
			if(count_to_prepend > array.max_size() - array.size()){
				ASTERIA_THROW_RUNTIME_ERROR("Prepending `", count_to_prepend, "` element(s) to this array would result in an overlarge array that cannot be allocated.");
			}
			array.insert(array.begin(), rocket::fill_iterator<std::nullptr_t>(0), rocket::fill_iterator<std::nullptr_t>(static_cast<std::ptrdiff_t>(count_to_prepend)));
			normalized_index = 0;
		} else if(normalized_index >= D_integer(array.size())){
			// Append `null`s until the subscript designates the end.
			ASTERIA_DEBUG_LOG("Creating array elements automatically in the back: index = ", candidate.index, ", size = ", array.size());
			const auto count_to_append = Unsigned_integer(normalized_index) - array.size() + 1;
			if(count_to_append > array.max_size() - array.size()){
				ASTERIA_THROW_RUNTIME_ERROR("Appending `", count_to_append, "` element(s) to this array would result in an overlarge array that cannot not be allocated.");
			}
			array.insert(array.end(), rocket::fill_iterator<std::nullptr_t>(0), rocket::fill_iterator<std::nullptr_t>(static_cast<std::ptrdiff_t>(count_to_append)));
			normalized_index = D_integer(array.size() - 1);
		}
		auto &value_opt = array.at(static_cast<std::size_t>(normalized_index));
		return std::ref(value_opt); }

	case Reference::type_object_member: {
		const auto &candidate = reference_opt->get<Reference::S_object_member>();
		// Get the parent, which has to be an object.
		const auto parent = drill_reference(candidate.parent_opt).get().share();
		if(get_value_type(parent) != Value::type_object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by string, while the operand has type `", get_value_type_name(parent), "`");
		}
		auto &object = parent->get<D_object>();
		// Find the element.
		auto it = object.find(candidate.key);
		if(it == object.end()){
			ASTERIA_DEBUG_LOG("Creating object member automatically: key = ", candidate.key);
			it = object.emplace(candidate.key, nullptr).first;
		}
		auto &value_opt = it->second;
		return std::ref(value_opt); }

	default:
		ASTERIA_DEBUG_LOG("Unknown reference type enumeration: type = ", type);
		std::terminate();
	}
}

namespace {
	class Extract_value_result {
	private:
		rocket::variant<std::nullptr_t, Sptr<const Value>, Xptr<Value>> m_variant;

	public:
		Extract_value_result(std::nullptr_t = nullptr) noexcept
			: m_variant(nullptr)
		{ }
		Extract_value_result(Sptr<const Value> copyable_pointer) noexcept
			: m_variant(std::move(copyable_pointer))
		{ }
		Extract_value_result(Xptr<Value> &&movable_pointer) noexcept
			: m_variant(std::move(movable_pointer))
		{ }

	public:
		Sptr<const Value> get_copyable_pointer() const noexcept {
			switch(m_variant.index()){
			default:
				return nullptr;
			case 1:
				return m_variant.get<Sptr<const Value>>();
			case 2:
				return m_variant.get<Xptr<Value>>();
			}
		}
		bool is_movable() const noexcept {
			return m_variant.index() == 2;
		}
		Xptr<Value> & get_movable_pointer(){
			return m_variant.get<Xptr<Value>>();
		}
	};

	Extract_value_result do_try_extract_value(Spparam<Reference> reference_opt){
		const auto type = get_reference_type(reference_opt);
		switch(type){
		case Reference::type_null:
			return nullptr;

		case Reference::type_constant: {
			auto &candidate = reference_opt->get<Reference::S_constant>();
			return candidate.source_opt; }

		case Reference::type_temporary_value: {
			auto &candidate = reference_opt->get<Reference::S_temporary_value>();
			return std::move(candidate.value_opt); }

		case Reference::type_variable: {
			auto &candidate = reference_opt->get<Reference::S_variable>();
			return candidate.variable->get_value_opt(); }

		case Reference::type_array_element: {
			auto &candidate = reference_opt->get<Reference::S_array_element>();
			// Get the parent, which has to be an array.
			auto parent_result = do_try_extract_value(candidate.parent_opt);
			const auto parent = parent_result.get_copyable_pointer();
			if(get_value_type(parent) != Value::type_array){
				ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integer, while the operand has type `", get_value_type_name(parent), "`");
			}
			const auto &array = parent->get<D_array>();
			// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
			auto normalized_index = (candidate.index >= 0) ? candidate.index : D_integer(Unsigned_integer(candidate.index) + array.size());
			if(normalized_index < 0){
				ASTERIA_DEBUG_LOG("D_array subscript falls before the front: index = ", candidate.index, ", size = ", array.size());
				return nullptr;
			} else if(normalized_index >= D_integer(array.size())){
				ASTERIA_DEBUG_LOG("D_array subscript falls after the back: index = ", candidate.index, ", size = ", array.size());
				return nullptr;
			}
			const auto &value_opt = array.at(static_cast<std::size_t>(normalized_index));
			if(parent_result.is_movable() == false){
				return value_opt.share_c();
			}
			return const_cast<Xptr<Value> &&>(value_opt); }

		case Reference::type_object_member: {
			auto &candidate = reference_opt->get<Reference::S_object_member>();
			// Get the parent, which has to be an object.
			auto parent_result = do_try_extract_value(candidate.parent_opt);
			const auto parent = parent_result.get_copyable_pointer();
			if(get_value_type(parent) != Value::type_object){
				ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by string, while the operand has type `", get_value_type_name(parent), "`");
			}
			const auto &object = parent->get<D_object>();
			// Find the element.
			auto it = object.find(candidate.key);
			if(it == object.end()){
				ASTERIA_DEBUG_LOG("D_object member not found: key = ", candidate.key);
				return nullptr;
			}
			const auto &value_opt = it->second;
			if(parent_result.is_movable() == false){
				return value_opt.share_c();
			}
			return const_cast<Xptr<Value> &&>(value_opt); }

		default:
			ASTERIA_DEBUG_LOG("Unknown reference type enumeration: type = ", type);
			std::terminate();
		}
	}
}

void extract_value_from_reference(Xptr<Value> &value_out, Spparam<Recycler> recycler, Xptr<Reference> &&reference_opt){
	auto result = do_try_extract_value(reference_opt);
	if(result.is_movable() == false){
		return copy_value(value_out, recycler, result.get_copyable_pointer());
	}
	return move_value(value_out, recycler, std::move(result.get_movable_pointer()));
}

namespace {
	bool do_check_materializability(Spparam<const Reference> reference_opt){
		const auto type = get_reference_type(reference_opt);
		switch(type){
		case Reference::type_null:
			return false;
		case Reference::type_constant:
			return false;
		case Reference::type_temporary_value:
			return true;
		case Reference::type_variable:
			return false;
		case Reference::type_array_element: {
			const auto &candidate = reference_opt->get<Reference::S_array_element>();
			return do_check_materializability(candidate.parent_opt); }
		case Reference::type_object_member: {
			const auto &candidate = reference_opt->get<Reference::S_object_member>();
			return do_check_materializability(candidate.parent_opt); }
		default:
			ASTERIA_DEBUG_LOG("Unknown reference type enumeration: type = ", type);
			std::terminate();
		}
	}
}

void materialize_reference(Xptr<Reference> &reference_inout_opt, Spparam<Recycler> recycler, bool constant){
	if(do_check_materializability(reference_inout_opt) == false){
		return;
	}
	Xptr<Value> value;
	extract_value_from_reference(value, recycler, std::move(reference_inout_opt));
	auto var = std::make_shared<Variable>(std::move(value), constant);
	Reference::S_variable ref_l = { std::move(var) };
	return set_reference(reference_inout_opt, std::move(ref_l));
}

}
