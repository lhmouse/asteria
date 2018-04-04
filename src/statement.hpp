// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STATEMENT_HPP_
#define ASTERIA_STATEMENT_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Statement {
public:
	enum Type : unsigned {
		type_expression_statement     =  0,
		type_compound_statement       =  1,
		type_variable_definition      =  2,
		type_function_definition      =  3,
		type_if_statement             =  4,
		type_switch_statement         =  5,
		type_do_while_statement       =  6,
		type_while_statement          =  7,
		type_for_statement            =  8,
		type_foreach_statement        =  9,
		type_try_statement            = 10,
		type_defer_statement          = 11,
		type_label_statement          = 12,
		type_case_label_statement     = 13,
		type_default_label_statement  = 14,
		type_goto_statement           = 15,
		type_break_statement          = 16,
		type_continue_statement       = 17,
		type_throw_statement          = 18,
		type_return_statement         = 19,
	};
	struct S_expression_statement {
		Xptr<Expression> expression_opt;
	};
	struct S_compound_statement {
		Xptr_vector<Statement> statement_list;
	};
	struct S_variable_definition {
		std::string identifier;
		bool immutable;
		Xptr<Initializer> initializer_opt;
	};
	struct S_function_definition {
		// TODO
	};
	struct S_if_statement {
		Xptr<Expression> condition_opt;
		Xptr_vector<Statement> true_branch;
		Xptr<S_if_statement> false_branch_opt;
	};
	struct S_switch_statement {
		Xptr<Expression> control_expression;
		Xptr_vector<Statement> statement_list;
	};
	struct S_do_while_statement {
		Xptr_vector<Statement> statement_list;
		Xptr<Expression> condition_opt;
	};
	struct S_while_statement {
		Xptr<Expression> condition_opt;
		Xptr_vector<Statement> statement_list;
	};
	struct S_for_statement {
		Xptr<Statement> initialization_opt;
		Xptr<Expression> condition_opt;
		Xptr<Expression> increment_opt;
		Xptr_vector<Statement> statement_list;
	};
	struct S_foreach_statement {
		std::string key_identifier;
		std::string value_identifier;
		bool value_immutable;
		Xptr<Initializer> range_initializer;
		Xptr_vector<Statement> statement_list;
	};
	struct S_try_statement {
		Xptr_vector<Statement> try_statement_list;
		std::string exception_identifier;
		bool exception_immutable;
		Xptr_vector<Statement> catch_statement_list;
	};
	struct S_defer_statement {
		Xptr_vector<Statement> statement_list;
	};
	struct S_label_statement {
		std::string identifier;
	};
	struct S_case_label_statement {
		boost::variant<D_null, D_boolean, D_integer, D_double, D_string> value_opt;
	};
	struct S_default_label_statement {
		// Nothing.
	};
	struct S_goto_statement {
		std::string target_label;
	};
	struct S_break_statement {
		// Nothing.
	};
	struct S_continue_statement {
		// Nothing.
	};
	struct S_throw_statement {
		Xptr<Expression> value_opt;
	};
	struct S_return_statement {
		Xptr<Expression> value_opt;
	};
	using Types = Type_tuple< S_expression_statement     //  0
	                        , S_compound_statement       //  1
	                        , S_variable_definition      //  2
	                        , S_function_definition      //  3
	                        , S_if_statement             //  4
	                        , S_switch_statement         //  5
	                        , S_do_while_statement       //  6
	                        , S_while_statement          //  7
	                        , S_for_statement            //  8
	                        , S_foreach_statement        //  9
	                        , S_try_statement            // 10
	                        , S_defer_statement          // 11
	                        , S_label_statement          // 12
	                        , S_case_label_statement     // 13
	                        , S_default_label_statement  // 14
	                        , S_goto_statement           // 15
	                        , S_break_statement          // 16
	                        , S_continue_statement       // 17
	                        , S_throw_statement          // 18
	                        , S_return_statement         // 19
		>;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Statement, ValueT)>
	Statement(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }
	Statement(Statement &&);
	Statement &operator=(Statement &&);
	~Statement();

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
