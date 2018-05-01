// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STATEMENT_HPP_
#define ASTERIA_STATEMENT_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Statement {
	friend Block;

public:
	enum Type : unsigned {
		type_expression_statement     =  0,
		type_variable_definition      =  1,
		type_function_definition      =  2,
		type_if_statement             =  3,
		type_switch_statement         =  4,
		type_do_while_statement       =  5,
		type_while_statement          =  6,
		type_for_statement            =  7,
		type_foreach_statement        =  8,
		type_try_statement            =  9,
		type_defer_statement          = 10,
		type_case_label_statement     = 11,
		type_default_label_statement  = 12,
		type_break_statement          = 13,
		type_continue_statement       = 14,
		type_throw_statement          = 15,
		type_return_statement         = 16,
	};
	struct S_expression_statement {
		Xptr<Expression> expression_opt;
	};
	struct S_variable_definition {
		std::string identifier;
		bool immutable;
		Xptr<Initializer> initializer_opt;
	};
	struct S_function_definition {
		std::string identifier;
		Sptr<const std::vector<Parameter>> parameters_opt;
		Xptr<Block> body_opt;
	};
	struct S_if_statement {
		Xptr<Expression> condition_opt;
		Xptr<Block> branch_true_opt;
		Xptr<Block> branch_false_opt;
	};
	struct S_switch_statement {
		Xptr<Expression> control_expression_opt;
		Xptr<Block> body_opt;
	};
	struct S_do_while_statement {
		Xptr<Block> body_opt;
		Xptr<Expression> condition_opt;
	};
	struct S_while_statement {
		Xptr<Expression> condition_opt;
		Xptr<Block> body_opt;
	};
	struct S_for_statement {
		Xptr<Statement> initialization_opt;
		Xptr<Expression> condition_opt;
		Xptr<Expression> increment_opt;
		Xptr<Block> body_opt;
	};
	struct S_foreach_statement {
		std::string key_identifier;
		std::string value_identifier;
		Xptr<Initializer> range_initializer;
		Xptr<Block> body_opt;
	};
	struct S_try_statement {
		Xptr<Block> branch_try_opt;
		std::string exception_identifier;
		Xptr<Block> branch_catch_opt;
	};
	struct S_defer_statement {
		Xptr<Block> body_opt;
	};
	struct S_case_label_statement {
		Sptr<const Variable> value_opt;
	};
	struct S_default_label_statement {
		// Nothing.
	};
	struct S_break_statement {
		// Nothing.
	};
	struct S_continue_statement {
		// Nothing.
	};
	struct S_throw_statement {
		Xptr<Expression> operand_opt;
	};
	struct S_return_statement {
		Xptr<Expression> operand_opt;
	};
	using Variant = rocket::variant<ASTERIA_CDR(void
		, S_expression_statement    //  0
		, S_variable_definition     //  1
		, S_function_definition     //  2
		, S_if_statement            //  3
		, S_switch_statement        //  4
		, S_do_while_statement      //  5
		, S_while_statement         //  6
		, S_for_statement           //  7
		, S_foreach_statement       //  8
		, S_try_statement           //  9
		, S_defer_statement         // 10
		, S_case_label_statement    // 11
		, S_default_label_statement // 12
		, S_break_statement         // 13
		, S_continue_statement      // 14
		, S_throw_statement         // 15
		, S_return_statement        // 16
	)>;

private:
	Variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Statement, ValueT)>
	Statement(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }
	Statement(Statement &&) noexcept;
	Statement &operator=(Statement &&);
	~Statement();

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.index());
	}
	template<typename ExpectT>
	const ExpectT *get_opt() const noexcept {
		return m_variant.try_get<ExpectT>();
	}
	template<typename ExpectT>
	const ExpectT &get() const {
		return m_variant.get<ExpectT>();
	}
};

}

#endif
