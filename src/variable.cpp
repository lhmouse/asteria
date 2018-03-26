// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "misc.hpp"

namespace Asteria {

const char *Variable::get_name_of_type(Variable::Type type) noexcept {
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
	case Variable::type_array:
		return "array";
	case Variable::type_object:
		return "object";
	case Variable::type_function:
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

// Non-member functions.

void dump_with_indent(std::ostream &os, const Array &array, bool include_types, unsigned indent_next, unsigned indent_increment){
	os <<'[';
	for(const auto &ptr : array){
		os <<std::endl;
		apply_indent(os, indent_next + indent_increment);
		dump_with_indent(os, *ptr, include_types, indent_next + indent_increment, indent_increment);
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
		dump_with_indent(os, *pair.second, include_types, indent_next + indent_increment, indent_increment);
		os <<',';
	}
	if(!object.empty()){
		os <<std::endl;
		apply_indent(os, indent_next);
	}
	os <<'}';
}
void dump_with_indent(std::ostream &os, const Variable &variable, bool include_types, unsigned indent_next, unsigned indent_increment){
	const auto type = variable.get_type();
	if(!include_types){
		os <<Variable::get_name_of_type(type) <<": ";
	}
	switch(type){
	case Variable::type_null:
		os <<"null";
		break;
	case Variable::type_boolean:
		os <<(variable.get<Boolean>() ? "true" : "false");
		break;
	case Variable::type_integer:
		os <<variable.get<Integer>();
		break;
	case Variable::type_double:
		os <<std::setprecision(16) <<variable.get<Double>();
		break;
	case Variable::type_string:
		quote_string(os, variable.get<String>());
		break;
	case Variable::type_opaque:
		os <<"opaque(\"" <<variable.get<Opaque>() <<"\")";
		break;
	case Variable::type_array:
		dump_with_indent(os, variable.get<Array>(), include_types, indent_next, indent_increment);
		break;
	case Variable::type_object:
		dump_with_indent(os, variable.get<Object>(), include_types, indent_next, indent_increment);
		break;
	case Variable::type_function:
		os <<"function";
		break;
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
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
