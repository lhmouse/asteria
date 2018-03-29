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
		prefix_operator_add     = 100,  // +
		prefix_operator_sub     = 101,  // -
		prefix_operator_not     = 102,  // ~
		prefix_operator_not_l   = 103,  // !
		prefix_operator_inc     = 104,  // ++
		prefix_operator_dec     = 105,  // --
	};
	enum Infix_operator : unsigned {
		infix_operator_add      = 200,  // *
		infix_operator_sub      = 201,  // *
		infix_operator_mul      = 202,  // *
		infix_operator_div      = 203,  // /
		infix_operator_mod      = 204,  // %
		infix_operator_sll      = 205,  // <<
		infix_operator_srl      = 206,  // >>>
		infix_operator_sra      = 207,  // >>
		infix_operator_and      = 208,  // &
		infix_operator_or       = 209,  // |
		infix_operator_xor      = 210,  // ^
		infix_operator_and_l    = 211,  // &&
		infix_operator_or_l     = 212,  // ||
		infix_operator_add_a    = 213,  // *
		infix_operator_sub_a    = 214,  // *
		infix_operator_mul_a    = 215,  // *
		infix_operator_div_a    = 216,  // /
		infix_operator_mod_a    = 217,  // %
		infix_operator_sll_a    = 218,  // <<
		infix_operator_srl_a    = 219,  // >>>
		infix_operator_sra_a    = 220,  // >>
		infix_operator_and_a    = 221,  // &
		infix_operator_or_a     = 222,  // |
		infix_operator_xor_a    = 223,  // ^
		infix_operator_and_l_a  = 224,  // &&
		infix_operator_or_l_a   = 225,  // ||
		infix_operator_assign   = 226,  // =
		infix_operator_cmpeq    = 227,  // ==
		infix_operator_cmpne    = 228,  // !=
		infix_operator_cmplt    = 229,  // <
		infix_operator_cmpgt    = 230,  // >
		infix_operator_cmplte   = 231,  // <=
		infix_operator_cmpgte   = 232,  // >=
	};
	enum Postfix_operator : unsigned {
		postfix_operator_inc    = 301,  // ++
		postfix_operator_dec    = 302,  // --
	};

	struct Trailer;

	enum Type : unsigned {
		type_unary_expression                    = 0,
		type_id_expression_with_trailer_opt      = 1,
		type_lambda_expression_with_trailer_opt  = 2,
		type_nested_expression_with_trailer_opt  = 3,
	};
	struct Unary_expression {
		Prefix_operator prefix_operator;
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
	using Types = Type_tuple< Unary_expression                    // 0
	                        , Id_expression_with_trailer_opt      // 1
	                        , Lambda_expression_with_trailer_opt  // 2
	                        , Nested_expression_with_trailer_opt  // 3
		>;

	enum Trailer_type : unsigned {
		trailer_type_binary_modifier    = 0,
		trailer_type_ternary_modifier   = 1,
		trailer_type_postfix_modifier   = 2,
		trailer_type_function_call      = 3,
		trailer_type_subscripting       = 4,
		trailer_type_member_access      = 5,
	};
	struct Binary_modifier {
		Infix_operator infix_operator;
		Value_ptr<Expression> next;
	};
	struct Ternary_modifier {
		Value_ptr<Expression> true_branch_opt;
		Value_ptr<Expression> false_branch;
	};
	struct Postfix_modifier {
		Postfix_operator postfix_operator;
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
	using Trailer_types = Type_tuple< Binary_modifier    // 0
	                                , Ternary_modifier   // 1
	                                , Postfix_modifier   // 2
	                                , Function_call      // 3
	                                , Subscripting       // 4
	                                , Member_access      // 5
		>;

	struct Trailer : Trailer_types::rebind_as_variant {
		using Trailer_types::rebind_as_variant::variant;
	};

private:
	Types::rebind_as_variant m_value;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Expression, ValueT)>
	Expression(ValueT &&value)
		: m_value(std::forward<ValueT>(value))
	{ }

	Expression(Expression &&);
	Expression &operator=(Expression &&);
	~Expression();

	Expression(const Expression &) = delete;
	Expression &operator=(const Expression &) = delete;

public:
	Reference evaluate(const Shared_ptr<Recycler> &recycler, const Shared_ptr<Scope> &scope) const;
};

}

#endif
