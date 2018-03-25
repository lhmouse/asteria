// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STATEMENT_HPP_
#define ASTERIA_STATEMENT_HPP_

namespace Asteria {

class Statement {
public:
	enum Category : unsigned char {
		category_empty_statement         =  0,

		category_variable_definition     =  1,
		category_constant_definition     =  2,
		category_using_declaration       =  3,
		category_function_definition     =  4,

		category_if_statement            =  5,
		category_else_statement          =  6,
		category_do_while_statement      =  7,
		category_while_statement         =  8,
		category_for_statement           =  9,
		category_foreach_statement       = 10,
		category_switch_statement        = 11,

		category_try_statement           = 12,
		category_defer_statement         = 13,

		category_label_statement         = 14,
		category_case_statement          = 15,

		category_goto_statement          = 16,
		category_break_statement         = 17,
		category_continue_statement      = 18,
		category_throw_statement         = 19,
		category_return_statement        = 20,
		category_compound_statement      = 21,
		category_expression_statement    = 22,
	};

private:

};

}

#endif
