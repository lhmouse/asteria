// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "misc.hpp"

template class boost::container::deque<std::shared_ptr<Asteria::Variable>>;
template class boost::container::flat_map<std::string, std::shared_ptr<Asteria::Variable>>;
template class std::function<std::shared_ptr<Asteria::Variable> (boost::container::deque<std::shared_ptr<Asteria::Variable>> &&)>;

namespace Asteria {

Variable::~Variable(){
	//
}

void Variable::do_throw_immutable() const {
	ASTERIA_THROW(__FUNCTION__, ": attempting to modify a constant: this = ", this);
}

// Non-member functions.

void dump_with_indent(std::ostream &os, const Array &array, unsigned indent_next, unsigned indent_increment){
	os <<'[';
	if(!array.empty()){
		for(const auto &ptr : array){
			os <<std::endl;
			apply_indent(os, indent_next + indent_increment);
			dump_with_indent(os, *ptr, indent_next + indent_increment, indent_increment);
			os <<',';
		}
		os <<std::endl;
		apply_indent(os, indent_next);
	}
	os <<']';
}
void dump_with_indent(std::ostream &os, const Object &object, unsigned indent_next, unsigned indent_increment){
	os <<'{';
	if(!object.empty()){
		for(const auto &pair : object){
			os <<std::endl;
			apply_indent(os, indent_next + indent_increment);
			quote_string(os, pair.first);
			os <<" = ";
			dump_with_indent(os, *pair.second, indent_next + indent_increment, indent_increment);
			os <<',';
		}
		os <<std::endl;
		apply_indent(os, indent_next);
	}
	os <<'}';
}
void dump_with_indent(std::ostream &os, const Variable &variable, unsigned indent_next, unsigned indent_increment){
	const auto type = variable.get_type();
	// Dump the type.
	switch(type){
	case Variable::type_null:
		os <<"null";
		break;
	case Variable::type_boolean:
		os <<"boolean";
		break;
	case Variable::type_integer:
		os <<"integer";
		break;
	case Variable::type_double:
		os <<"double";
		break;
	case Variable::type_string:
		os <<"string";
		break;
	case Variable::type_opaque:
		os <<"opaque";
		break;
	case Variable::type_array:
		os <<"array";
		break;
	case Variable::type_object:
		os <<"object";
		break;
	case Variable::type_function:
		os <<"function";
		break;
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
	// Dump attributes.
	if(variable.is_immutable()){
		os <<" const";
	}
	// Dump the value.
	os <<": ";
	switch(type){
	case Variable::type_null:
		os <<"null";
		break;
	case Variable::type_boolean:
		os <<std::boolalpha <<variable.get<Boolean>();
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
		os <<variable.get<Opaque>();
		break;
	case Variable::type_array:
		dump_with_indent(os, variable.get<Array>(), indent_next, indent_increment);
		break;
	case Variable::type_object:
		dump_with_indent(os, variable.get<Object>(), indent_next, indent_increment);
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
	dump_with_indent(os, rhs, 0, 2);
	return os;
}
std::ostream &operator<<(std::ostream &os, const Object &rhs){
	dump_with_indent(os, rhs, 0, 2);
	return os;
}
std::ostream &operator<<(std::ostream &os, const Variable &rhs){
	dump_with_indent(os, rhs, 0, 2);
	return os;
}

}
