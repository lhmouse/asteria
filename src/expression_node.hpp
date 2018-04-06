// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_NODE_HPP_
#define ASTERIA_EXPRESSION_NODE_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Expression_node {
public:
	enum Operator_generic : unsigned {
		// Postfix operators
		operator_postfix_inc   = 10, // ++
		operator_postfix_dec   = 11, // --
		operator_postfix_at    = 12, // []
		// Prefix operators.
		operator_prefix_add    = 30, // +
		operator_prefix_sub    = 31, // -
		operator_prefix_not_b  = 32, // ~
		operator_prefix_not_l  = 33, // !
		operator_prefix_inc    = 34, // ++
		operator_prefix_dec    = 35, // --
		// Infix relational operators.
		operator_infix_cmp_eq  = 50, // ==
		operator_infix_cmp_ne  = 51, // !=
		operator_infix_cmp_lt  = 52, // <
		operator_infix_cmp_gt  = 53, // >
		operator_infix_cmp_lte = 54, // <=
		operator_infix_cmp_gte = 55, // >=
		// Infix arithmetic operators.
		operator_infix_add     = 60, // +
		operator_infix_sub     = 61, // -
		operator_infix_mul     = 62, // *
		operator_infix_div     = 63, // /
		operator_infix_mod     = 64, // %
		operator_infix_sll     = 65, // <<
		operator_infix_srl     = 66, // >>>
		operator_infix_sra     = 67, // >>
		operator_infix_and_b   = 68, // &
		operator_infix_or_b    = 69, // |
		operator_infix_xor_b   = 70, // ^
		operator_infix_assign  = 71, // =
	};

	enum Type : unsigned {
		type_literal            = 0,
		type_named_reference    = 1,
		type_subexpression      = 2,
		type_branch             = 3,
		type_function_call      = 4,
		type_operator_rpn       = 5,
		type_lambda_definition  = 6,
	};
	struct S_literal {
		// Consumes: Nothing.
		// Produces: An rvalue.
		Sptr<const Variable> source_opt;
	};
	struct S_named_reference {
		// Consumes: Nothing.
		// Produces: An lvalue.
		std::string identifier;
	};
	struct S_subexpression {
		// Consumes: Nothing.
		// Produces: A reference.
		Xptr<Expression> subexpression_opt;
	};
	struct S_branch {
		// Consumes: A reference.
		// Produces: A reference.
		Xptr<Expression> branch_true_opt;
		Xptr<Expression> branch_false_opt;
	};
	struct S_function_call {
		// Consumes: A reference followed by the specified number of references.
		// Produces: A reference.
		std::size_t number_of_arguments;
	};
	struct S_operator_rpn {
		// Consumes: One or two references.
		// Produces: A reference.
		Operator_generic operator_generic;
		bool compound_assignment;
	};
	struct S_lambda_definition {
		// Consumes: Nothing.
		// Produces: An rvalue.
		// TODO
	};
	using Types = Type_tuple< S_literal            // 0
	                        , S_named_reference    // 1
	                        , S_subexpression      // 2
	                        , S_branch             // 3
	                        , S_function_call      // 4
	                        , S_operator_rpn       // 5
	                        , S_lambda_definition  // 6
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
	ExpectT *get_opt() noexcept {
		return boost::get<ExpectT>(&m_variant);
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

}

#endif
