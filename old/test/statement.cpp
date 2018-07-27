// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/statement.hpp"
#include "../src/scope.hpp"
#include "../src/expression_node.hpp"
#include "../src/initializer.hpp"
#include "../src/value.hpp"
#include "../src/stored_reference.hpp"

using namespace Asteria;

int main()
{
	std::vector<Statement> block, init, body;
	Expression expr, cond, inc;
	// value sum = 0;
	Expression_node::S_literal expr_l = { std::make_shared<Value>(D_integer(0)) };
	expr.emplace_back(std::move(expr_l));
	Initializer::S_assignment_init initzr_a = { std::move(expr) };
	Statement::S_variable_definition stmt_v = { D_string::shallow("sum"), false, std::move(initzr_a) };
	block.emplace_back(std::move(stmt_v));
	// for(value i = 1; i <= 10; ++i){
	//   sum += i;
	// }
	// >>> value i = 1
	expr_l = { std::make_shared<Value>(D_integer(1)) };
	expr.emplace_back(std::move(expr_l));
	initzr_a = { std::move(expr) };
	stmt_v = { D_string::shallow("i"), false, std::move(initzr_a) };
	init.emplace_back(std::move(stmt_v));
	// >>> i <= 10
	expr_l = { std::make_shared<Value>(D_integer(10)) };
	cond.emplace_back(std::move(expr_l));
	Expression_node::S_named_reference expr_n = { D_string::shallow("i") };
	cond.emplace_back(std::move(expr_n));
	Expression_node::S_operator_rpn expr_op = { Expression_node::operator_infix_cmp_lte, false };
	cond.emplace_back(std::move(expr_op));
	// >>> ++i
	expr_n = { D_string::shallow("i") };
	inc.emplace_back(std::move(expr_n));
	expr_op = { Expression_node::operator_prefix_inc, false };
	inc.emplace_back(std::move(expr_op));
	// >>> sum += i
	expr_n = { D_string::shallow("i") };
	expr.emplace_back(std::move(expr_n));
	expr_n = { D_string::shallow("sum") };
	expr.emplace_back(std::move(expr_n));
	expr_op = { Expression_node::operator_infix_add, true };
	expr.emplace_back(std::move(expr_op));
	Statement::S_expression_statement stmt_e = { std::move(expr) };
	body.emplace_back(std::move(stmt_e));
	Statement::S_for_statement stmt_f = { std::move(init), std::move(cond), std::move(inc), std::move(body) };
	block.emplace_back(std::move(stmt_f));
	// for each(key, value in [100,200,300]){
	//   sum += key + value;
	// }
	// >>> array
	D_array array;
	array.emplace_back(std::make_shared<Value>(D_integer(100)));
	array.emplace_back(std::make_shared<Value>(D_integer(200)));
	array.emplace_back(std::make_shared<Value>(D_integer(300)));
	expr_l = { std::make_shared<Value>(std::move(array)) };
	expr.emplace_back(std::move(expr_l));
	initzr_a = { std::move(expr) };
	// >>> sum += key + value;
	expr_n = { D_string::shallow("value") };
	expr.emplace_back(std::move(expr_n));
	expr_n = { D_string::shallow("key") };
	expr.emplace_back(std::move(expr_n));
	expr_op = { Expression_node::operator_infix_add, false };
	expr.emplace_back(std::move(expr_op));
	expr_n = { D_string::shallow("sum") };
	expr.emplace_back(std::move(expr_n));
	expr_op = { Expression_node::operator_infix_add, true };
	expr.emplace_back(std::move(expr_op));
	stmt_e = { std::move(expr) };
	body.clear();
	body.emplace_back(std::move(stmt_e));
	Statement::S_for_each_statement stmt_fe = { D_string::shallow("key"), D_string::shallow("value"), std::move(initzr_a), std::move(body) };
	block.emplace_back(std::move(stmt_fe));

	const auto scope = std::make_shared<Scope>(Scope::purpose_plain, nullptr);
	Vp<Reference> reference;
	const auto result = execute_block_in_place(reference, scope, block);
	ASTERIA_TEST_CHECK(result == Statement::execution_result_next);
	ASTERIA_TEST_CHECK(scope->get_named_reference_opt(D_string::shallow("i")) == nullptr);
	const auto sum_ref = scope->get_named_reference_opt(D_string::shallow("sum"));
	const auto sum_var = read_reference_opt(sum_ref);
	ASTERIA_TEST_CHECK(sum_var);
	ASTERIA_TEST_CHECK(sum_var->as<D_integer>() == 658);
}
