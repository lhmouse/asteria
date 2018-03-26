// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "misc.hpp"

template class boost::container::deque<std::shared_ptr<Asteria::Variable>>;
template class boost::container::flat_map<std::string, std::shared_ptr<Asteria::Variable>>;
template class std::function<std::shared_ptr<Asteria::Variable> (boost::container::deque<std::shared_ptr<Asteria::Variable>> &&)>;

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
	ASTERIA_THROW("Runtime type mismatch, expecting type `", get_name_of_type(expect), "` but got value `", *this, "`");
}
void Variable::do_throw_immutable() const {
	ASTERIA_THROW("Attempt to modify a constant having value `", *this, "`");
}

// Non-member functions.

void dump_with_indent(std::ostream &os, const Array &array, bool values_only, unsigned indent_next, unsigned indent_increment){
	os <<'[';
	for(const auto &ptr : array){
		os <<std::endl;
		apply_indent(os, indent_next + indent_increment);
		dump_with_indent(os, *ptr, values_only, indent_next + indent_increment, indent_increment);
		os <<',';
	}
	if(!array.empty()){
		os <<std::endl;
		apply_indent(os, indent_next);
	}
	os <<']';
}
void dump_with_indent(std::ostream &os, const Object &object, bool values_only, unsigned indent_next, unsigned indent_increment){
	os <<'{';
	for(const auto &pair : object){
		os <<std::endl;
		apply_indent(os, indent_next + indent_increment);
		quote_string(os, pair.first);
		os <<" = ";
		dump_with_indent(os, *pair.second, values_only, indent_next + indent_increment, indent_increment);
		os <<',';
	}
	if(!object.empty()){
		os <<std::endl;
		apply_indent(os, indent_next);
	}
	os <<'}';
}
void dump_with_indent(std::ostream &os, const Variable &variable, bool values_only, unsigned indent_next, unsigned indent_increment){
	const auto type = variable.get_type();

	if(!values_only){
		// Dump the type.
		os <<Variable::get_name_of_type(type);
		// Dump attributes.
		if(variable.is_immutable()){
			os <<" const";
		}
		// Separate the value.
		os <<": ";
	}

	// Dump the value.
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
		dump_with_indent(os, variable.get<Array>(), values_only, indent_next, indent_increment);
		break;
	case Variable::type_object:
		dump_with_indent(os, variable.get<Object>(), values_only, indent_next, indent_increment);
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
	dump_with_indent(os, rhs, false, 0, 2);
	return os;
}
std::ostream &operator<<(std::ostream &os, const Object &rhs){
	dump_with_indent(os, rhs, false, 0, 2);
	return os;
}
std::ostream &operator<<(std::ostream &os, const Variable &rhs){
	dump_with_indent(os, rhs, false, 0, 2);
	return os;
}

}
