// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "value.hpp"
#include "opaque_base.hpp"
#include "function_base.hpp"
#include "utilities.hpp"
#include <cmath>

namespace Asteria {

Value::~Value() = default;

const char * get_type_name(Value::Type type) noexcept {
	switch(type){
	case Value::type_null:
		return "null";
	case Value::type_boolean:
		return "boolean";
	case Value::type_integer:
		return "integer";
	case Value::type_double:
		return "double";
	case Value::type_string:
		return "string";
	case Value::type_opaque:
		return "opaque";
	case Value::type_function:
		return "function";
	case Value::type_array:
		return "array";
	case Value::type_object:
		return "object";
	default:
		ASTERIA_DEBUG_LOG("An unknown value type enumeration `", type, "` is encountered. This is probably a bug. Please report.");
		std::terminate();
	}
}
Value::Type get_value_type(Sp_cref<const Value> value_opt) noexcept {
	if(value_opt){
		return value_opt->which();
	} else {
		return Value::type_null;
	}
}
const char * get_value_type_name(Sp_cref<const Value> value_opt) noexcept {
	const auto type = get_value_type(value_opt);
	return get_type_name(type);
}

namespace {
	void do_indent(std::ostream &os, unsigned indent){
		if(indent == 0){
			return;
		}
		os <<std::setfill(' ') <<std::setw(static_cast<int>(indent)) <<"";
	}
	void do_quote_string(std::ostream &os, const D_string &str){
		os <<'\"';
		for(auto it = str.begin(); it != str.end(); ++it){
			const unsigned value = static_cast<unsigned char>(*it);
			switch(value){
			case '\"':
				os <<'\\' <<'\"';
				continue;
			case '\\':
				os <<'\\' <<'\\';
				continue;
			case '\a':
				os <<'\\' <<'a';
				continue;
			case '\b':
				os <<'\\' <<'b';
				continue;
			case '\f':
				os <<'\\' <<'f';
				continue;
			case '\n':
				os <<'\\' <<'n';
				continue;
			case '\r':
				os <<'\\' <<'r';
				continue;
			case '\t':
				os <<'\\' <<'t';
				continue;
			case '\v':
				os <<'\\' <<'v';
				continue;
			}
			if((value < 0x20) || (0x7E < value)){
				os <<'\\' <<'x' <<std::hex <<std::setfill('0') <<std::setw(2) <<value;
				continue;
			}
			os <<static_cast<char>(value);
		}
		os <<'\"';
	}
}

void dump_value(std::ostream &os, Sp_cref<const Value> value_opt, unsigned indent_next, unsigned indent_increment){
	const auto type = get_value_type(value_opt);
	os <<get_type_name(type) <<": ";
	switch(type){
	case Value::type_null:
		os <<"null";
		return;
	case Value::type_boolean: {
		const auto &cand = value_opt->as<D_boolean>();
		os <<std::boolalpha <<std::nouppercase <<cand;
		return; }
	case Value::type_integer: {
		const auto &cand = value_opt->as<D_integer>();
		os <<std::dec <<cand;
		return; }
	case Value::type_double: {
		const auto &cand = value_opt->as<D_double>();
		os <<std::dec <<std::nouppercase <<std::setprecision(16) <<cand;
		return; }
	case Value::type_string: {
		const auto &cand = value_opt->as<D_string>();
		do_quote_string(os, cand);
		return; }
	case Value::type_opaque: {
		const auto &cand = value_opt->as<D_opaque>();
		os <<"opaque(\"" <<typeid(*cand).name() <<"\", ";
		do_quote_string(os, cand->describe());
		os << ')';
		return; }
	case Value::type_function: {
		const auto &cand = value_opt->as<D_opaque>();
		os <<"function(\"" <<typeid(*cand).name() <<"\", ";
		do_quote_string(os, cand->describe());
		os << ')';
		return; }
	case Value::type_array: {
		const auto &array = value_opt->as<D_array>();
		os <<'[';
		for(auto it = array.begin(); it != array.end(); ++it){
			os <<std::endl;
			do_indent(os, indent_next + indent_increment);
			os <<std::dec <<std::setw(static_cast<int>(std::ceil(std::log10(static_cast<double>(array.size()) - 0.1)))) <<(it - array.begin());
			os <<" = ";
			dump_value(os, *it, indent_next + indent_increment, indent_increment);
			os <<',';
		}
		if(!array.empty()){
			os <<std::endl;
			do_indent(os, indent_next);
		}
		os <<']';
		return; }
	case Value::type_object: {
		const auto &object = value_opt->as<D_object>();
		os <<'{';
		for(auto it = object.begin(); it != object.end(); ++it){
			os <<std::endl;
			do_indent(os, indent_next + indent_increment);
			do_quote_string(os, it->first);
			os <<" = ";
			dump_value(os, it->second, indent_next + indent_increment, indent_increment);
			os <<',';
		}
		if(!object.empty()){
			os <<std::endl;
			do_indent(os, indent_next);
		}
		os <<'}';
		return; }
	default:
		ASTERIA_DEBUG_LOG("An unknown value type enumeration `", type, "` is encountered. This is probably a bug. Please report.");
		std::terminate();
	}
}
std::ostream & operator<<(std::ostream &os, Sp_cref<const Value> value_opt){
	dump_value(os, value_opt);
	return os;
}
std::ostream & operator<<(std::ostream &os, Vp_cref<const Value> value_opt){
	dump_value(os, value_opt);
	return os;
}

void allocate_value(Vp<Value> &value_out){
	auto sp = std::make_shared<Value>(true);
	value_out.reset(std::move(sp));
}
void copy_value(Vp<Value> &value_out, Sp_cref<const Value> src_opt){
	const auto type = get_value_type(src_opt);
	switch(type){
	case Value::type_null:
		clear_value(value_out);
		return;
	case Value::type_boolean: {
		const auto &cand = src_opt->as<D_boolean>();
		set_value(value_out, cand);
		return; }
	case Value::type_integer: {
		const auto &cand = src_opt->as<D_integer>();
		set_value(value_out, cand);
		return; }
	case Value::type_double: {
		const auto &cand = src_opt->as<D_double>();
		set_value(value_out, cand);
		return; }
	case Value::type_string: {
		const auto &cand = src_opt->as<D_string>();
		set_value(value_out, cand);
		return; }
	case Value::type_opaque:
		ASTERIA_THROW_RUNTIME_ERROR("Values having opaque types cannot be copied.");
	case Value::type_function: {
		const auto &cand = src_opt->as<D_function>();
		set_value(value_out, cand);
		return; }
	case Value::type_array: {
		const auto &cand = src_opt->as<D_array>();
		D_array array;
		array.reserve(cand.size());
		for(const auto &elem : cand){
			copy_value(value_out, elem);
			array.emplace_back(std::move(value_out));
		}
		set_value(value_out, std::move(array));
		return; }
	case Value::type_object: {
		const auto &cand = src_opt->as<D_object>();
		D_object object;
		object.reserve(cand.size());
		for(const auto &pair : cand){
			copy_value(value_out, pair.second);
			object.emplace(pair.first, std::move(value_out));
		}
		set_value(value_out, std::move(object));
		return; }
	default:
		ASTERIA_DEBUG_LOG("An unknown value type enumeration `", type, "` is encountered. This is probably a bug. Please report.");
		std::terminate();
	}
}
bool test_value(Sp_cref<const Value> value_opt) noexcept {
	const auto type = get_value_type(value_opt);
	switch(type){
	case Value::type_null:
		return false;
	case Value::type_boolean: {
		const auto &cand = value_opt->as<D_boolean>();
		return cand; }
	case Value::type_integer: {
		const auto &cand = value_opt->as<D_integer>();
		return cand != 0; }
	case Value::type_double: {
		const auto &cand = value_opt->as<D_double>();
		return std::fpclassify(cand) != FP_ZERO; }
	case Value::type_string: {
		const auto &cand = value_opt->as<D_string>();
		return cand.empty() == false; }
	case Value::type_opaque:
	case Value::type_function:
		return true;
	case Value::type_array: {
		const auto &cand = value_opt->as<D_array>();
		return cand.empty() == false; }
	case Value::type_object: {
		const auto &cand = value_opt->as<D_object>();
		return cand.empty() == false; }
	default:
		ASTERIA_DEBUG_LOG("An unknown value type enumeration `", type, "` is encountered. This is probably a bug. Please report.");
		std::terminate();
	}
}
Value::Comparison_result compare_values(Sp_cref<const Value> lhs_opt, Sp_cref<const Value> rhs_opt) noexcept {
	// `null` is considered to be equal to `null` and less than anything else.
	const auto type_lhs = get_value_type(lhs_opt);
	const auto type_rhs = get_value_type(rhs_opt);
	if(type_lhs != type_rhs){
		if(type_lhs == Value::type_null){
			return Value::comparison_result_less;
		}
		if(type_rhs == Value::type_null){
			return Value::comparison_result_greater;
		}
		return Value::comparison_result_unordered;
	}
	// If both operands have the same type, perform normal comparison.
	switch(type_lhs){
	case Value::type_null:
		return Value::comparison_result_equal;
	case Value::type_boolean: {
		const auto &cand_lhs = lhs_opt->as<D_boolean>();
		const auto &cand_rhs = rhs_opt->as<D_boolean>();
		if(cand_lhs < cand_rhs){
			return Value::comparison_result_less;
		}
		if(cand_lhs > cand_rhs){
			return Value::comparison_result_greater;
		}
		return Value::comparison_result_equal; }
	case Value::type_integer: {
		const auto &cand_lhs = lhs_opt->as<D_integer>();
		const auto &cand_rhs = rhs_opt->as<D_integer>();
		if(cand_lhs < cand_rhs){
			return Value::comparison_result_less;
		}
		if(cand_lhs > cand_rhs){
			return Value::comparison_result_greater;
		}
		return Value::comparison_result_equal; }
	case Value::type_double: {
		const auto &cand_lhs = lhs_opt->as<D_double>();
		const auto &cand_rhs = rhs_opt->as<D_double>();
		if(std::isunordered(cand_lhs, cand_rhs)){
			return Value::comparison_result_unordered;
		}
		if(std::isless(cand_lhs, cand_rhs)){
			return Value::comparison_result_less;
		}
		if(std::isgreater(cand_lhs, cand_rhs)){
			return Value::comparison_result_greater;
		}
		return Value::comparison_result_equal; }
	case Value::type_string: {
		const auto &cand_lhs = lhs_opt->as<D_string>();
		const auto &cand_rhs = rhs_opt->as<D_string>();
		const int cmp = cand_lhs.compare(cand_rhs);
		if(cmp < 0){
			return Value::comparison_result_less;
		}
		if(cmp > 0){
			return Value::comparison_result_greater;
		}
		return Value::comparison_result_equal; }
	case Value::type_opaque:
	case Value::type_function:
		return Value::comparison_result_unordered;
	case Value::type_array: {
		const auto &array_lhs = lhs_opt->as<D_array>();
		const auto &array_rhs = rhs_opt->as<D_array>();
		const auto len_min = std::min(array_lhs.size(), array_rhs.size());
		for(std::size_t i = 0; i < len_min; ++i){
			const auto res = compare_values(array_lhs.at(i), array_rhs.at(i));
			if(res != Value::comparison_result_equal){
				return res;
			}
		}
		if(array_lhs.size() < array_rhs.size()){
			return Value::comparison_result_less;
		}
		if(array_lhs.size() > array_rhs.size()){
			return Value::comparison_result_greater;
		}
		return Value::comparison_result_equal; }
	case Value::type_object:
		return Value::comparison_result_unordered;
	default:
		ASTERIA_DEBUG_LOG("An unknown value type enumeration `", type_lhs, "` is encountered. This is probably a bug. Please report.");
		std::terminate();
	}
}

}
