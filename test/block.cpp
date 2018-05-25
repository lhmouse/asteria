// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/block.hpp"
#include "../src/scope.hpp"
#include "../src/recycler.hpp"
#include "../src/expression.hpp"
#include "../src/initializer.hpp"
#include "../src/stored_value.hpp"
#include "../src/stored_reference.hpp"
#include "../src/local_variable.hpp"

using namespace Asteria;

int main(){
	Xptr<Block> block, init, body;
	std::vector<Statement> stmts, stmts_nested;
	std::vector<Expression_node> expr_nodes;
	Xptr<Expression> expr, cond, inc;
	Xptr<Initializer> initzr;
	// var sum = 0;
	expr_nodes.clear();
	Expression_node::S_literal expr_l = { std::make_shared<Variable>(D_integer(0)) };
	expr_nodes.emplace_back(std::move(expr_l));
	expr.emplace(std::move(expr_nodes));
	Initializer::S_assignment_init initzr_a = { std::move(expr) };
	initzr.emplace(std::move(initzr_a));
	Statement::S_variable_definition stmt_v = { D_string::shallow("sum"), false, std::move(initzr) };
	stmts.emplace_back(std::move(stmt_v));
	// for(var i = 1; i <= 10; ++i){
	//   sum += i;
	// }
	// >>> var i = 1
	expr_nodes.clear();
	expr_l = { std::make_shared<Variable>(D_integer(1)) };
	expr_nodes.emplace_back(std::move(expr_l));
	expr.emplace(std::move(expr_nodes));
	initzr_a = { std::move(expr) };
	initzr.emplace(std::move(initzr_a));
	stmt_v = { D_string::shallow("i"), false, std::move(initzr) };
	stmts_nested.emplace_back(std::move(stmt_v));
	init.emplace(std::move(stmts_nested));
	// >>> i <= 10
	expr_nodes.clear();
	expr_l = { std::make_shared<Variable>(D_integer(10)) };
	expr_nodes.emplace_back(std::move(expr_l));
	Expression_node::S_named_reference expr_n = { D_string::shallow("i") };
	expr_nodes.emplace_back(std::move(expr_n));
	Expression_node::S_operator_rpn expr_op = { Expression_node::operator_infix_cmp_lte, false };
	expr_nodes.emplace_back(std::move(expr_op));
	cond.emplace(std::move(expr_nodes));
	// >>> ++i
	expr_nodes.clear();
	expr_n = { D_string::shallow("i") };
	expr_nodes.emplace_back(std::move(expr_n));
	expr_op = { Expression_node::operator_prefix_inc, false };
	expr_nodes.emplace_back(std::move(expr_op));
	inc.emplace(std::move(expr_nodes));
	// >>> sum += i
	expr_nodes.clear();
	expr_n = { D_string::shallow("i") };
	expr_nodes.emplace_back(std::move(expr_n));
	expr_n = { D_string::shallow("sum") };
	expr_nodes.emplace_back(std::move(expr_n));
	expr_op = { Expression_node::operator_infix_add, true };
	expr_nodes.emplace_back(std::move(expr_op));
	expr.emplace(std::move(expr_nodes));
	stmts_nested.clear();
	Statement::S_expression_statement stmt_e = { std::move(expr) };
	stmts_nested.emplace_back(std::move(stmt_e));
	body.emplace(std::move(stmts_nested));
	Statement::S_for_statement stmt_f = { std::move(init), std::move(cond), std::move(inc), std::move(body) };
	stmts.emplace_back(std::move(stmt_f));
	// for each(key, value in [100,200,300]){
	//   sum += key + value;
	// }
	// >>> array
	expr_nodes.clear();
	D_array array;
	array.emplace_back(std::make_shared<Variable>(D_integer(100)));
	array.emplace_back(std::make_shared<Variable>(D_integer(200)));
	array.emplace_back(std::make_shared<Variable>(D_integer(300)));
	expr_l = { std::make_shared<Variable>(std::move(array)) };
	expr_nodes.emplace_back(std::move(expr_l));
	expr.emplace(std::move(expr_nodes));
	initzr_a = { std::move(expr) };
	initzr.emplace(std::move(initzr_a));
	// >>> sum += key + value;
	expr_nodes.clear();
	expr_n = { D_string::shallow("value") };
	expr_nodes.emplace_back(std::move(expr_n));
	expr_n = { D_string::shallow("key") };
	expr_nodes.emplace_back(std::move(expr_n));
	expr_op = { Expression_node::operator_infix_add, false };
	expr_nodes.emplace_back(std::move(expr_op));
	expr_n = { D_string::shallow("sum") };
	expr_nodes.emplace_back(std::move(expr_n));
	expr_op = { Expression_node::operator_infix_add, true };
	expr_nodes.emplace_back(std::move(expr_op));
	expr.emplace(std::move(expr_nodes));
	stmts_nested.clear();
	stmt_e = { std::move(expr) };
	stmts_nested.emplace_back(std::move(stmt_e));
	body.emplace(std::move(stmts_nested));
	Statement::S_for_each_statement stmt_fe = { D_string::shallow("key"), D_string::shallow("value"), std::move(initzr), std::move(body) };
	stmts.emplace_back(std::move(stmt_fe));
	// Finish it.
	block.emplace(std::move(stmts));

	const auto recycler = std::make_shared<Recycler>();
	const auto scope = std::make_shared<Scope>(Scope::purpose_plain, nullptr);
	Xptr<Reference> reference;
	const auto result = execute_block_in_place(reference, scope, recycler, block);
	ASTERIA_TEST_CHECK(result == Block::execution_result_end_of_block);
	ASTERIA_TEST_CHECK(scope->get_local_reference_opt(D_string::shallow("i")) == nullptr);
	const auto sum_ref = scope->get_local_reference_opt(D_string::shallow("sum"));
	const auto sum_var = read_reference_opt(sum_ref);
	ASTERIA_TEST_CHECK(sum_var);
	ASTERIA_TEST_CHECK(sum_var->get<D_integer>() == 658);
}
