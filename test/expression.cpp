// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/expression.hpp"
#include "../src/scope.hpp"
#include "../src/recycler.hpp"
#include "../src/stored_value.hpp"
#include "../src/stored_reference.hpp"
#include "../src/variable.hpp"

using namespace Asteria;

int main(){
	const auto recycler = std::make_shared<Recycler>();
	const auto scope = std::make_shared<Scope>(Scope::purpose_plain, nullptr);

	auto dval = std::make_shared<Variable>();
	set_value(dval->drill_for_value(), recycler, D_double(1.5));
	Reference::S_variable lref = { dval };
	auto lrwref = scope->drill_for_named_reference(D_string::shallow("dval"));
	set_reference(lrwref, std::move(lref));

	auto cval = std::make_shared<Variable>();
	set_value(cval->drill_for_value(), recycler, D_integer(10));
	lref = { cval };
	lrwref = scope->drill_for_named_reference(D_string::shallow("cval"));
	set_reference(lrwref, std::move(lref));

	auto rval = std::make_shared<Variable>();
	set_value(rval->drill_for_value(), recycler, D_array());
	lref = { rval };
	lrwref = scope->drill_for_named_reference(D_string::shallow("rval"));
	set_reference(lrwref, std::move(lref));

	// Plain: rval[1] = !condition ? (dval++ + 0.25) : (cval * "hello,");
	// RPN:   condition ! ?: 1 rval [] =          ::= expr
	//                    \+-- 0.25 dval ++ +     ::= branch_true
	//                     \-- "hello," cval *    ::= branch_false

	std::vector<Expression_node> nodes;
	Expression_node::S_literal s_lit = { std::make_shared<Value>(D_double(0.25)) };
	nodes.emplace_back(std::move(s_lit)); // 0.25
	Expression_node::S_named_reference s_nref = { D_string::shallow("dval") };
	nodes.emplace_back(std::move(s_nref)); // dval
	Expression_node::S_operator_rpn s_opr = { Expression_node::operator_postfix_inc, false };
	nodes.emplace_back(std::move(s_opr)); // ++
	s_opr = { Expression_node::operator_infix_add, false };
	nodes.emplace_back(std::move(s_opr)); // +
	auto branch_true = Vp<Expression>(std::make_shared<Expression>(std::move(nodes)));

	nodes.clear();
	s_lit = { std::make_shared<Value>(D_string("hello,")) };
	nodes.emplace_back(std::move(s_lit)); // "hello,"
	s_nref = { D_string::shallow("cval") };
	nodes.emplace_back(std::move(s_nref)); // cval
	s_opr = { Expression_node::operator_infix_mul, false };
	nodes.emplace_back(std::move(s_opr)); // *
	auto branch_false = Vp<Expression>(std::make_shared<Expression>(std::move(nodes)));

	nodes.clear();
	s_nref = { D_string::shallow("condition") };
	nodes.emplace_back(std::move(s_nref)); // condition
	s_opr = { Expression_node::operator_prefix_notl, false };
	nodes.emplace_back(std::move(s_opr)); // !
	Expression_node::S_branch s_br = { std::move(branch_true), std::move(branch_false) };
	nodes.emplace_back(std::move(s_br)); // ?:
	s_lit = { std::make_shared<Value>(D_integer(1)) };
	nodes.emplace_back(std::move(s_lit)); // 1
	s_nref = { D_string::shallow("rval") };
	nodes.emplace_back(std::move(s_nref)); // rval
	s_opr = { Expression_node::operator_postfix_at, false };
	nodes.emplace_back(std::move(s_opr)); // []
	s_opr = { Expression_node::operator_infix_assign, false };
	nodes.emplace_back(std::move(s_opr)); // =
	auto expr = Vp<Expression>(std::make_shared<Expression>(std::move(nodes)));

	auto condition = std::make_shared<Variable>();
	lref = { condition };
	lrwref = scope->drill_for_named_reference(D_string::shallow("condition"));
	set_reference(lrwref, std::move(lref));

	set_value(condition->drill_for_value(), recycler, D_boolean(false));
	Vp<Reference> result;
	evaluate_expression(result, recycler, expr, scope);
	ASTERIA_TEST_CHECK(dval->get_value_opt()->get<D_double>() == 2.5);
	ASTERIA_TEST_CHECK(cval->get_value_opt()->get<D_integer>() == 10);
	auto rptr = read_reference_opt(result);
	ASTERIA_TEST_CHECK(rval->get_value_opt()->get<D_array>().at(1).get() == rptr.get());
	ASTERIA_TEST_CHECK(rptr->get<D_double>() == 1.75);

	set_value(condition->drill_for_value(), recycler, D_boolean(true));
	evaluate_expression(result, recycler, expr, scope);
	ASTERIA_TEST_CHECK(dval->get_value_opt()->get<D_double>() == 2.5);
	ASTERIA_TEST_CHECK(cval->get_value_opt()->get<D_integer>() == 10);
	rptr = read_reference_opt(result);
	ASTERIA_TEST_CHECK(rval->get_value_opt()->get<D_array>().at(1).get() == rptr.get());
	ASTERIA_TEST_CHECK(rptr->get<D_string>() == "hello,hello,hello,hello,hello,hello,hello,hello,hello,hello,");
}
