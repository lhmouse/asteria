// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_HPP_
#define ASTERIA_EXPRESSION_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Expression {
public:
	enum Prefix_operator : unsigned {
		prefix_operator_add     = 10,  // +
		prefix_operator_sub     = 11,  // -
		prefix_operator_not     = 12,  // ~
		prefix_operator_not_l   = 13,  // !
		prefix_operator_inc     = 14,  // ++
		prefix_operator_dec     = 15,  // --
	};
	enum Infix_operator : unsigned {
		infix_operator_add      = 40,  // +
		infix_operator_sub      = 41,  // -
		infix_operator_mul      = 42,  // *
		infix_operator_div      = 43,  // /
		infix_operator_mod      = 44,  // %
		infix_operator_sll      = 45,  // <<
		infix_operator_srl      = 46,  // >>>
		infix_operator_sra      = 47,  // >>
		infix_operator_and      = 48,  // &
		infix_operator_or       = 49,  // |
		infix_operator_xor      = 50,  // ^
		infix_operator_and_l    = 51,  // &&
		infix_operator_or_l     = 52,  // ||
		infix_operator_add_a    = 53,  // *=
		infix_operator_sub_a    = 54,  // *=
		infix_operator_mul_a    = 55,  // *=
		infix_operator_div_a    = 56,  // /=
		infix_operator_mod_a    = 57,  // %=
		infix_operator_sll_a    = 58,  // <<=
		infix_operator_srl_a    = 59,  // >>>=
		infix_operator_sra_a    = 60,  // >>=
		infix_operator_and_a    = 61,  // &=
		infix_operator_or_a     = 62,  // |=
		infix_operator_xor_a    = 63,  // ^=
		infix_operator_and_l_a  = 64,  // &&=
		infix_operator_or_l_a   = 65,  // ||=
		infix_operator_assign   = 66,  // =
		infix_operator_cmpeq    = 67,  // ==
		infix_operator_cmpne    = 68,  // !=
		infix_operator_cmplt    = 69,  // <
		infix_operator_cmpgt    = 70,  // >
		infix_operator_cmplte   = 71,  // <=
		infix_operator_cmpgte   = 72,  // >=
	};
	enum Postfix_operator : unsigned {
		postfix_operator_inc    = 90,  // ++
		postfix_operator_dec    = 91,  // --
	};

	class Initiator;
	class Trailer;

	enum Type : unsigned {
		type_prefix_operator_expression  = 0,
		type_initiator_and_trailer       = 1,
	};
	struct Prefix_operator_expression {
		Prefix_operator prefix_operator;
		Value_ptr<Expression> operand;
	};
	struct Initiator_and_trailer {
		Value_ptr<Initiator> initiator;
		Value_ptr<Trailer> trailer_opt;
	};
	using Types = Type_tuple< Prefix_operator_expression  // 0
	                        , Initiator_and_trailer       // 1
		>;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Expression, ValueT)>
	Expression(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }

	Expression(Expression &&);
	Expression &operator=(Expression &&);
	~Expression();

	Expression(const Expression &) = delete;
	Expression &operator=(const Expression &) = delete;

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.which());
	}
	template<typename ExpectT>
	const ExpectT &get() const {
		return boost::get<ExpectT>(m_variant);
	}
	template<typename ExpectT>
	ExpectT &get(){
		return boost::get<ExpectT>(m_variant);
	}
	template<typename ValueT>
	void set(ValueT &&value){
		m_variant = std::forward<ValueT>(value);
	}
};

class Expression::Initiator {
public:
	enum Type : unsigned {
		type_prefix_operator_expression  = 0,
		type_initiator_and_trailer       = 1,
	};
	struct Prefix_operator_expression {
		Prefix_operator prefix_operator;
		Value_ptr<Expression> operand;
	};
	struct Initiator_and_trailer {
		Value_ptr<Initiator> initiator;
		Value_ptr<Trailer> trailer_opt;
	};
	using Types = Type_tuple< Prefix_operator_expression  // 0
	                        , Initiator_and_trailer       // 1
		>;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Initiator, ValueT)>
	Initiator(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }

	Initiator(Initiator &&);
	Initiator &operator=(Initiator &&);
	~Initiator();

	Initiator(const Initiator &) = delete;
	Initiator &operator=(const Initiator &) = delete;

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.which());
	}
	template<typename ExpectT>
	const ExpectT &get() const {
		return boost::get<ExpectT>(m_variant);
	}
	template<typename ExpectT>
	ExpectT &get(){
		return boost::get<ExpectT>(m_variant);
	}
	template<typename ValueT>
	void set(ValueT &&value){
		m_variant = std::forward<ValueT>(value);
	}
};

extern Reference evaluate_expression(Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Expression> expression_opt);

}

#endif
