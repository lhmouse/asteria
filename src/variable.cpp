// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "misc.hpp"

namespace Asteria {

Variable::Variable(const Variable &) = default;
Variable &Variable::operator=(const Variable &) = default;
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
	case Variable::type_array:
		return "array";
	case Variable::type_object:
		return "object";
	case Variable::type_function:
		return "function";
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration `", type, "`. This is probably a bug, please report.");
		return "unknown";
	}
}

Variable::Type get_variable_type(const Variable *variable_opt) noexcept {
	return variable_opt ? variable_opt->get_type() : Variable::type_null;
}
Variable::Type get_variable_type(const Shared_ptr<const Variable> &variable_opt) noexcept {
	return get_variable_type(variable_opt.get());
}
Variable::Type get_variable_type(const Value_ptr<Variable> &variable_opt) noexcept {
	return get_variable_type(variable_opt.get());
}

const char *get_variable_type_name(const Variable *variable_opt) noexcept {
	return get_type_name(get_variable_type(variable_opt));
}
const char *get_variable_type_name(const Shared_ptr<const Variable> &variable_opt) noexcept {
	return get_variable_type_name(variable_opt.get());
}
const char *get_variable_type_name(const Value_ptr<Variable> &variable_opt) noexcept {
	return get_variable_type_name(variable_opt.get());
}

void dump_variable_recursive(std::ostream &os, const Variable *variable_opt, unsigned indent_next, unsigned indent_increment){
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
		quote_string(os, value.magic);
		os <<", \"" <<value.handle << "\")";
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
		ASTERIA_DEBUG_LOG("Unknown type enumeration `", type, "`. This is probably a bug, please report.");
		std::terminate();
	}
}
void dump_variable_recursive(std::ostream &os, const Shared_ptr<const Variable> &variable_opt, unsigned indent_next, unsigned indent_increment){
	return dump_variable_recursive(os, variable_opt.get(), indent_next, indent_increment);
}
void dump_variable_recursive(std::ostream &os, const Value_ptr<Variable> &variable_opt, unsigned indent_next, unsigned indent_increment){
	return dump_variable_recursive(os, variable_opt.get(), indent_next, indent_increment);
}

std::ostream &operator<<(std::ostream &os, const Variable *variable_opt){
	dump_variable_recursive(os, variable_opt);
	return os;
}
std::ostream &operator<<(std::ostream &os, const Shared_ptr<const Variable> &variable_opt){
	return operator<<(os, variable_opt.get());
}
std::ostream &operator<<(std::ostream &os, const Value_ptr<Variable> &variable_opt){
	return operator<<(os, variable_opt.get());
}

void dispose_variable_recursive(Variable *variable_opt) noexcept {
	const auto type = get_variable_type(variable_opt);
	switch(type){
	case Variable::type_null:
		break;
	case Variable::type_boolean:
	case Variable::type_integer:
	case Variable::type_double:
	case Variable::type_string:
	case Variable::type_opaque:
		variable_opt->set(false);
		break;
	case Variable::type_array: {
		auto &value = variable_opt->get<Array>();
		for(auto &elem : value){
			dispose_variable_recursive(elem.get());
		}
		variable_opt->set(false);
		break; }
	case Variable::type_object: {
		auto &value = variable_opt->get<Object>();
		for(auto &pair : value){
			dispose_variable_recursive(pair.second.get());
		}
		variable_opt->set(false);
		break; }
	case Variable::type_function:
		variable_opt->set(false);
		break;
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration `", type, "`. This is probably a bug, please report.");
		std::terminate();
	}
}

}

template class boost::container::vector<Asteria::Value_ptr<Asteria::Variable>>;
template class boost::container::flat_map<std::string, Asteria::Value_ptr<Asteria::Variable>>;
template class std::function<Asteria::Shared_ptr<Asteria::Variable> (boost::container::vector<Asteria::Shared_ptr<Asteria::Variable>> &&)>;
