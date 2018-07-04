// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "stored_reference.hpp"
#include "value.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference::~Reference() = default;

Reference::Type get_reference_type(Sp_cref<const Reference> ref_opt) noexcept {
	return ref_opt ? ref_opt->get_type() : Reference::type_null;
}

void dump_reference(std::ostream &os, Sp_cref<const Reference> ref_opt, unsigned indent_next, unsigned indent_increment){
	const auto type = get_reference_type(ref_opt);
	switch(type){
	case Reference::type_null:
		os <<"null ";
		return dump_value(os, nullptr, indent_next, indent_increment);

	case Reference::type_constant: {
		const auto &cand = ref_opt->get<Reference::S_constant>();
		os <<"constant ";
		return dump_value(os, cand.src_opt, indent_next, indent_increment); }

	case Reference::type_temporary_value: {
		const auto &cand = ref_opt->get<Reference::S_temporary_value>();
		os <<"temporary value ";
		return dump_value(os, cand.value_opt, indent_next, indent_increment); }

	case Reference::type_variable: {
		const auto &cand = ref_opt->get<Reference::S_variable>();
		if(cand.variable->is_immutable()){
			os <<"immutable ";
		}
		os <<"variable ";
		return dump_value(os, cand.variable->get_value_opt(), indent_next, indent_increment); }

	case Reference::type_array_element: {
		const auto &cand = ref_opt->get<Reference::S_array_element>();
		os <<"the element at index [" <<cand.index <<"] of ";
		return dump_reference(os, cand.parent_opt, indent_next, indent_increment); }

	case Reference::type_object_member: {
		const auto &cand = ref_opt->get<Reference::S_object_member>();
		os <<"the cand having key \"" <<cand.key <<"\" in ";
		return dump_reference(os, cand.parent_opt, indent_next, indent_increment); }

	default:
		ASTERIA_DEBUG_LOG("An unknown reference type enumeration: type = ", type);
		std::terminate();
	}
}
//std::ostream & operator<<(std::ostream &os, const Sp_formatter<Reference> &reference_fmt){
//	dump_reference(os, reference_fmt.get());
//	return os;
//}

void copy_reference(Vp<Reference> &ref_out, Sp_cref<const Reference> src_opt){
	const auto type = get_reference_type(src_opt);
	switch(type){
	case Reference::type_null:
		return set_reference(ref_out, nullptr);

	case Reference::type_constant: {
		const auto &cand = src_opt->get<Reference::S_constant>();
		return set_reference(ref_out, cand); }

	case Reference::type_temporary_value:
		ASTERIA_THROW_RUNTIME_ERROR("References holding temporary values cannot be copied.");

	case Reference::type_variable: {
		const auto &cand = src_opt->get<Reference::S_variable>();
		return set_reference(ref_out, cand); }

	case Reference::type_array_element: {
		const auto &cand = src_opt->get<Reference::S_array_element>();
		copy_reference(ref_out, cand.parent_opt);
		Reference::S_array_element array_element = { std::move(ref_out), cand.index };
		return set_reference(ref_out, std::move(array_element)); }

	case Reference::type_object_member: {
		const auto &cand = src_opt->get<Reference::S_object_member>();
		copy_reference(ref_out, cand.parent_opt);
		Reference::S_object_member object_member = { std::move(ref_out), cand.key };
		return set_reference(ref_out, std::move(object_member)); }

	default:
		ASTERIA_DEBUG_LOG("An unknown reference type enumeration: type = ", type);
		std::terminate();
	}
}
void move_reference(Vp<Reference> &ref_out, Vp<Reference> &&src_opt){
	if(src_opt == nullptr){
		ref_out.reset();
	} else {
		ref_out.reset(src_opt.release());
	}
}

