// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXPRESSION_HPP_
#define ASTERIA_EXPRESSION_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Expression {
public:
	enum Operator_generic : unsigned {
		// Postfix operators
		operator_postfix_inc   = 10, // ++
		operator_postfix_dec   = 11, // --
		// Prefix operators.
		operator_prefix_add    = 30, // +
		operator_prefix_sub    = 31, // -
		operator_prefix_not    = 32, // ~
		operator_prefix_not_l  = 33, // !
		operator_prefix_inc    = 34, // ++
		operator_prefix_dec    = 35, // --
		// Infix operators.
		operator_infix_add     = 60, // +
		operator_infix_sub     = 61, // -
		operator_infix_mul     = 62, // *
		operator_infix_div     = 63, // /
		operator_infix_mod     = 64, // %
		operator_infix_sll     = 65, // <<
		operator_infix_srl     = 66, // >>>
		operator_infix_sra     = 67, // >>
		operator_infix_and     = 68, // &
		operator_infix_or      = 69, // |
		operator_infix_xor     = 70, // ^
		operator_infix_and_l   = 71, // &&
		operator_infix_or_l    = 72, // ||
		operator_infix_cmpeq   = 73, // ==
		operator_infix_cmpne   = 74, // !=
		operator_infix_cmplt   = 75, // <
		operator_infix_cmpgt   = 76, // >
		operator_infix_cmplte  = 77, // <=
		operator_infix_cmpgte  = 78, // >=
		operator_infix_assign  = 79, // =
	};

	enum Type : unsigned {
		type_literal_generic          = 0,
		type_named_variable           = 1,
		type_operator_reverse_polish  = 2,
		type_function_call            = 3,
		type_subscripting_generic     = 4,
		type_lambda_definition        = 5,
	};
	struct Literal_generic {
		Sptr<const Variable> source_opt;
	};
	struct Named_variable {
		std::string identifier;
	};
	struct Operator_reverse_polish {
		Operator_generic operator_generic;
	};
	struct Function_call {
		Xptr_vector<Expression> argument_list_opt;
	};
	struct Subscripting_generic {
		Xptr<Expression> subscript_opt;
	};
	struct Lambda_definition {
		// TODO
	};
	using Types = Type_tuple< Literal_generic          // 0
	                        , Named_variable           // 1
	                        , Operator_reverse_polish  // 2
	                        , Function_call            // 3
	                        , Subscripting_generic     // 4
	                        , Lambda_definition        // 5
		>;

private:
	boost::container::vector<Types::rebind_as_variant> m_variant_list;

public:
	Expression()
		: m_variant_list()
	{ }

	Expression(Expression &&);
	Expression &operator=(Expression &&);
	~Expression();

public:
	bool is_empty() const noexcept {
		return m_variant_list.empty();
	}
	std::size_t get_size() const noexcept {
		return m_variant_list.size();
	}
	Type get_type_at(std::size_t n) const {
		return static_cast<Type>(m_variant_list.at(n).which());
	}
	template<typename ExpectT>
	const ExpectT *get_at_opt(std::size_t n) const {
		return boost::get<ExpectT>(&(m_variant_list.at(n)));
	}
	template<typename ExpectT>
	ExpectT *get_at_opt(std::size_t n){
		return boost::get<ExpectT>(&(m_variant_list.at(n)));
	}
	template<typename ExpectT>
	const ExpectT &get_at(std::size_t n) const {
		return boost::get<ExpectT>(m_variant_list.at(n));
	}
	template<typename ExpectT>
	ExpectT &get_at(std::size_t n){
		return boost::get<ExpectT>(m_variant_list.at(n));
	}
	template<typename ValueT>
	void set_at(std::size_t n, ValueT &&value){
		m_variant_list.at(n) = std::forward<ValueT>(value);
	}
	template<typename ValueT>
	void append(ValueT &&value){
		m_variant_list.emplace_back(std::forward<ValueT>(value));
	}
	void clear() noexcept {
		m_variant_list.clear();
	}
};

extern Reference evaluate_expression_recursive(Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Expression> expression_opt);

}

#endif
