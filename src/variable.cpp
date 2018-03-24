// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "misc.hpp"

template class boost::container::deque<std::shared_ptr<Asteria::Variable>>;
template class boost::container::flat_map<std::string, std::shared_ptr<Asteria::Variable>>;
template class std::function<std::shared_ptr<Asteria::Variable> (boost::container::deque<std::shared_ptr<Asteria::Variable>> &&)>;

namespace Asteria {

namespace {
	void apply_indent(std::ostream &os, unsigned indent){
		if(indent != 0){
			os <<std::setfill(' ') <<std::setw(static_cast<int>(indent)) <<"";
		}
	}

	void quote_string(std::ostream &os, const char *str, std::size_t len){
		os <<'\"';
		for(std::size_t off = 0; off < len; ++off){
			const unsigned value = static_cast<unsigned char>(str[off]);
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
		std::terminate();
	}
}

Variable::~Variable(){
	//
}

void Variable::do_throw_immutable() const {
	ASTERIA_THROW(__FUNCTION__, ": attempting to modify a constant: this = ", this);
}
void Variable::do_dump_recursive(std::ostream &os, unsigned indent_next, unsigned indent_increment) const {
	const auto type = get_type();
	// Dump the type.
	os <<get_name_of_type(type) <<" ";
	// Dump attributes. At the moment the only attribute is immutability.
	if(is_immutable()){
		os <<"<const> ";
	}
	// Dump the value.
	switch(type){
	case type_null:
		os <<'(' <<"null" <<')';
		break;
	case type_boolean:
		os <<'(' <<std::boolalpha <<get<Type_boolean>() <<')';
		break;
	case type_integer:
		os <<'(' <<get<Type_integer>() <<')';
		break;
	case type_double:
		os <<'(' <<std::setprecision(16) <<get<Type_double>() <<')';
		break;
	case type_string: {
		os <<'(';
		const auto &str = get<Type_string>();
		quote_string(os, str.data(), str.size());
		os <<')';
		break; }
	case type_opaque:
		os <<'(' <<get<Type_opaque>() <<')';
		break;
	case type_array: {
		os <<'[';
		const auto &arr = get<Type_array>();
		if(!arr.empty()){
			for(const auto &elem : arr){
				os <<std::endl;
				apply_indent(os, indent_next + indent_increment);
				elem->do_dump_recursive(os, indent_next + indent_increment, indent_increment);
				os <<',';
			}
			os <<std::endl;
			apply_indent(os, indent_next);
		}
		os <<']';
		break; }
	case type_object: {
		os <<'{';
		const auto &obj = get<Type_object>();
		if(!obj.empty()){
			for(const auto &pair : obj){
				os <<std::endl;
				apply_indent(os, indent_next + indent_increment);
				quote_string(os, pair.first.data(), pair.first.size());
				os <<" = ";
				pair.second->do_dump_recursive(os, indent_next + indent_increment, indent_increment);
				os <<',';
			}
			os <<std::endl;
			apply_indent(os, indent_next);
		}
		os <<'}';
		break; }
	case type_function:
		os <<'(' <<"function" <<')';
		break;
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
		std::terminate();
	}
}

std::ostream &operator<<(std::ostream &os, const Variable &rhs){
	rhs.dump(os);
	return os;
}
std::ostream &operator<<(std::ostream &os, const Variable *rhs){
	return rhs ? (os <<*rhs) : (os <<"(null pointer)");
}
std::ostream &operator<<(std::ostream &os, Variable *rhs){
	return rhs ? (os <<*rhs) : (os <<"(null pointer)");
}
std::ostream &operator<<(std::ostream &os, const std::shared_ptr<const Variable> &rhs){
	return os <<rhs.get();
}
std::ostream &operator<<(std::ostream &os, const std::shared_ptr<Variable> &rhs){
	return os <<rhs.get();
}

}
