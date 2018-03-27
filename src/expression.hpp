// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_HPP_
#define ASTERIA_EXPRESSION_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Expression {
public:
	enum Operator : unsigned {
		operator_add     =  0,  // +
		operator_sub     =  1,  // -
		operator_not     =  2,  // ~
		operator_not_l   =  3,  // !
		operator_inc     =  4,  // ++
		operator_dec     =  5,  // --
		operator_mul     =  6,  // *
		operator_div     =  7,  // /
		operator_mod     =  8,  // %
		operator_sll     =  9,  // <<
		operator_srl     = 10,  // >>>
		operator_sra     = 11,  // >>
		operator_and     = 12,  // &
		operator_or      = 13,  // |
		operator_xor     = 14,  // ^
		operator_and_l   = 15,  // &&
		operator_or_l    = 16,  // ||
		operator_add_a   = 17,  // +=
		operator_sub_a   = 18,  // -=
		operator_mul_a   = 19,  // *=
		operator_div_a   = 20,  // /=
		operator_mod_a   = 21,  // %=
		operator_sll_a   = 22,  // <<=
		operator_srl_a   = 23,  // >>>=
		operator_sra_a   = 24,  // >>=
		operator_and_a   = 25,  // &=
		operator_or_a    = 26,  // |=
		operator_xor_a   = 27,  // ^=
		operator_and_la  = 28,  // &&=
		operator_or_la   = 29,  // ||=
		operator_assign  = 30,  // =
		operator_eq      = 31,  // ==
		operator_ne      = 32,  // !=
		operator_lt      = 33,  // <
		operator_gt      = 34,  // >
		operator_lte     = 35,  // <=
		operator_gte     = 36,  // >=
	};

	struct Trailer;

	enum Type : unsigned {
		type_prefix_expression                  = 0,
		type_id_expression_with_trailer_opt     = 1,
		type_lambda_expression_with_trailer_opt = 2,
		type_nested_expression_with_trailer_opt = 3,
	};

	using Prefix_expression                  = std::tuple<Operator, Value_ptr<Expression>>;
	using Id_expression_with_trailer_opt     = std::tuple<boost::variant<std::string, Value_ptr<Initializer>>, Value_ptr<Trailer>>;
	using Lambda_expression_with_trailer_opt = std::tuple<boost::container::deque<std::string>, Value_ptr_deque<Statement>, Value_ptr<Trailer>>;
	using Nested_expression_with_trailer_opt = std::tuple<Value_ptr<Expression>, Value_ptr<Trailer>>;

	using Types = Type_tuple< Prefix_expression                   // 0
	                        , Id_expression_with_trailer_opt      // 1
	                        , Lambda_expression_with_trailer_opt  // 2
	                        , Nested_expression_with_trailer_opt  // 3
		>;

private:
	Types::rebound_variant m_value;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Expression, ValueT)>
	Expression(ValueT &&value)
		: m_value(std::forward<ValueT>(value))
	{ }
	~Expression();

public:
	Reference evaluate() const;
};

}

#endif
