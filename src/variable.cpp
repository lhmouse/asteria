// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "misc.hpp"

template class boost::container::deque<Asteria::Value_ptr<Asteria::Variable>>;
template class boost::container::flat_map<std::string, Asteria::Value_ptr<Asteria::Variable>>;
template class std::function<Asteria::Value_ptr<Asteria::Variable> (Asteria::Array &&)>;

namespace Asteria {

Variable::~Variable(){
	//
}

void Variable::do_throw_type_mismatch(Type expect) const {
	ASTERIA_THROW_RUNTIME_ERROR("Runtime type mismatch, expecting type `", get_type_name(expect), "` but got `", get_type_name(get_type()), "`");
}
void Variable::do_throw_immutable() const {
	ASTERIA_THROW_RUNTIME_ERROR("Attempt to modify the constant `", *this, "`");
}

// Non-member functions

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
	case Variable::type_array:
		return "array";
	case Variable::type_object:
		return "object";
	case Variable::type_function:
		return "function";
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		return "unknown";
	}
}

void dump_variable_recursive(std::ostream &os, Observer_ptr<const Variable> variable_opt, unsigned indent_next, unsigned indent_increment){
	const auto type = get_variable_type(variable_opt);
	os <<get_type_name(type) <<": ";
	switch(type){
	case Variable::type_null: {
		os <<"null";
		break; }
	case Variable::type_boolean: {
		const auto &value = variable_opt->get<Boolean>();
		os <<std::boolalpha <<std::nouppercase <<value;
		break; }
	case Variable::type_integer: {
		const auto &value = variable_opt->get<Integer>();
		os <<std::dec <<value;
		break; }
	case Variable::type_double: {
		const auto &value = variable_opt->get<Double>();
		os <<std::dec <<std::nouppercase <<std::setprecision(16) <<value;
		break; }
	case Variable::type_string: {
		const auto &value = variable_opt->get<String>();
		quote_string(os, value);
		break; }
	case Variable::type_opaque: {
		const auto &value = variable_opt->get<Opaque>();
		os <<"opaque(";
		quote_string(os, value.first);
		os <<", \"" <<value.second << "\")";
		break; }
	case Variable::type_array: {
		const auto &value = variable_opt->get<Array>();
		os <<'[';
		for(const auto &elem : value){
			os <<std::endl;
			apply_indent(os, indent_next + indent_increment);
			dump_variable_recursive(os, elem, indent_next + indent_increment, indent_increment);
			os <<',';
		}
		if(!value.empty()){
			os <<std::endl;
			apply_indent(os, indent_next);
		}
		os <<']';
		break; }
	case Variable::type_object: {
		const auto &value = variable_opt->get<Object>();
		os <<'{';
		for(const auto &pair : value){
			os <<std::endl;
			apply_indent(os, indent_next + indent_increment);
			quote_string(os, pair.first);
			os <<" = ";
			dump_variable_recursive(os, pair.second, indent_next + indent_increment, indent_increment);
			os <<',';
		}
		if(!value.empty()){
			os <<std::endl;
			apply_indent(os, indent_next);
		}
		os <<'}';
		break; }
	case Variable::type_function: {
		os <<"function";
		break; }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

}