Sp<const Value> read_reference_opt(Sp_cref<const Reference> ref_opt){
	const auto type = get_reference_type(ref_opt);
	switch(type){
	case Reference::type_null:
		return nullptr;

	case Reference::type_constant: {
		const auto &cand = ref_opt->get<Reference::S_constant>();
		return cand.src_opt; }

	case Reference::type_temporary_value: {
		const auto &cand = ref_opt->get<Reference::S_temporary_value>();
		return cand.value_opt; }

	case Reference::type_variable: {
		const auto &cand = ref_opt->get<Reference::S_variable>();
		return cand.variable->get_value_opt(); }

	case Reference::type_array_element: {
		const auto &cand = ref_opt->get<Reference::S_array_element>();
		// Get the parent, which has to be an array.
		const auto parent = read_reference_opt(cand.parent_opt);
		if(get_value_type(parent) != Value::type_array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integer, while the operand has type `", get_value_type_name(parent), "`.");
		}
		const auto &array = parent->get<D_array>();
		// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
		auto normalized_index = (cand.index >= 0) ? cand.index : D_integer(Unsigned_integer(cand.index) + array.size());
		if(normalized_index < 0){
			ASTERIA_DEBUG_LOG("Array subscript falls before the front: index = ", cand.index, ", size = ", array.size());
			return nullptr;
		} else if(normalized_index >= D_integer(array.size())){
			ASTERIA_DEBUG_LOG("Array subscript falls after the back: index = ", cand.index, ", size = ", array.size());
			return nullptr;
		}
		const auto &value_opt = array.at(static_cast<std::size_t>(normalized_index));
		return value_opt; }

	case Reference::type_object_member: {
		const auto &cand = ref_opt->get<Reference::S_object_member>();
		// Get the parent, which has to be an object.
		const auto parent = read_reference_opt(cand.parent_opt);
		if(get_value_type(parent) != Value::type_object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by string, while the operand has type `", get_value_type_name(parent), "`.");
		}
		const auto &object = parent->get<D_object>();
		// Find the element.
		auto it = object.find(cand.key);
		if(it == object.end()){
			ASTERIA_DEBUG_LOG("Object member not found: key = ", cand.key);
			return nullptr;
		}
		const auto &value_opt = it->second;
		return value_opt; }

	default:
		ASTERIA_DEBUG_LOG("An unknown reference type enumeration: type = ", type);
		std::terminate();
	}
}
std::reference_wrapper<Vp<Value>> drill_reference(Sp_cref<const Reference> ref_opt){
	const auto type = get_reference_type(ref_opt);
	switch(type){
	case Reference::type_null:
		ASTERIA_THROW_RUNTIME_ERROR("Writing through a null reference is an error.");

	case Reference::type_constant: {
		const auto &cand = ref_opt->get<Reference::S_constant>();
		ASTERIA_THROW_RUNTIME_ERROR("The constant `", cand.src_opt, "` cannot be modified."); }

	case Reference::type_temporary_value: {
		const auto &cand = ref_opt->get<Reference::S_temporary_value>();
		ASTERIA_THROW_RUNTIME_ERROR("Modifying the temporary value `", cand.value_opt, "` is likely to be an error hence is not allowed."); }

	case Reference::type_variable: {
		const auto &cand = ref_opt->get<Reference::S_variable>();
		return cand.variable->mutate_value(); }

	case Reference::type_array_element: {
		const auto &cand = ref_opt->get<Reference::S_array_element>();
		// Get the parent, which has to be an array.
		const auto parent = drill_reference(cand.parent_opt).get().share();
		if(get_value_type(parent) != Value::type_array){
			ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integer, while the operand has type `", get_value_type_name(parent), "`.");
		}
		auto &array = parent->get<D_array>();
		// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
		auto normalized_index = (cand.index >= 0) ? cand.index : D_integer(Unsigned_integer(cand.index) + array.size());
		if(normalized_index < 0){
			// Prepend `null`s until the subscript designates the beginning.
			const auto count_to_prepend = 0 - Unsigned_integer(normalized_index);
			if(count_to_prepend > array.max_size() - array.size()){
				ASTERIA_THROW_RUNTIME_ERROR("Prepending `", count_to_prepend, "` element(s) to this array would result in an overlarge array that cannot be allocated.");
			}
			array.insert(array.begin(), rocket::fill_iterator<Nullptr>(0), rocket::fill_iterator<Nullptr>(static_cast<std::ptrdiff_t>(count_to_prepend)));
			ASTERIA_DEBUG_LOG("Created array elements automatically in the front: index = ", cand.index, ", size = ", array.size());
			normalized_index = 0;
		} else if(normalized_index >= D_integer(array.size())){
			// Append `null`s until the subscript designates the end.
			const auto count_to_append = Unsigned_integer(normalized_index) - array.size() + 1;
			if(count_to_append > array.max_size() - array.size()){
				ASTERIA_THROW_RUNTIME_ERROR("Appending `", count_to_append, "` element(s) to this array would result in an overlarge array that cannot not be allocated.");
			}
			array.insert(array.end(), rocket::fill_iterator<Nullptr>(0), rocket::fill_iterator<Nullptr>(static_cast<std::ptrdiff_t>(count_to_append)));
			ASTERIA_DEBUG_LOG("Created array elements automatically in the back: index = ", cand.index, ", size = ", array.size());
			normalized_index = D_integer(array.size() - 1);
		}
		auto &value_opt = array.at(static_cast<std::size_t>(normalized_index));
		return std::ref(value_opt); }

	case Reference::type_object_member: {
		const auto &cand = ref_opt->get<Reference::S_object_member>();
		// Get the parent, which has to be an object.
		const auto parent = drill_reference(cand.parent_opt).get().share();
		if(get_value_type(parent) != Value::type_object){
			ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by string, while the operand has type `", get_value_type_name(parent), "`.");
		}
		auto &object = parent->get<D_object>();
		// Find the element.
#ifdef __cpp_lib_unordered_map_try_emplace
		auto pair = object.insert_or_assign(cand.key, nullptr);
#else
		auto pair = object.insert(std::make_pair(cand.key, nullptr));
#endif
		if(pair.second){
			ASTERIA_DEBUG_LOG("Created object member automatically: key = ", cand.key);
		}
		auto &value_opt = pair.first->second;
		return std::ref(value_opt); }

	default:
		ASTERIA_DEBUG_LOG("An unknown reference type enumeration: type = ", type);
		std::terminate();
	}
}

