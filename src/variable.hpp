// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HPP_
#define ASTERIA_VARIABLE_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Variable {
public:
	enum Type : unsigned {
		type_null      = 0,
		type_boolean   = 1,
		type_integer   = 2,
		type_double    = 3,
		type_string    = 4,
		type_opaque    = 5,
		type_array     = 6,
		type_object    = 7,
		type_function  = 8,
	};

	using Types = Type_tuple< Null        // 0
	                        , Boolean     // 1
	                        , Integer     // 2
	                        , Double      // 3
	                        , String      // 4
	                        , Opaque      // 5
	                        , Array       // 6
	                        , Object      // 7
	                        , Function    // 8
		>;

public:
	static const char *get_name_of_type(Type type) noexcept;

private:
	Types::rebound_variant m_variant;
	bool m_immutable;

public:
	Variable()
		: m_variant(nullptr), m_immutable(false)
	{ }
	template<typename ValueT>
	Variable(ValueT &&value, bool immutable)
		: m_variant(std::forward<ValueT>(value)), m_immutable(immutable)
	{ }
	~Variable();

private:
	__attribute__((__noreturn__)) void do_throw_type_mismatch(Type expect) const;
	__attribute__((__noreturn__)) void do_throw_immutable() const;

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.which());
	}
	const char *get_type_name() const noexcept {
		return get_name_of_type(get_type());
	}
	template<Type expectT>
	const typename Types::at<expectT>::type &get() const {
		if(get_type() != expectT){
			do_throw_type_mismatch(expectT);
		}
		return *boost::get<typename Types::at<expectT>::type>(&m_variant);
	}
	template<Type expectT>
	typename Types::at<expectT>::type &get(){
		if(get_type() != expectT){
			do_throw_type_mismatch(expectT);
		}
		return *boost::get<typename Types::at<expectT>::type>(&m_variant);
	}
	template<typename ExpectT>
	const ExpectT &get() const {
		return get<static_cast<Type>(Types::index_of<ExpectT>::value)>();
	}
	template<typename ExpectT>
	ExpectT &get(){
		return get<static_cast<Type>(Types::index_of<ExpectT>::value)>();
	}
	template<typename ValueT>
	void set(ValueT &&value){
		if(m_immutable){
			do_throw_immutable();
		}
		m_variant = std::forward<ValueT>(value);
	}

	bool is_immutable() const noexcept {
		return m_immutable;
	}
	void set_immutable(bool immutable = true) noexcept {
		m_immutable = immutable;
	}
};

// These functions are for debugging purposes only.

extern void dump_with_indent(std::ostream &os, const Array &array, bool values_only, unsigned indent_next, unsigned indent_increment);
extern void dump_with_indent(std::ostream &os, const Object &object, bool values_only, unsigned indent_next, unsigned indent_increment);
extern void dump_with_indent(std::ostream &os, const Variable &variable, bool values_only, unsigned indent_next, unsigned indent_increment);

extern std::ostream &operator<<(std::ostream &os, const Array &rhs);
extern std::ostream &operator<<(std::ostream &os, const Object &rhs);
extern std::ostream &operator<<(std::ostream &os, const Variable &rhs);

}

#endif
