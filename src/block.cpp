// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "block.hpp"
#include "statement.hpp"
#include "scope.hpp"
#include "expression.hpp"
#include "initializer.hpp"
#include "reference.hpp"
#include "stored_reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Block::Block(Block &&) noexcept = default;
Block &Block::operator=(Block &&) = default;
Block::~Block() = default;

void bind_block_in_place(Xptr<Block> &bound_result_out, Spcref<Scope> scope, Spcref<const Block> block_opt){
	if(block_opt == nullptr){
		// Return a null block.
		return bound_result_out.reset();
	}
	// Bind expressions recursively.
	std::vector<Statement> bound_statements;
	bound_statements.reserve(block_opt->size());
	for(std::size_t stmt_index = 0; stmt_index < block_opt->size(); ++stmt_index){
		const auto &stmt = block_opt->at(stmt_index);
		const auto type = stmt.get_type();
		switch(type){
		case Statement::type_expression_statement: {
			const auto &params = stmt.get<Statement::S_expression_statement>();
			// Bind the expression recursively.
			Xptr<Expression> bound_expr;
			bind_expression(bound_expr, params.expression_opt, scope);
			Statement::S_expression_statement stmt_e = { std::move(bound_expr) };
			bound_statements.emplace_back(std::move(stmt_e));
			break; }
		case Statement::type_variable_definition: {
			const auto &params = stmt.get<Statement::S_variable_definition>();
			// Create a null local reference for the variable.
			const auto wref = scope->drill_for_local_reference(params.identifier);
			set_reference(wref, nullptr);
			// Bind the initializer recursively.
			Xptr<Initializer> bound_init;
			bind_initializer(bound_init, params.initializer_opt, scope);
			Statement::S_variable_definition stmt_v = { params.identifier, params.immutable, std::move(bound_init) };
			bound_statements.emplace_back(std::move(stmt_v));
			break; }
		case Statement::type_function_definition: {
			const auto &params = stmt.get<Statement::S_function_definition>();
			// Create a null local reference for the function.
			const auto wref = scope->drill_for_local_reference(params.identifier);
			set_reference(wref, nullptr);
			// Bind the function body recursively.
			const auto scope_lexical = std::make_shared<Scope>(Scope::purpose_lexical, scope);
			prepare_function_scope_lexical(scope_lexical, params.parameters_opt);
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope_lexical);
			Statement::S_function_definition stmt_f = { params.identifier, params.parameters_opt, std::move(bound_body) };
			bound_statements.emplace_back(std::move(stmt_f));
			break; }
		case Statement::type_if_statement: {
			const auto &params = stmt.get<Statement::S_if_statement>();
			// Bind the condition recursively.
			Xptr<Expression> bound_cond;
			bind_expression(bound_cond, params.condition_opt, scope);
			// Bind both branches recursively.
			Xptr<Block> bound_branch_true;
			bind_block(bound_branch_true, params.branch_true_opt, scope);
			Xptr<Block> bound_branch_false;
			bind_block(bound_branch_false, params.branch_false_opt, scope);
			Statement::S_if_statement stmt_i = { std::move(bound_cond), std::move(bound_branch_true), std::move(bound_branch_false) };
			bound_statements.emplace_back(std::move(stmt_i));
			break; }
		case Statement::type_switch_statement: {
			const auto &params = stmt.get<Statement::S_switch_statement>();
			// Bind the control expression recursively.
			Xptr<Expression> bound_ctrl;
			bind_expression(bound_ctrl, params.control_opt, scope);
			// Bind clauses recursively. A clause consists of a label expression and a body block.
			// Notice that clauses in a switch statement share the same scope.
			const auto scope_switch = std::make_shared<Scope>(Scope::purpose_lexical, scope);
			std::vector<std::pair<Xptr<Expression>, Xptr<Block>>> bound_clauses;
			bound_clauses.reserve(params.clauses_opt.size());
			for(const auto &pair : params.clauses_opt){
				Xptr<Expression> bound_label;
				bind_expression(bound_label, pair.first, scope_switch);
				Xptr<Block> bound_body;
				bind_block_in_place(bound_body, scope_switch, pair.second);
				bound_clauses.emplace_back(std::move(bound_label), std::move(bound_body));
			}
			Statement::S_switch_statement stmt_s = { std::move(bound_ctrl), std::move(bound_clauses) };
			bound_statements.emplace_back(std::move(stmt_s));
			break; }
		case Statement::type_do_while_statement: {
			const auto &params = stmt.get<Statement::S_do_while_statement>();
			// Bind the body and the condition recursively.
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope);
			Xptr<Expression> bound_cond;
			bind_expression(bound_cond, params.condition_opt, scope);
			Statement::S_do_while_statement stmt_d = { std::move(bound_body), std::move(bound_cond) };
			bound_statements.emplace_back(std::move(stmt_d));
			break; }
		case Statement::type_while_statement: {
			const auto &params = stmt.get<Statement::S_while_statement>();
			// Bind the condition and the body recursively.
			Xptr<Expression> bound_cond;
			bind_expression(bound_cond, params.condition_opt, scope);
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope);
			Statement::S_while_statement stmt_w = { std::move(bound_cond), std::move(bound_body) };
			bound_statements.emplace_back(std::move(stmt_w));
			break; }
		case Statement::type_for_statement: {
			const auto &params = stmt.get<Statement::S_for_statement>();
			// The scope of the lopp initialization outlasts the scope of the loop body, which will be
			// created and destroyed upon entrance and exit of each iteration.
			const auto scope_for = std::make_shared<Scope>(Scope::purpose_lexical, scope);
			// Bind the loop initialization recursively.
			Xptr<Block> bound_init;
			bind_block_in_place(bound_init, scope_for, params.initialization_opt);
			// Bind the condition, the increment and the body recursively.
			Xptr<Expression> bound_cond;
			bind_expression(bound_cond, params.condition_opt, scope_for);
			Xptr<Expression> bound_inc;
			bind_expression(bound_inc, params.increment_opt, scope_for);
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope_for);
			Statement::S_for_statement stmt_f = { std::move(bound_init), std::move(bound_cond), std::move(bound_inc), std::move(bound_body) };
			bound_statements.emplace_back(std::move(stmt_f));
			break; }
		case Statement::type_foreach_statement: {
			const auto &params = stmt.get<Statement::S_foreach_statement>();
			// The scope of the lopp initialization outlasts the scope of the loop body, which will be
			// created and destroyed upon entrance and exit of each iteration.
			const auto scope_for = std::make_shared<Scope>(Scope::purpose_lexical, scope);
			// Bind the loop range initializer recursively.
			Xptr<Initializer> bound_range_init;
			bind_initializer(bound_range_init, params.range_initializer_opt, scope_for);
			// Create null local references for the key and the value.
			auto wref = scope->drill_for_local_reference(params.key_identifier);
			set_reference(wref, nullptr);
			wref = scope->drill_for_local_reference(params.value_identifier);
			set_reference(wref, nullptr);
			// Bind the body recursively.
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope);
			Statement::S_foreach_statement stmt_f = { params.key_identifier, params.value_identifier, std::move(bound_range_init), std::move(bound_body) };
			bound_statements.emplace_back(std::move(stmt_f));
			break; }
		case Statement::type_try_statement: {
			const auto &params = stmt.get<Statement::S_try_statement>();
			// Bind both branches recursively.
			Xptr<Block> bound_branch_try;
			bind_block(bound_branch_try, params.branch_try_opt, scope);
			Xptr<Block> bound_branch_catch;
			bind_block(bound_branch_catch, params.branch_catch_opt, scope);
			Statement::S_try_statement stmt_t = { std::move(bound_branch_try), params.exception_identifier, std::move(bound_branch_catch) };
			bound_statements.emplace_back(std::move(stmt_t));
			break; }
		case Statement::type_defer_statement: {
			const auto &params = stmt.get<Statement::S_defer_statement>();
			// Bind the body recursively.
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope);
			Statement::S_defer_statement stmt_d = { std::move(bound_body) };
			bound_statements.emplace_back(std::move(stmt_d));
			break; }
		case Statement::type_break_statement: {
			const auto &params = stmt.get<Statement::S_break_statement>();
			// Copy it as is.
			bound_statements.emplace_back(params);
			break; }
		case Statement::type_continue_statement: {
			const auto &params = stmt.get<Statement::S_continue_statement>();
			// Copy it as is.
			bound_statements.emplace_back(params);
			break; }
		case Statement::type_throw_statement: {
			const auto &params = stmt.get<Statement::S_throw_statement>();
			// Bind the operand recursively.
			Xptr<Expression> bound_operand;
			bind_expression(bound_operand, params.operand_opt, scope);
			Statement::S_throw_statement stmt_t = { std::move(bound_operand) };
			bound_statements.emplace_back(std::move(stmt_t));
			break; }
		case Statement::type_return_statement: {
			const auto &params = stmt.get<Statement::S_return_statement>();
			// Bind the operand recursively.
			Xptr<Expression> bound_operand;
			bind_expression(bound_operand, params.operand_opt, scope);
			Statement::S_return_statement stmt_r = { std::move(bound_operand) };
			bound_statements.emplace_back(std::move(stmt_r));
			break; }
		default:
			ASTERIA_DEBUG_LOG("Unknown statement type enumeration `", type, "` at index `", stmt_index, "`. This is probably a bug, please report.");
			std::terminate();
		}
	}
	return bound_result_out.emplace(std::move(bound_statements));
}
Block::Execution_result execute_block_in_place(Xptr<Reference> &reference_out, Spcref<Scope> scope, Spcref<Recycler> recycler, Spcref<const Block> block_opt){
	(void)reference_out;
	(void)recycler;
	(void)block_opt;
	(void)scope;
	return Block::execution_result_end_of_block;
}

void bind_block(Xptr<Block> &bound_result_out, Spcref<const Block> block_opt, Spcref<const Scope> scope){
	if(block_opt == nullptr){
		return bound_result_out.reset();
	}
	const auto scope_working = std::make_shared<Scope>(Scope::purpose_lexical, scope);
	return bind_block_in_place(bound_result_out, scope_working, block_opt);
}
Block::Execution_result execute_block(Xptr<Reference> &reference_out, Spcref<Recycler> recycler, Spcref<const Block> block_opt, Spcref<const Scope> scope){
	if(block_opt == nullptr){
		return Block::execution_result_end_of_block;
	}
	const auto scope_working = std::make_shared<Scope>(Scope::purpose_plain, scope);
	return execute_block_in_place(reference_out, scope_working, recycler, block_opt);
}

}
