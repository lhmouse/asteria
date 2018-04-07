// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HPP_
#define ASTERIA_VARIABLE_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Variable {
	friend Stored_value;

public:
	enum Type : unsigned {
		type_null      = -1u,
		type_boolean   =  0,
		type_integer   =  1,
		type_double    =  2,
		type_string    =  3,
		type_opaque    =  4,
		type_function  =  5,
		type_array     =  6,
		type_object    =  7,
	};
	using Types = Type_tuple< D_boolean    //  0
	                        , D_integer    //  1
	                        , D_double     //  2
	                        , D_string     //  3
	                        , D_opaque     //  4
	                        , D_function   //  5
	                        , D_array      //  6
	                        , D_object     //  7
		>;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Variable, ValueT)>
	Variable(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }
	Variable(Variable &&);
	Variable &operator=(Variable &&);
	~Variable();

private:
	__attribute__((__noreturn__)) void do_throw_type_mismatch(Type expect) const;

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.which());
	}
	template<typename ExpectT>
	const ExpectT *get_opt() const noexcept {
		return boost::get<ExpectT>(&m_variant);
	}
	template<typename ExpectT>
	ExpectT *get_opt() noexcept {
		return boost::get<ExpectT>(&m_variant);
	}
	template<typename ExpectT>
	const ExpectT &get() const {
		const auto ptr = get_opt<ExpectT>();
		if(!ptr){
			do_throw_type_mismatch(static_cast<Type>(Types::index_of<ExpectT>::value));
		}
		return *ptr;
	}
	template<typename ExpectT>
	ExpectT &get(){
		const auto ptr = get_opt<ExpectT>();
		if(!ptr){
			do_throw_type_mismatch(static_cast<Type>(Types::index_of<ExpectT>::value));
		}
		return *ptr;
	}
	template<typename ValueT>
	void set(ValueT &&value){
		m_variant = std::forward<ValueT>(value);
	}
};

extern const char *get_type_name(Variable::Type type) noexcept;
extern Variable::Type get_variable_type(Spref<const Variable> variable_opt) noexcept;
extern const char *get_variable_type_name(Spref<const Variable> variable_opt) noexcept;

extern bool test_variable(Spref<const Variable> variable_opt) noexcept;
extern void dump_variable_recursive(std::ostream &os, Spref<const Variable> variable_opt, unsigned indent_next = 0, unsigned indent_increment = 2);

extern std::ostream &operator<<(std::ostream &os, Spref<const Variable> variable_opt);
extern std::ostream &operator<<(std::ostream &os, const Xptr<Variable> &variable_opt);

// These functions return the old contents of the variables before the operations.
extern Sptr<Variable> set_variable(Xptr<Variable> &variable_out, Spref<Recycler> recycler, Stored_value &&value_opt);
extern Sptr<Variable> copy_variable_recursive(Xptr<Variable> &variable_out, Spref<Recycler> recycler, Spref<const Variable> source_opt);

// This function is used to break dependency circles.
extern void dispose_variable_recursive(Spref<Variable> variable_opt) noexcept;

}

#endif