namespace {
	class Extract_value_result {
	private:
		rocket::variant<Nullptr, Sp<const Value>, Vp<Value>> m_variant;

	public:
		Extract_value_result(Nullptr = nullptr) noexcept
			: m_variant(nullptr)
		{ }
		Extract_value_result(Sp<const Value> copyable_pointer) noexcept
			: m_variant(std::move(copyable_pointer))
		{ }
		Extract_value_result(Vp<Value> &&movable_pointer) noexcept
			: m_variant(std::move(movable_pointer))
		{ }

	public:
		Sp<const Value> get_copyable_pointer() const noexcept {
			switch(m_variant.index()){
			default:
				return nullptr;
			case 1:
				return m_variant.get<Sp<const Value>>();
			case 2:
				return m_variant.get<Vp<Value>>();
			}
		}
		bool is_movable() const noexcept {
			return m_variant.index() == 2;
		}
		Vp<Value> & get_movable_pointer(){
			return m_variant.get<Vp<Value>>();
		}
	};

	Extract_value_result do_try_extract_value(Sp_cref<Reference> ref_opt){
		const auto type = get_reference_type(ref_opt);
		switch(type){
		case Reference::type_null:
			return nullptr;

		case Reference::type_constant: {
			auto &cand = ref_opt->get<Reference::S_constant>();
			return cand.src_opt; }

		case Reference::type_temporary_value: {
			auto &cand = ref_opt->get<Reference::S_temporary_value>();
			return std::move(cand.value_opt); }

		case Reference::type_variable: {
			auto &cand = ref_opt->get<Reference::S_variable>();
			return cand.variable->get_value_opt().share(); }

		case Reference::type_array_element: {
			auto &cand = ref_opt->get<Reference::S_array_element>();
			// Get the parent, which has to be an array.
			auto parent_result = do_try_extract_value(cand.parent_opt);
			const auto parent = parent_result.get_copyable_pointer();
			if(get_value_type(parent) != Value::type_array){
				ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integer, while the operand has type `", get_value_type_name(parent), "`.");
			}
			const auto &array = parent->get<D_array>();
			// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
			auto normalized_index = (cand.index >= 0) ? cand.index : D_integer(Unsigned_integer(cand.index) + array.size());
			if(normalized_index < 0){
				ASTERIA_DEBUG_LOG("Array subscript falls before the front: index = ", cand.index, ", size = ", array.size());
				return nullptr;
			} else if(normalized_index >= D_integer(array.size())){
				ASTERIA_DEBUG_LOG("Array subscript falls after the back: index = ", cand.index, ", size = ", array.size());
				return nullptr;
			}
			const auto &value_opt = array.at(static_cast<std::size_t>(normalized_index));
			if(parent_result.is_movable() == false){
				return value_opt.cshare();
			}
			return const_cast<Vp<Value> &&>(value_opt); }

		case Reference::type_object_member: {
			auto &cand = ref_opt->get<Reference::S_object_member>();
			// Get the parent, which has to be an object.
			auto parent_result = do_try_extract_value(cand.parent_opt);
			const auto parent = parent_result.get_copyable_pointer();
			if(get_value_type(parent) != Value::type_object){
				ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by string, while the operand has type `", get_value_type_name(parent), "`.");
			}
			const auto &object = parent->get<D_object>();
			// Find the element.
			auto it = object.find(cand.key);
			if(it == object.end()){
				ASTERIA_DEBUG_LOG("Object member not found: key = ", cand.key);
				return nullptr;
			}
			const auto &value_opt = it->second;
			if(parent_result.is_movable() == false){
				return value_opt.cshare();
			}
			return const_cast<Vp<Value> &&>(value_opt); }

		default:
			ASTERIA_DEBUG_LOG("An unknown reference type enumeration: type = ", type);
			std::terminate();
		}
	}
}

