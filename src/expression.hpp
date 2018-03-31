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
		prefix_operator_add    = 10,  // +
		prefix_operator_sub    = 11,  // -
		prefix_operator_not    = 12,  // ~
		prefix_operator_not_l  = 13,  // !
		prefix_operator_inc    = 14,  // ++
		prefix_operator_dec    = 15,  // --
	};
	enum Postfix_operator : unsigned {
		postfix_operator_inc   = 30,  // ++
		postfix_operator_dec   = 31,  // --
	};
	enum Infix_operator : unsigned {
		infix_operator_add     = 50,  // +
		infix_operator_sub     = 51,  // -
		infix_operator_mul     = 52,  // *
		infix_operator_div     = 53,  // /
		infix_operator_mod     = 54,  // %
		infix_operator_sll     = 55,  // <<
		infix_operator_srl     = 56,  // >>>
		infix_operator_sra     = 57,  // >>
		infix_operator_and     = 58,  // &
		infix_operator_or      = 59,  // |
		infix_operator_xor     = 60,  // ^
		infix_operator_and_l   = 61,  // &&
		infix_operator_or_l    = 62,  // ||
		infix_operator_cmpeq   = 63,  // ==
		infix_operator_cmpne   = 64,  // !=
		infix_operator_cmplt   = 65,  // <
		infix_operator_cmpgt   = 66,  // >
		infix_operator_cmplte  = 67,  // <=
		infix_operator_cmpgte  = 68,  // >=
		infix_operator_assign  = 69,  // =
	};

	class Initiator;
	class Trailer;

	enum Type : unsigned {
		type_prefix_expression       = 0,
		type_initiator_and_trailers  = 1,
	};
	struct Prefix_expression {
		Prefix_operator prefix_operator;
		Xptr<Expression> operand;
	};
	struct Initiator_and_trailers {
		Xptr<Initiator> initiator;
		Xptr<Trailer> trailer_first_opt;
	};
	using Types = Type_tuple< Prefix_expression       // 0
	                        , Initiator_and_trailers  // 1
		>;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Expression, ValueT)>
	Expression(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }

	Expression(const Expression &);
	Expression &operator=(const Expression &);
	Expression(Expression &&);
	Expression &operator=(Expression &&);
	~Expression();

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

class Expression::Initiator {
public:
	enum Type : unsigned {
		type_identifier         = 0,
		type_literal            = 1,
		type_lambda_expression  = 2,
		type_nested_expression  = 3,
	};
	struct Identifier {
		std::string identifier;
	};
	struct Literal {
		Xptr<Variable> value;
	};
	struct Lambda_expression {
		boost::container::vector<Function_parameter> parameter_list;
		// TODO Xptr_vector<Statement> body_statement_list;
	};
	struct Nested_expression {
		Xptr<Expression> nested_expression;
	};
	using Types = Type_tuple< Identifier         // 0
	                        , Literal            // 1
	                        , Lambda_expression  // 2
	                        , Nested_expression  // 3
		>;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Initiator, ValueT)>
	Initiator(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }

	Initiator(const Initiator &);
	Initiator &operator=(const Initiator &);
	Initiator(Initiator &&);
	Initiator &operator=(Initiator &&);
	~Initiator();

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

class Expression::Trailer {
public:
	enum Type : unsigned {
		type_postfix_trailer        = 0,
		type_infix_trailer          = 1,
		type_ternary_trailer        = 2,
		type_assignment_trailer     = 3,
		type_function_call_trailer  = 4,
		type_subscripting_trailer   = 5,
		type_member_access_trailer  = 6,
	};
	struct Postfix_trailer {
		Postfix_operator postfix_operator;
		Xptr<Trailer> trailer_next_opt;
	};
	struct Infix_trailer {
		Infix_operator infix_operator;
		Xptr<Expression> operand_next;
	};
	struct Ternary_trailer {
		Xptr<Expression> branch_true_opt;
		Xptr<Expression> branch_false;
	};
	struct Assignment_trailer {
		Infix_operator infix_operator;
		Xptr<Expression> operand_next;
	};
	struct Function_call_trailer {
		Xptr_vector<Expression> argument_list_opt;
		Xptr<Trailer> trailer_next_opt;
	};
	struct Subscripting_trailer {
		Xptr<Expression> subscript;
		Xptr<Trailer> trailer_next_opt;
	};
	struct Member_access_trailer {
		std::string identifier;
		Xptr<Trailer> trailer_next_opt;
	};
	using Types = Type_tuple< Postfix_trailer        // 0
	                        , Infix_trailer          // 1
	                        , Ternary_trailer        // 2
	                        , Assignment_trailer     // 3
	                        , Function_call_trailer  // 4
	                        , Subscripting_trailer   // 5
	                        , Member_access_trailer  // 6
		>;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Trailer, ValueT)>
	Trailer(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }

	Trailer(const Trailer &);
	Trailer &operator=(const Trailer &);
	Trailer(Trailer &&);
	Trailer &operator=(Trailer &&);
	~Trailer();

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

extern Reference evaluate_expression(Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Expression> expression_opt);

}

#endif
