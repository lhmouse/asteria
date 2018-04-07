// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "stored_value.hpp"
#include "utilities.hpp"
#include "recycler.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cmath> // std::ceil

namespace Asteria {

Variable::Variable(Variable &&) = default;
Variable &Variable::operator=(Variable &&) = default;
Variable::~Variable() = default;

void Variable::do_throw_type_mismatch(Type expect) const {
	ASTERIA_THROW_RUNTIME_ERROR("Runtime type mismatch, expecting type `", get_type_name(expect), "` but got `", get_type_name(get_type()), "`");
}

const char *get_type_name(Variable::Type type) noexcept {
	switch(type){
	case Variable::type_null:
		return "null";
	case Variable::type_boolean:
		return "boolean";
	case Variable::type_integer:
		return "integer";
	case Variable::type_double:
		return "double";
	case Variable::type_string:
		return "string";
	case Variable::type_opaque:
		return "opaque";
	case Variable::type_function:
		return "function";
	case Variable::type_array:
		return "array";
	case Variable::type_object:
		return "object";
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration `", type, "`. This is probably a bug, please report.");
		return "unknown";
	}
}
Variable::Type get_variable_type(Spref<const Variable> variable_opt) noexcept {
	return variable_opt ? variable_opt->get_type() : Variable::type_null;
}
const char *get_variable_type_name(Spref<const Variable> variable_opt) noexcept {
	return get_type_name(get_variable_type(variable_opt));
}

namespace {
	void do_indent(std::ostream &os, unsigned indent){
		if(indent == 0){
			return;
		}
		os <<std::setfill(' ') <<std::setw(static_cast<int>(indent)) <<"";
	}
	void do_quote_string(std::ostream &os, const std::string &str){
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

bool test_variable(Spref<const Variable> variable_opt) noexcept {
	const auto type = get_variable_type(variable_opt);
	switch(type){
	case Variable::type_null:
		return false;
	case Variable::type_boolean: {
		const auto &value = variable_opt->get<D_boolean>();
		return value; }
	case Variable::type_integer: {
		const auto &value = variable_opt->get<D_integer>();
		return value != 0; }
	case Variable::type_double: {
		const auto &value = variable_opt->get<D_double>();
		return std::fpclassify(value) != FP_ZERO; }
	case Variable::type_string: {
		const auto &value = variable_opt->get<D_string>();
		return value.empty() == false; }
	case Variable::type_opaque:
	case Variable::type_function:
		return true;
	case Variable::type_array: {
		const auto &value = variable_opt->get<D_array>();
		return value.empty() == false; }
	case Variable::type_object: {
		const auto &value = variable_opt->get<D_object>();
		return value.empty() == false; }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration `", type, "`. This is probably a bug, please report.");
		std::terminate();
	}
}

void dump_variable_recursive(std::ostream &os, Spref<const Variable> variable_opt, unsigned indent_next, unsigned indent_increment){
	const auto type = get_variable_type(variable_opt);
	os <<get_type_name(type) <<": ";
	switch(type){
	case Variable::type_null: {
		os <<"null";
		return; }
	case Variable::type_boolean: {
		const auto &value = variable_opt->get<D_boolean>();
		os <<std::boolalpha <<std::nouppercase <<value;
		return; }
	case Variable::type_integer: {
		const auto &value = variable_opt->get<D_integer>();
		os <<std::dec <<value;
		return; }
	case Variable::type_double: {
		const auto &value = variable_opt->get<D_double>();
		os <<std::dec <<std::nouppercase <<std::setprecision(16) <<value;
		return; }
	case Variable::type_string: {
		const auto &value = variable_opt->get<D_string>();
		do_quote_string(os, value);
		return; }
	case Variable::type_opaque: {
		const auto &value = variable_opt->get<D_opaque>();
		boost::uuids::uuid uuid_temp;
		std::memcpy(&uuid_temp, value.uuid.data(), 16);
		os <<"opaque(\"" <<uuid_temp <<"\", \"" <<value.handle << "\")";
		return; }
	case Variable::type_function: {
		os <<"function";
		return; }
	case Variable::type_array: {
		const auto &array = variable_opt->get<D_array>();
		os <<'[';
		for(auto it = array.begin(); it != array.end(); ++it){
			os <<std::endl;
			const auto &elem = *it;
			do_indent(os, indent_next + indent_increment);
			os <<std::dec <<std::setw(static_cast<int>(std::ceil(std::log10(static_cast<double>(array.size()))))) <<(it - array.begin());
			os <<" = ";
			dump_variable_recursive(os, elem, indent_next + indent_increment, indent_increment);
			os <<',';
		}
		if(!(array.empty())){
			os <<std::endl;
			do_indent(os, indent_next);
		}
		os <<']';
		return; }
	case Variable::type_object: {
		const auto &object = variable_opt->get<D_object>();
		os <<'{';
		for(auto it = object.begin(); it != object.end(); ++it){
			os <<std::endl;
			const auto &pair = *it;
			do_indent(os, indent_next + indent_increment);
			do_quote_string(os, pair.first);
			os <<" = ";
			dump_variable_recursive(os, pair.second, indent_next + indent_increment, indent_increment);
			os <<',';
		}
		if(!(object.empty())){
			os <<std::endl;
			do_indent(os, indent_next);
		}
		os <<'}';
		return; }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration `", type, "`. This is probably a bug, please report.");
		std::terminate();
	}
}

std::ostream &operator<<(std::ostream &os, Spref<const Variable> variable_opt){
	dump_variable_recursive(os, variable_opt);
	return os;
}
std::ostream &operator<<(std::ostream &os, const Xptr<Variable> &variable_opt){
	dump_variable_recursive(os, variable_opt);
	return os;
}

Sptr<Variable> set_variable(Xptr<Variable> &variable_out, Spref<Recycler> recycler, Stored_value &&value_opt){
	return recycler->set_variable(variable_out, std::move(value_opt));
}
Sptr<Variable> copy_variable_recursive(Xptr<Variable> &variable_out, Spref<Recycler> recycler, Spref<const Variable> source_opt){
	const auto type = get_variable_type(source_opt);
	switch(type){
	case Variable::type_null: {
		return recycler->set_variable(variable_out, nullptr); }
	case Variable::type_boolean: {
		const auto &source = source_opt->get<D_boolean>();
		return recycler->set_variable(variable_out, source); }
	case Variable::type_integer: {
		const auto &source = source_opt->get<D_integer>();
		return recycler->set_variable(variable_out, source); }
	case Variable::type_double: {
		const auto &source = source_opt->get<D_double>();
		return recycler->set_variable(variable_out, source); }
	case Variable::type_string: {
		const auto &source = source_opt->get<D_string>();
		return recycler->set_variable(variable_out, source); }
	case Variable::type_opaque:
		ASTERIA_THROW_RUNTIME_ERROR("Variables having opaque types cannot be copied");
	case Variable::type_function: {
		const auto &source = source_opt->get<D_function>();
		return recycler->set_variable(variable_out, source); }
	case Variable::type_array: {
		const auto &source = source_opt->get<D_array>();
		D_array array;
		array.reserve(source.size());
		for(const auto &elem : source){
			copy_variable_recursive(variable_out, recycler, elem);
			array.emplace_back(std::move(variable_out));
		}
		return recycler->set_variable(variable_out, std::move(array)); }
	case Variable::type_object: {
		const auto &source = source_opt->get<D_object>();
		D_object object;
		object.reserve(source.size());
		for(const auto &pair : source){
			copy_variable_recursive(variable_out, recycler, pair.second);
			object.emplace(pair.first, std::move(variable_out));
		}
		return recycler->set_variable(variable_out, std::move(object)); }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration `", type, "`. This is probably a bug, please report.");
		std::terminate();
	}
}

void dispose_variable_recursive(Spref<Variable> variable_opt) noexcept {
	const auto type = get_variable_type(variable_opt);
	switch(type){
	case Variable::type_null:
	case Variable::type_boolean:
	case Variable::type_integer:
	case Variable::type_double:
	case Variable::type_string:
	case Variable::type_opaque:
	case Variable::type_function:
		return;
	case Variable::type_array: {
		auto &value = variable_opt->get<D_array>();
		for(auto &elem : value){
			dispose_variable_recursive(elem);
			elem = nullptr;
		}
		return; }
	case Variable::type_object: {
		auto &value = variable_opt->get<D_object>();
		for(auto &pair : value){
			dispose_variable_recursive(pair.second);
			pair.second = nullptr;
		}
		return; }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration `", type, "`. This is probably a bug, please report.");
		std::terminate();
	}
}

}