void extract_value_from_reference(Vp<Value> &value_out, Vp<Reference> &&ref_opt){
	auto result = do_try_extract_value(ref_opt);
	if(result.is_movable()){
		auto &movable_value = result.get_movable_pointer();
		if(movable_value){
			value_out.reset(movable_value.release());
			return;
		}
	}
	copy_value(value_out, result.get_copyable_pointer());
}

namespace {
	bool do_check_materializability(Sp_cref<const Reference> ref_opt){
		const auto type = get_reference_type(ref_opt);
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
			const auto &cand = ref_opt->get<Reference::S_array_element>();
			return do_check_materializability(cand.parent_opt); }
		case Reference::type_object_member: {
			const auto &cand = ref_opt->get<Reference::S_object_member>();
			return do_check_materializability(cand.parent_opt); }
		default:
			ASTERIA_DEBUG_LOG("An unknown reference type enumeration: type = ", type);
			std::terminate();
		}
	}
}

void materialize_reference(Vp<Reference> &reference_inout_opt, bool immutable){
	if(do_check_materializability(reference_inout_opt) == false){
		return;
	}
	Vp<Value> value;
	extract_value_from_reference(value, std::move(reference_inout_opt));
	auto var = std::make_shared<Variable>(std::move(value), immutable);
	Reference::S_variable ref_l = { std::move(var) };
	return set_reference(reference_inout_opt, std::move(ref_l));
}

}
// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "value.hpp"
#include "utilities.hpp"

namespace Asteria {

Variable::~Variable() = default;

void Variable::do_throw_immutable() const {
	ASTERIA_THROW_RUNTIME_ERROR("This variable having value `", m_value_opt, "` is immutable.");
}

}
