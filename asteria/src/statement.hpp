// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STATEMENT_HPP_
#define ASTERIA_STATEMENT_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Statement {
public:
	enum Target_scope : std::uint8_t {
		target_scope_unspecified  = 0,
		target_scope_switch       = 1,
		target_scope_while        = 2,
		target_scope_for          = 3,
	};
	enum Execution_result : std::uint8_t {
		execution_result_next                  = 0,
		execution_result_break_unspecified     = 1,
		execution_result_break_switch          = 2,
		execution_result_break_while           = 3,
		execution_result_break_for             = 4,
		execution_result_continue_unspecified  = 5,
		execution_result_continue_while        = 6,
		execution_result_continue_for          = 7,
		execution_result_return                = 8,
	};

	enum Type : signed char {
		type_expression_statement  =  0,
		type_variable_definition   =  1,
		type_function_definition   =  2,
		type_if_statement          =  3,
		type_switch_statement      =  4,
		type_do_while_statement    =  5,
		type_while_statement       =  6,
		type_for_statement         =  7,
		type_for_each_statement    =  8,
		type_try_statement         =  9,
		type_defer_statement       = 10,
		type_break_statement       = 11,
		type_continue_statement    = 12,
		type_throw_statement       = 13,
		type_return_statement      = 14,
	};
	struct S_expression_statement {
		Vp<Expression> expr_opt;
	};
	struct S_variable_definition {
		Cow_string id;
		bool constant;
		Vp<Initializer> init_opt;
	};
	struct S_function_definition {
		Cow_string id;
		Cow_string location;
		Sp_vector<const Parameter> params_opt;
		Vp<Block> body_opt;
	};
	struct S_if_statement {
		Vp<Expression> cond_opt;
		Vp<Block> branch_true_opt;
		Vp<Block> branch_false_opt;
	};
	struct S_switch_statement {
		Vp<Expression> ctrl_opt;
		T_vector<T_pair<Vp<Expression>, Vp<Block>>> clauses_opt;
	};
	struct S_do_while_statement {
		Vp<Block> body_opt;
		Vp<Expression> cond_opt;
	};
	struct S_while_statement {
		Vp<Expression> cond_opt;
		Vp<Block> body_opt;
	};
	struct S_for_statement {
		Vp<Block> init_opt;
		Vp<Expression> cond_opt;
		Vp<Expression> step_opt;
		Vp<Block> body_opt;
	};
	struct S_for_each_statement {
		Cow_string key_id;
		Cow_string value_id;
		Vp<Initializer> range_init_opt;
		Vp<Block> body_opt;
	};
	struct S_try_statement {
		Vp<Block> branch_try_opt;
		Cow_string except_id;
		Vp<Block> branch_catch_opt;
	};
	struct S_defer_statement {
		Cow_string location;
		Vp<Block> body_opt;
	};
	struct S_break_statement {
		Target_scope target_scope;
	};
	struct S_continue_statement {
		Target_scope target_scope;
	};
	struct S_throw_statement {
		Vp<Expression> operand_opt;
	};
	struct S_return_statement {
		Vp<Expression> operand_opt;
	};
	using Variant = rocket::variant<ASTERIA_CDR(void
		, S_expression_statement  //  0
		, S_variable_definition   //  1
		, S_function_definition   //  2
		, S_if_statement          //  3
		, S_switch_statement      //  4
		, S_do_while_statement    //  5
		, S_while_statement       //  6
		, S_for_statement         //  7
		, S_for_each_statement    //  8
		, S_try_statement         //  9
		, S_defer_statement       // 10
		, S_break_statement       // 11
		, S_continue_statement    // 12
		, S_throw_statement       // 13
		, S_return_statement      // 14
	)>;

private:
	Variant m_variant;

public:
	template<typename CandidateT, ASTERIA_ACCEPTABLE_BY_VARIANT(CandidateT, Variant)>
	Statement(CandidateT &&cand)
		: m_variant(std::forward<CandidateT>(cand))
	{ }
	Statement(Statement &&) noexcept;
	Statement & operator=(Statement &&) noexcept;
	~Statement();

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.index());
	}
	template<typename ExpectT>
	const ExpectT * get_opt() const noexcept {
		return m_variant.try_get<ExpectT>();
	}
	template<typename ExpectT>
	const ExpectT & get() const {
		return m_variant.get<ExpectT>();
	}
};

extern void bind_statement_in_place(T_vector<Statement> &bound_stmts_out, Spr<Scope> scope_inout, const Statement &stmt);
extern Statement::Execution_result execute_statement_in_place(Vp<Reference> &result_out, Spr<Scope> scope_inout, Spr<Recycler> recycler, const Statement &stmt);

}

#endif
