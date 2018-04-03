// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "utilities.hpp"
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

namespace {
	void apply_indent(std::ostream &os, unsigned indent){
		if(indent == 0){
			return;
		}
		os <<std::setfill(' ') <<std::setw(static_cast<int>(indent)) <<"";
	}
	void quote_string(std::ostream &os, const std::string &str){
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
void dump_variable_recursive(std::ostream &os, Spref<const Variable> variable_opt, unsigned indent_next, unsigned indent_increment){
	const auto type = get_variable_type(variable_opt);
	os <<get_type_name(type) <<": ";
	switch(type){
	case Variable::type_null: {
		os <<"null";
		return; }
	case Variable::type_boolean: {
		const auto &value = variable_opt->get<Boolean>();
		os <<std::boolalpha <<std::nouppercase <<value;
		return; }
	case Variable::type_integer: {
		const auto &value = variable_opt->get<Integer>();
		os <<std::dec <<value;
		return; }
	case Variable::type_double: {
		const auto &value = variable_opt->get<Double>();
		os <<std::dec <<std::nouppercase <<std::setprecision(16) <<value;
		return; }
	case Variable::type_string: {
		const auto &value = variable_opt->get<String>();
		quote_string(os, value);
		return; }
	case Variable::type_opaque: {
		const auto &value = variable_opt->get<Opaque>();
		boost::uuids::uuid uuid_temp;
		std::memcpy(&uuid_temp, value.uuid.data(), 16);
		os <<"opaque(\"" <<uuid_temp <<"\", " <<std::dec <<value.context <<", \"" <<value.handle << "\")";
		return; }
	case Variable::type_function: {
		os <<"function";
		return; }
	case Variable::type_array: {
		const auto &array = variable_opt->get<Array>();
		os <<'[';
		for(auto it = array.begin(); it != array.end(); ++it){
			os <<std::endl;
			apply_indent(os, indent_next + indent_increment);
			os <<std::dec <<std::setw(static_cast<int>(std::ceil(std::log10(static_cast<double>(array.size()))))) <<(it - array.begin());
			os <<" = ";
			dump_variable_recursive(os, *it, indent_next + indent_increment, indent_increment);
			os <<',';
		}
		if(!(array.empty())){
			os <<std::endl;
			apply_indent(os, indent_next);
		}
		os <<']';
		return; }
	case Variable::type_object: {
		const auto &object = variable_opt->get<Object>();
		os <<'{';
		for(auto it = object.begin(); it != object.end(); ++it){
			os <<std::endl;
			apply_indent(os, indent_next + indent_increment);
			quote_string(os, it->first);
			os <<" = ";
			dump_variable_recursive(os, it->second, indent_next + indent_increment, indent_increment);
			os <<',';
		}
		if(!(object.empty())){
			os <<std::endl;
			apply_indent(os, indent_next);
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
		auto &value = variable_opt->get<Array>();
		for(auto &elem : value){
			dispose_variable_recursive(elem);
			elem = nullptr;
		}
		return; }
	case Variable::type_object: {
		auto &value = variable_opt->get<Object>();
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
