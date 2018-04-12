// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_NODE_HPP_
#define ASTERIA_EXPRESSION_NODE_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Expression_node {
	friend Expression;

public:
	enum Operator_generic : unsigned {
		// Postfix operators
		operator_postfix_inc    = 10, // ++
		operator_postfix_dec    = 11, // --
		operator_postfix_at     = 12, // []
		// Prefix operators.
		operator_prefix_pos     = 30, // +
		operator_prefix_neg     = 31, // -
		operator_prefix_not_b   = 32, // ~
		operator_prefix_not_l   = 33, // !
		operator_prefix_inc     = 34, // ++
		operator_prefix_dec     = 35, // --
		// Infix relational operators.
		operator_infix_cmp_eq   = 50, // ==
		operator_infix_cmp_ne   = 51, // !=
		operator_infix_cmp_lt   = 52, // <
		operator_infix_cmp_gt   = 53, // >
		operator_infix_cmp_lte  = 54, // <=
		operator_infix_cmp_gte  = 55, // >=
		// Infix arithmetic operators.
		operator_infix_add      = 60, // +
		operator_infix_sub      = 61, // -
		operator_infix_mul      = 62, // *
		operator_infix_div      = 63, // /
		operator_infix_mod      = 64, // %
		operator_infix_sll      = 65, // <<
		operator_infix_srl      = 66, // >>>
		operator_infix_sra      = 67, // >>
		operator_infix_and_b    = 68, // &
		operator_infix_or_b     = 69, // |
		operator_infix_xor_b    = 70, // ^
		operator_infix_assign   = 71, // =
	};

	enum Type : unsigned {
		type_literal            = 0, // +1
		type_named_reference    = 1, // +1
		type_subexpression      = 2, // +1
		type_lambda_definition  = 3, // +1
		type_pruning            = 4, // -X
		type_branch             = 5, // -1, +1
		type_function_call      = 6, // -X, +1
		type_operator_rpn       = 7, // -X, +1
	};
	struct S_literal {
		Sptr<const Variable> source_opt;
	};
	struct S_named_reference {
		std::string identifier;
	};
	struct S_subexpression {
		Xptr<Expression> subexpression_opt;
	};
	struct S_lambda_definition {
		// TODO
	};
	struct S_pruning {
		std::size_t count_to_pop;
	};
	struct S_branch {
		Xptr<Expression> branch_true_opt;
		Xptr<Expression> branch_false_opt;
	};
	struct S_function_call {
		std::size_t argument_count;
	};
	struct S_operator_rpn {
		Operator_generic operator_generic;
		bool compound_assignment; // This parameter is ignored for `++`, `--`, `[]`, `=` and all rational operators.
	};
	using Types = Type_tuple< S_literal            // 0
	                        , S_named_reference    // 1
	                        , S_subexpression      // 2
	                        , S_lambda_definition  // 3
	                        , S_pruning            // 4
	                        , S_branch             // 5
	                        , S_function_call      // 6
	                        , S_operator_rpn       // 7
		>;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Expression_node, ValueT)>
	Expression_node(ValueT &&variant)
		: m_variant(std::forward<ValueT>(variant))
	{ }
	Expression_node(Expression_node &&);
	Expression_node &operator=(Expression_node &&);
	~Expression_node();

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.which());
	}
	template<typename ExpectT>
	const ExpectT *get_opt() const noexcept {
		return boost::get<ExpectT>(&m_variant);
	}
	template<typename ExpectT>
	const ExpectT &get() const {
		return boost::get<ExpectT>(m_variant);
	}
};

extern const char *get_operator_name_generic(Expression_node::Operator_generic operator_generic) noexcept;

}

#endif
