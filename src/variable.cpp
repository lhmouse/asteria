// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "misc.hpp"

namespace Asteria {

const char *Variable::get_name_of_type(Variable::Type type) noexcept {
	switch(type){
	case type_null:
		return "null";
	case type_boolean:
		return "boolean";
	case type_integer:
		return "integer";
	case type_double:
		return "double";
	case type_string:
		return "string";
	case type_opaque:
		return "opaque";
	case type_array:
		return "array";
	case type_object:
		return "object";
	case type_function:
		return "function";
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		return "<unknown>";
	}
}

Variable::~Variable(){
	//
}

void Variable::do_throw_type_mismatch(Type expect) const {
	ASTERIA_THROW("Runtime type mismatch, expecting type `", get_name_of_type(expect), "` but got `", get_type_name(), "`");
}
void Variable::dump_with_indent_recursive(std::ostream &os, bool include_types, unsigned indent_next, unsigned indent_increment) const {
	const auto type = get_type();
	if(!include_types){
		os <<Variable::get_name_of_type(type) <<": ";
	}
	switch(type){
	case type_null:
		os <<"null";
		break;
	case type_boolean:
		os <<(get<Boolean>() ? "true" : "false");
		break;
	case type_integer:
		os <<get<Integer>();
		break;
	case type_double:
		os <<std::setprecision(16) <<get<Double>();
		break;
	case type_string:
		quote_string(os, get<String>());
		break;
	case type_opaque:
		os <<"opaque(\"" <<get<Opaque>() <<"\")";
		break;
	case type_array:
		dump_with_indent(os, get<Array>(), include_types, indent_next, indent_increment);
		break;
	case type_object:
		dump_with_indent(os, get<Object>(), include_types, indent_next, indent_increment);
		break;
	case type_function:
		os <<"function";
		break;
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

// Non-member functions.

void dump_with_indent(std::ostream &os, const Array &array, bool include_types, unsigned indent_next, unsigned indent_increment){
	os <<'[';
	for(const auto &ptr : array){
		os <<std::endl;
		apply_indent(os, indent_next + indent_increment);
		ptr->dump_with_indent_recursive(os, include_types, indent_next + indent_increment, indent_increment);
		os <<',';
	}
	if(!array.empty()){
		os <<std::endl;
		apply_indent(os, indent_next);
	}
	os <<']';
}
void dump_with_indent(std::ostream &os, const Object &object, bool include_types, unsigned indent_next, unsigned indent_increment){
	os <<'{';
	for(const auto &pair : object){
		os <<std::endl;
		apply_indent(os, indent_next + indent_increment);
		quote_string(os, pair.first);
		os <<" = ";
		pair.second->dump_with_indent_recursive(os, include_types, indent_next + indent_increment, indent_increment);
		os <<',';
	}
	if(!object.empty()){
		os <<std::endl;
		apply_indent(os, indent_next);
	}
	os <<'}';
}
void dump_with_indent(std::ostream &os, const Variable &variable, bool include_types, unsigned indent_next, unsigned indent_increment){
	variable.dump_with_indent_recursive(os, include_types, indent_next, indent_increment);
}

std::ostream &operator<<(std::ostream &os, const Array &rhs){
	dump_with_indent(os, rhs, true, 0, 2);
	return os;
}
std::ostream &operator<<(std::ostream &os, const Object &rhs){
	dump_with_indent(os, rhs, true, 0, 2);
	return os;
}
std::ostream &operator<<(std::ostream &os, const Variable &rhs){
	dump_with_indent(os, rhs, true, 0, 2);
	return os;
}

}
