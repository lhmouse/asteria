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
		operator_prefix_add     = 100,  // +
		operator_prefix_sub     = 101,  // -
		operator_prefix_not     = 102,  // ~
		operator_prefix_not_l   = 103,  // !
		operator_prefix_inc     = 104,  // ++
		operator_prefix_dec     = 105,  // --
		operator_infix_add      = 200,  // +
		operator_infix_sub      = 201,  // -
		operator_infix_mul      = 202,  // *
		operator_infix_div      = 203,  // /
		operator_infix_mod      = 204,  // %
		operator_infix_sll      = 205,  // <<
		operator_infix_srl      = 206,  // >>>
		operator_infix_sra      = 207,  // >>
		operator_infix_and      = 208,  // &
		operator_infix_or       = 209,  // |
		operator_infix_xor      = 210,  // ^
		operator_infix_and_l    = 211,  // &&
		operator_infix_or_l     = 212,  // ||
		operator_infix_add_a    = 213,  // *=
		operator_infix_sub_a    = 214,  // *=
		operator_infix_mul_a    = 215,  // *=
		operator_infix_div_a    = 216,  // /=
		operator_infix_mod_a    = 217,  // %=
		operator_infix_sll_a    = 218,  // <<=
		operator_infix_srl_a    = 219,  // >>>=
		operator_infix_sra_a    = 220,  // >>=
		operator_infix_and_a    = 221,  // &=
		operator_infix_or_a     = 222,  // |=
		operator_infix_xor_a    = 223,  // ^=
		operator_infix_and_l_a  = 224,  // &&=
		operator_infix_or_l_a   = 225,  // ||=
		operator_infix_assign   = 226,  // =
		operator_infix_cmpeq    = 227,  // ==
		operator_infix_cmpne    = 228,  // !=
		operator_infix_cmplt    = 229,  // <
		operator_infix_cmpgt    = 230,  // >
		operator_infix_cmplte   = 231,  // <=
		operator_infix_cmpgte   = 232,  // >=
		operator_postfix_inc    = 300,  // ++
		operator_postfix_dec    = 301,  // --
	};

	struct Trailer;

	enum Type : unsigned {
		type_unary_expression                    = 0,
		type_id_expression_with_trailer_opt      = 1,
		type_lambda_expression_with_trailer_opt  = 2,
		type_nested_expression_with_trailer_opt  = 3,
	};
	struct Unary_expression {
		Operator operator_prefix;
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
		trailer_type_binary_remainder   = 0,
		trailer_type_ternary_remainder  = 1,
		trailer_type_postfix_remainder  = 2,
		trailer_type_function_call      = 3,
		trailer_type_subscripting       = 4,
		trailer_type_member_access      = 5,
	};
	struct Binary_remainder {
		Operator operator_infix;
		Value_ptr<Expression> next;
	};
	struct Ternary_remainder {
		Value_ptr<Expression> true_branch_opt;
		Value_ptr<Expression> false_branch;
	};
	struct Postfix_remainder {
		Operator operator_postfix;
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
	using Trailer_types = Type_tuple< Binary_remainder   // 0
	                                , Ternary_remainder  // 1
	                                , Postfix_remainder  // 2
	                                , Function_call      // 3
	                                , Subscripting       // 4
	                                , Member_access      // 5
		>;
	struct Trailer : Trailer_types::rebind_as_variant {
		using Trailer_types::rebind_as_variant::variant;
	};

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

	Reference evaluate(const Shared_ptr<Recycler> &recycler, const Shared_ptr<Scope> &scope) const;
};

}

#endif
