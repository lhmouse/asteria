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

void bind_block(Xptr<Block> &block_out, Spcref<const Block> source_opt, Spcref<const Scope> scope){
	if(source_opt == nullptr){
		return block_out.reset();
	}
	const auto scope_child = std::make_shared<Scope>(Scope::type_lexical, scope);
	std::vector<Statement> stmts;
	stmts.reserve(source_opt->size());
	for(const auto &stmt : *source_opt){
		const auto type = stmt.get_type();
		switch(type){
		case Statement::type_expression_statement: {
			const auto &params = stmt.get<Statement::S_expression_statement>();
			Xptr<Expression> bound_expr;
			bind_expression(bound_expr, params.expression_opt, scope_child);
			Statement::S_expression_statement stmt_e = { std::move(bound_expr) };
			stmts.emplace_back(std::move(stmt_e));
			break; }
		case Statement::type_variable_definition: {
			const auto &params = stmt.get<Statement::S_variable_definition>();
			// Declare a local reference in this lexical scope_child.
			// This is necessary to prevent identifiers that designate references inside source statements from being bound onto references outside them.
			const auto wref = scope_child->drill_for_local_reference(params.identifier);
			set_reference(wref, nullptr);
			// Bind the initializer recursively.
			Xptr<Initializer> bound_init;
			bind_initializer(bound_init, params.initializer_opt, scope_child);
			Statement::S_variable_definition stmt_v = { params.identifier, params.immutable, std::move(bound_init) };
			stmts.emplace_back(std::move(stmt_v));
			break; }
		case Statement::type_function_definition: {
			const auto &params = stmt.get<Statement::S_function_definition>();
			// Declare a local reference in this lexical scope_child.
			// This is necessary to prevent identifiers that designate references inside source statements from being bound onto references outside them.
			const auto wref = scope_child->drill_for_local_reference(params.identifier);
			set_reference(wref, nullptr);
			// Bind the function body recursively.
			const auto scope_with_args = std::make_shared<Scope>(Scope::type_lexical, scope_child);
			prepare_lexical_scope(scope_with_args, params.parameters_opt);
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope_with_args);
			Statement::S_function_definition stmt_f = { params.identifier, params.parameters_opt, std::move(bound_body) };
			stmts.emplace_back(std::move(stmt_f));
			break; }
		case Statement::type_if_statement: {
			const auto &params = stmt.get<Statement::S_if_statement>();
			Xptr<Expression> bound_cond;
			bind_expression(bound_cond, params.condition_opt, scope_child);
			Xptr<Block> bound_branch_true;
			bind_block(bound_branch_true, params.branch_true_opt, scope_child);
			Xptr<Block> bound_branch_false;
			bind_block(bound_branch_false, params.branch_false_opt, scope_child);
			Statement::S_if_statement stmt_i = { std::move(bound_cond), std::move(bound_branch_true), std::move(bound_branch_false) };
			stmts.emplace_back(std::move(stmt_i));
			break; }
		case Statement::type_switch_statement: {
			const auto &params = stmt.get<Statement::S_switch_statement>();
			Xptr<Expression> bound_ctrl;
			bind_expression(bound_ctrl, params.control_expression_opt, scope_child);
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope_child);
			Statement::S_switch_statement stmt_s = { std::move(bound_ctrl), std::move(bound_body) };
			stmts.emplace_back(std::move(stmt_s));
			break; }
		case Statement::type_do_while_statement: {
			const auto &params = stmt.get<Statement::S_do_while_statement>();
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope_child);
			Xptr<Expression> bound_cond;
			bind_expression(bound_cond, params.condition_opt, scope_child);
			Statement::S_do_while_statement stmt_d = { std::move(bound_body), std::move(bound_cond) };
			stmts.emplace_back(std::move(stmt_d));
			break; }
		case Statement::type_while_statement: {
			const auto &params = stmt.get<Statement::S_while_statement>();
			Xptr<Expression> bound_cond;
			bind_expression(bound_cond, params.condition_opt, scope_child);
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope_child);
			Statement::S_while_statement stmt_w = { std::move(bound_cond), std::move(bound_body) };
			stmts.emplace_back(std::move(stmt_w));
			break; }
		case Statement::type_for_statement: {
			const auto &params = stmt.get<Statement::S_for_statement>();
			Xptr<Block> bound_init;
			bind_block(bound_init, params.initialization_opt, scope_child);
			Xptr<Expression> bound_cond;
			bind_expression(bound_cond, params.condition_opt, scope_child);
			Xptr<Expression> bound_inc;
			bind_expression(bound_inc, params.increment_opt, scope_child);
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope_child);
			Statement::S_for_statement stmt_f = { std::move(bound_init), std::move(bound_cond), std::move(bound_inc), std::move(bound_body) };
			stmts.emplace_back(std::move(stmt_f));
			break; }
		case Statement::type_foreach_statement: {
			const auto &params = stmt.get<Statement::S_foreach_statement>();
			Xptr<Initializer> bound_init;
			bind_initializer(bound_init, params.range_initializer, scope_child);
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope_child);
			Statement::S_foreach_statement stmt_f = { params.key_identifier, params.value_identifier, std::move(bound_init), std::move(bound_body) };
			stmts.emplace_back(std::move(stmt_f));
			break; }
		case Statement::type_try_statement: {
			const auto &params = stmt.get<Statement::S_try_statement>();
			Xptr<Block> bound_branch_try;
			bind_block(bound_branch_try, params.branch_try_opt, scope_child);
			Xptr<Block> bound_branch_catch;
			bind_block(bound_branch_catch, params.branch_catch_opt, scope_child);
			Statement::S_try_statement stmt_t = { std::move(bound_branch_try), params.exception_identifier, std::move(bound_branch_catch) };
			stmts.emplace_back(std::move(stmt_t));
			break; }
		case Statement::type_defer_statement: {
			const auto &params = stmt.get<Statement::S_defer_statement>();
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope_child);
			Statement::S_defer_statement stmt_d = { std::move(bound_body) };
			stmts.emplace_back(std::move(stmt_d));
			break; }
		case Statement::type_case_label_statement: {
			const auto &params = stmt.get<Statement::S_case_label_statement>();
			stmts.emplace_back(params);
			break; }
		case Statement::type_default_label_statement: {
			const auto &params = stmt.get<Statement::S_default_label_statement>();
			stmts.emplace_back(params);
			break; }
		case Statement::type_break_statement: {
			const auto &params = stmt.get<Statement::S_break_statement>();
			stmts.emplace_back(params);
			break; }
		case Statement::type_continue_statement: {
			const auto &params = stmt.get<Statement::S_continue_statement>();
			stmts.emplace_back(params);
			break; }
		case Statement::type_throw_statement: {
			const auto &params = stmt.get<Statement::S_throw_statement>();
			Xptr<Expression> bound_operand;
			bind_expression(bound_operand, params.operand_opt, scope_child);
			Statement::S_throw_statement stmt_t = { std::move(bound_operand) };
			stmts.emplace_back(std::move(stmt_t));
			break; }
		case Statement::type_return_statement: {
			const auto &params = stmt.get<Statement::S_return_statement>();
			Xptr<Expression> bound_operand;
			bind_expression(bound_operand, params.operand_opt, scope_child);
			Statement::S_return_statement stmt_r = { std::move(bound_operand) };
			stmts.emplace_back(std::move(stmt_r));
			break; }
		default:
			ASTERIA_DEBUG_LOG("Unknown reference node type enumeration `", type, "`. This is probably a bug, please report.");
			std::terminate();
		}
	}
	return block_out.reset(std::make_shared<Block>(std::move(stmts)));
}
Block::Execution_result execute_block(Xptr<Reference> &reference_out, Spcref<Recycler> recycler, Spcref<const Block> block_opt, Spcref<const Scope> scope){
	(void)reference_out;
	(void)recycler;
	(void)block_opt;
	(void)scope;
	return Block::execution_result_next;
}

}
