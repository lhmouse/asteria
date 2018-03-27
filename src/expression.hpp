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
		type_prefix_operator                     = 0,
		type_id_expression_with_trailer_opt      = 1,
		type_lambda_expression_with_trailer_opt  = 2,
		type_nested_expression_with_trailer_opt  = 3,
	};
	struct Prefix_operator {
		Operator prefix_operator;
		Value_ptr<Expression> next;
	};
	struct Id_expression_with_trailer_opt {
		boost::variant<std::string, Value_ptr<Initializer>> id_or_initializer;
		Value_ptr<Trailer> trailer_opt;
	};
	struct Lambda_expression_with_trailer_opt {
		boost::container::vector<std::string> parameter_list;
		Value_ptr_vector<Statement> function_body;
		Value_ptr<Trailer> trailer_opt;
	};
	struct Nested_expression_with_trailer_opt {
		Value_ptr<Expression> nested_expression;
		Value_ptr<Trailer> trailer_opt;
	};
	using Types = Type_tuple< Prefix_operator                     // 0
	                        , Id_expression_with_trailer_opt      // 1
	                        , Lambda_expression_with_trailer_opt  // 2
	                        , Nested_expression_with_trailer_opt  // 3
		>;

	enum Trailer_type : unsigned {
		trailer_type_infix_operator     = 0,
		trailer_type_ternary_operator   = 1,
		trailer_type_postfix_operator   = 2,
		trailer_type_function_call      = 3,
		trailer_type_subscripting       = 4,
		trailer_type_member_access      = 5,
	};
	struct Infix_operator {
		Operator infix_operator;
		Value_ptr<Expression> next;
	};
	struct Ternary_operator {
		Value_ptr<Expression> true_branch_opt;
		Value_ptr<Expression> false_branch;
	};
	struct Postfix_operator {
		Operator postfix_operator;
		Value_ptr<Trailer> next_trailer_opt;
	};
	struct Function_call {
		Value_ptr_vector<Expression> argument_list;
		Value_ptr<Trailer> next_trailer_opt;
	};
	struct Subscripting {
		Value_ptr<Expression> subscript;
		Value_ptr<Trailer> next_trailer_opt;
	};
	struct Member_access {
		std::string key;
		Value_ptr<Trailer> next_trailer_opt;
	};
	using Trailer_types = Type_tuple< Infix_operator     // 0
	                                , Ternary_operator   // 1
	                                , Postfix_operator   // 2
	                                , Function_call      // 3
	                                , Subscripting       // 4
	                                , Member_access      // 5
		>;

	struct Trailer : Trailer_types::rebound_variant {
		using Trailer_types::rebound_variant::variant;
	};

private:
	Types::rebound_variant m_value;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Expression, ValueT)>
	Expression(ValueT &&value)
		: m_value(std::forward<ValueT>(value))
	{ }
	~Expression();

public:
	Reference evaluate(const std::shared_ptr<Scope> &scope) const;
};

}

#endif
