// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HPP_
#define ASTERIA_VARIABLE_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

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
	using Variant = rocket::variant<ASTERIA_CDR(void
		, D_boolean   //  0
		, D_integer   //  1
		, D_double    //  2
		, D_string    //  3
		, D_opaque    //  4
		, D_function  //  5
		, D_array     //  6
		, D_object    //  7
	)>;

	enum Comparison_result : unsigned {
		comparison_result_unordered  = 0,
		comparison_result_less       = 1,
		comparison_result_equal      = 2,
		comparison_result_greater    = 3,
	};

private:
	const Wptr<Recycler> m_weak_recycler;
	Variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Variable, ValueT)>
	Variable(ValueT &&value)
		: m_weak_recycler(), m_variant(std::forward<ValueT>(value))
	{ }
	template<typename ValueT>
	Variable(Wptr<Recycler> weak_recycler, ValueT &&value)
		: m_weak_recycler(std::move(weak_recycler)), m_variant(std::forward<ValueT>(value))
	{ }
	~Variable();

	Variable(const Variable &) = delete;
	Variable & operator=(const Variable &) = delete;

private:
	ROCKET_NORETURN void do_throw_type_mismatch(Type expect) const;

public:
	Sptr<Recycler> get_recycler_opt() const noexcept {
		return m_weak_recycler.lock();
	}

	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.index());
	}
	template<typename ExpectT>
	const ExpectT * get_opt() const noexcept {
		return m_variant.try_get<ExpectT>();
	}
	template<typename ExpectT>
	ExpectT * get_opt() noexcept {
		return m_variant.try_get<ExpectT>();
	}
	template<typename ExpectT>
	const ExpectT & get() const {
		const auto ptr = m_variant.try_get<ExpectT>();
		if(!ptr){
			do_throw_type_mismatch(static_cast<Type>(Variant::index_of<ExpectT>::value));
		}
		return *ptr;
	}
	template<typename ExpectT>
	ExpectT & get(){
		const auto ptr = m_variant.try_get<ExpectT>();
		if(!ptr){
			do_throw_type_mismatch(static_cast<Type>(Variant::index_of<ExpectT>::value));
		}
		return *ptr;
	}
	template<typename ValueT>
	void set(ValueT &&value){
		m_variant = std::forward<ValueT>(value);
	}
};

extern const char * get_type_name(Variable::Type type) noexcept;

extern Variable::Type get_variable_type(Sparg<const Variable> variable_opt) noexcept;
extern const char * get_variable_type_name(Sparg<const Variable> variable_opt) noexcept;

extern bool test_variable(Sparg<const Variable> variable_opt) noexcept;
extern void dump_variable(std::ostream &os, Sparg<const Variable> variable_opt, unsigned indent_next = 0, unsigned indent_increment = 2);
extern std::ostream & operator<<(std::ostream &os, const Sptr_fmt<Variable> &variable_fmt);

extern void copy_variable(Xptr<Variable> &variable_out, Sparg<Recycler> recycler, Sparg<const Variable> source_opt);
extern void move_variable(Xptr<Variable> &variable_out, Sparg<Recycler> recycler, Xptr<Variable> &&source_opt);

// This function is useful for breaking dependency circles.
extern void purge_variable(Sparg<Variable> variable_opt) noexcept;

extern Variable::Comparison_result compare_variables(Sparg<const Variable> lhs_opt, Sparg<const Variable> rhs_opt) noexcept;

}

#endif
