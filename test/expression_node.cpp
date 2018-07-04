// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/expression_node.hpp"
#include "../src/statement.hpp"
#include "../src/scope.hpp"
#include "../src/recycler.hpp"
#include "../src/value.hpp"
#include "../src/stored_reference.hpp"

using namespace Asteria;

int main(){
	const auto recycler = std::make_shared<Recycler>();
	const auto scope = std::make_shared<Scope>(Scope::purpose_plain, nullptr);

	auto dval = std::make_shared<Variable>();
	set_value(dval->mutate_value(), recycler, D_double(1.5));
	Reference::S_variable lref = { dval };
	auto lrwref = scope->mutate_named_reference(D_string::shallow("dval"));
	set_reference(lrwref, std::move(lref));

	auto cval = std::make_shared<Variable>();
	set_value(cval->mutate_value(), recycler, D_integer(10));
	lref = { cval };
	lrwref = scope->mutate_named_reference(D_string::shallow("cval"));
	set_reference(lrwref, std::move(lref));

	auto rval = std::make_shared<Variable>();
	set_value(rval->mutate_value(), recycler, D_array());
	lref = { rval };
	lrwref = scope->mutate_named_reference(D_string::shallow("rval"));
	set_reference(lrwref, std::move(lref));

	// Plain: rval[1] = !condition ? (dval++ + 0.25) : (cval * "hello,");
	// RPN:   condition ! ?: 1 rval [] =          ::= expr
	//                    \+-- 0.25 dval ++ +     ::= branch_true
	//                     \-- "hello," cval *    ::= branch_false

	std::vector<Expression_node> branch_true;
	Expression_node::S_literal s_lit = { std::make_shared<Value>(D_double(0.25)) };
	branch_true.emplace_back(std::move(s_lit)); // 0.25
	Expression_node::S_named_reference s_nref = { D_string::shallow("dval") };
	branch_true.emplace_back(std::move(s_nref)); // dval
	Expression_node::S_operator_rpn s_opr = { Expression_node::operator_postfix_inc, false };
	branch_true.emplace_back(std::move(s_opr)); // ++
	s_opr = { Expression_node::operator_infix_add, false };
	branch_true.emplace_back(std::move(s_opr)); // +

	std::vector<Expression_node> branch_false;
	s_lit = { std::make_shared<Value>(D_string("hello,")) };
	branch_false.emplace_back(std::move(s_lit)); // "hello,"
	s_nref = { D_string::shallow("cval") };
	branch_false.emplace_back(std::move(s_nref)); // cval
	s_opr = { Expression_node::operator_infix_mul, false };
	branch_false.emplace_back(std::move(s_opr)); // *

	std::vector<Expression_node> expr;
	s_nref = { D_string::shallow("condition") };
	expr.emplace_back(std::move(s_nref)); // condition
	s_opr = { Expression_node::operator_prefix_notl, false };
	expr.emplace_back(std::move(s_opr)); // !
	Expression_node::S_branch s_br = { std::move(branch_true), std::move(branch_false) };
	expr.emplace_back(std::move(s_br)); // ?:
	s_lit = { std::make_shared<Value>(D_integer(1)) };
	expr.emplace_back(std::move(s_lit)); // 1
	s_nref = { D_string::shallow("rval") };
	expr.emplace_back(std::move(s_nref)); // rval
	s_opr = { Expression_node::operator_postfix_at, false };
	expr.emplace_back(std::move(s_opr)); // []
	s_opr = { Expression_node::operator_infix_assign, false };
	expr.emplace_back(std::move(s_opr)); // =

	auto condition = std::make_shared<Variable>();
	lref = { condition };
	lrwref = scope->mutate_named_reference(D_string::shallow("condition"));
	set_reference(lrwref, std::move(lref));

	set_value(condition->mutate_value(), recycler, D_boolean(false));
	Vp<Reference> result;
	evaluate_expression(result, recycler, expr, scope);
	ASTERIA_TEST_CHECK(dval->get_value_opt()->get<D_double>() == 2.5);
	ASTERIA_TEST_CHECK(cval->get_value_opt()->get<D_integer>() == 10);
	auto rptr = read_reference_opt(result);
	ASTERIA_TEST_CHECK(rval->get_value_opt()->get<D_array>().at(1).get() == rptr.get());
	ASTERIA_TEST_CHECK(rptr->get<D_double>() == 1.75);

	set_value(condition->mutate_value(), recycler, D_boolean(true));
	evaluate_expression(result, recycler, expr, scope);
	ASTERIA_TEST_CHECK(dval->get_value_opt()->get<D_double>() == 2.5);
	ASTERIA_TEST_CHECK(cval->get_value_opt()->get<D_integer>() == 10);
	rptr = read_reference_opt(result);
	ASTERIA_TEST_CHECK(rval->get_value_opt()->get<D_array>().at(1).get() == rptr.get());
	ASTERIA_TEST_CHECK(rptr->get<D_string>() == "hello,hello,hello,hello,hello,hello,hello,hello,hello,hello,");
}
