// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression_node.hpp"
#include "utilities.hpp"

namespace Asteria {

Expression_node::Expression_node(Expression_node &&) noexcept = default;
Expression_node &Expression_node::operator=(Expression_node &&) = default;
Expression_node::~Expression_node() = default;

const char * get_operator_name_generic(Expression_node::Operator_generic operator_generic) noexcept {
	switch(operator_generic){
	case Expression_node::operator_postfix_inc:
		return "postfix increment";
	case Expression_node::operator_postfix_dec:
		return "postfix decrement";
	case Expression_node::operator_postfix_at:
		return "subscripting";
	case Expression_node::operator_prefix_pos:
		return "unary plus";
	case Expression_node::operator_prefix_neg:
		return "negation";
	case Expression_node::operator_prefix_not_b:
		return "bitwise not";
	case Expression_node::operator_prefix_not_l:
		return "logical not";
	case Expression_node::operator_prefix_inc:
		return "prefix increment";
	case Expression_node::operator_prefix_dec:
		return "prefix decrement";
	case Expression_node::operator_infix_cmp_eq:
		return "equality comparison";
	case Expression_node::operator_infix_cmp_ne:
		return "inequality comparison";
	case Expression_node::operator_infix_cmp_lt:
		return "less-than comparison";
	case Expression_node::operator_infix_cmp_gt:
		return "greater-than comparison";
	case Expression_node::operator_infix_cmp_lte:
		return "less-than-or-equal comparison";
	case Expression_node::operator_infix_cmp_gte:
		return "greater-than-or-equal comparison";
	case Expression_node::operator_infix_add:
		return "addition";
	case Expression_node::operator_infix_sub:
		return "subtraction";
	case Expression_node::operator_infix_mul:
		return "multiplication";
	case Expression_node::operator_infix_div:
		return "division";
	case Expression_node::operator_infix_mod:
		return "modulo";
	case Expression_node::operator_infix_sll:
		return "logical left shift";
	case Expression_node::operator_infix_sla:
		return "arithmetic left shift";
	case Expression_node::operator_infix_srl:
		return "logical right shift";
	case Expression_node::operator_infix_sra:
		return "arithmetic right shift";
	case Expression_node::operator_infix_and_b:
		return "bitwise and";
	case Expression_node::operator_infix_or_b:
		return "bitwise or";
	case Expression_node::operator_infix_xor_b:
		return "bitwise xor";
	case Expression_node::operator_infix_assign:
		return "assginment";
	default:
		ASTERIA_DEBUG_LOG("Unknown operator enumeration `", operator_generic, "`. This is probably a bug, please report.");
		return "<unknown>";
	}
}

}
