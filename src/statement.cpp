// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "statement.hpp"
#include "expression.hpp"
#include "scope.hpp"
#include "reference.hpp"
#include "stored_reference.hpp"
#include "initializer.hpp"
#include "utilities.hpp"

namespace Asteria {

Statement::Statement(Statement &&) noexcept = default;
Statement &Statement::operator=(Statement &&) = default;
Statement::~Statement() = default;

Statement::Type get_statement_type(Spcref<const Statement> statement_opt) noexcept {
	return statement_opt ? statement_opt->get_type() : Statement::type_null;
}

void bind_statement_reusing_scope(Xptr<Statement> &statement_out, Spcref<Scope> scope, Spcref<const Statement> source_opt){
	const auto type = get_statement_type(source_opt);
	switch(type){
	case Statement::type_null:
		return statement_out.reset();
	case Statement::type_expression_statement: {
		const auto &params = source_opt->get<Statement::S_expression_statement>();
		Xptr<Expression> bound_expr;
		bind_expression(bound_expr, params.expression_opt, scope);
		Statement::S_expression_statement stmt_e = { std::move(bound_expr) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_e))); }
	case Statement::type_compound_statement: {
		const auto &params = source_opt->get<Statement::S_compound_statement>();
		const auto scope_next = std::make_shared<Scope>(Scope::type_lexical, scope);
		Xptr_vector<Statement> bound_nested_stmts;
		bound_nested_stmts.reserve(params.nested_statements_opt.size());
		for(const auto &stmt_src : params.nested_statements_opt){
			bind_statement_reusing_scope(statement_out, scope_next, stmt_src);
			bound_nested_stmts.emplace_back(std::move(statement_out));
		}
		Statement::S_compound_statement stmt_c = { std::move(bound_nested_stmts) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_c))); }
	case Statement::type_variable_definition: {
		const auto &params = source_opt->get<Statement::S_variable_definition>();
		// Declare a local reference in this lexical scope.
		// This is necessary to prevent identifiers that designate references inside source statements from being bound onto references outside them.
		const auto wref = scope->drill_for_local_reference(params.identifier);
		set_reference(wref, nullptr);
		// Bind the initializer recursively.
		Xptr<Initializer> bound_init;
		bind_initializer(bound_init, params.initializer_opt, scope);
		Statement::S_variable_definition stmt_v = { params.identifier, params.immutable, std::move(bound_init) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_v))); }
	case Statement::type_function_definition: {
		const auto &params = source_opt->get<Statement::S_function_definition>();
		// Declare a local reference in this lexical scope.
		// This is necessary to prevent identifiers that designate references inside source statements from being bound onto references outside them.
		const auto wref = scope->drill_for_local_reference(params.identifier);
		set_reference(wref, nullptr);
		// Bind the function body recursively.
		Xptr<Statement> bound_body;
		bind_statement(bound_body, params.body_opt, scope);
		Statement::S_function_definition stmt_f = { params.identifier, params.parameters_opt, std::move(bound_body) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_f))); }
	case Statement::type_if_statement: {
		const auto &params = source_opt->get<Statement::S_if_statement>();
		Xptr<Expression> bound_cond;
		bind_expression(bound_cond, params.condition_opt, scope);
		Xptr<Statement> bound_branch_true;
		bind_statement(bound_branch_true, params.branch_true_opt, scope);
		Xptr<Statement> bound_branch_false;
		bind_statement(bound_branch_false, params.branch_false_opt, scope);
		Statement::S_if_statement stmt_i = { std::move(bound_cond), std::move(bound_branch_true), std::move(bound_branch_false) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_i))); }
	case Statement::type_switch_statement: {
		const auto &params = source_opt->get<Statement::S_switch_statement>();
		Xptr<Expression> bound_ctrl;
		bind_expression(bound_ctrl, params.control_expression_opt, scope);
		Xptr<Statement> bound_body;
		bind_statement(bound_body, params.body_opt, scope);
		Statement::S_switch_statement stmt_s = { std::move(bound_ctrl), std::move(bound_body) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_s))); }
	case Statement::type_do_while_statement: {
		const auto &params = source_opt->get<Statement::S_do_while_statement>();
		Xptr<Statement> bound_body;
		bind_statement(bound_body, params.body_opt, scope);
		Xptr<Expression> bound_cond;
		bind_expression(bound_cond, params.condition_opt, scope);
		Statement::S_do_while_statement stmt_d = { std::move(bound_body), std::move(bound_cond) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_d))); }
	case Statement::type_while_statement: {
		const auto &params = source_opt->get<Statement::S_while_statement>();
		Xptr<Expression> bound_cond;
		bind_expression(bound_cond, params.condition_opt, scope);
		Xptr<Statement> bound_body;
		bind_statement(bound_body, params.body_opt, scope);
		Statement::S_while_statement stmt_w = { std::move(bound_cond), std::move(bound_body) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_w))); }
	case Statement::type_for_statement: {
		const auto &params = source_opt->get<Statement::S_for_statement>();
		Xptr<Statement> bound_init;
		bind_statement(bound_init, params.initialization_opt, scope);
		Xptr<Expression> bound_cond;
		bind_expression(bound_cond, params.condition_opt, scope);
		Xptr<Expression> bound_inc;
		bind_expression(bound_inc, params.increment_opt, scope);
		Xptr<Statement> bound_body;
		bind_statement(bound_body, params.body_opt, scope);
		Statement::S_for_statement stmt_f = { std::move(bound_init), std::move(bound_cond), std::move(bound_inc), std::move(bound_body) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_f))); }
	case Statement::type_foreach_statement: {
		const auto &params = source_opt->get<Statement::S_foreach_statement>();
		Xptr<Initializer> bound_init;
		bind_initializer(bound_init, params.range_initializer, scope);
		Xptr<Statement> bound_body;
		bind_statement(bound_body, params.body_opt, scope);
		Statement::S_foreach_statement stmt_f = { params.key_identifier, params.value_identifier, std::move(bound_init), std::move(bound_body) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_f))); }
	case Statement::type_try_statement: {
		const auto &params = source_opt->get<Statement::S_try_statement>();
		Xptr<Statement> bound_branch_try;
		bind_statement(bound_branch_try, params.branch_try_opt, scope);
		Xptr<Statement> bound_branch_catch;
		bind_statement(bound_branch_catch, params.branch_catch_opt, scope);
		Statement::S_try_statement stmt_t = { std::move(bound_branch_try), params.exception_identifier, std::move(bound_branch_catch) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_t))); }
	case Statement::type_defer_statement: {
		const auto &params = source_opt->get<Statement::S_defer_statement>();
		Xptr<Statement> bound_body;
		bind_statement(bound_body, params.body_opt, scope);
		Statement::S_defer_statement stmt_b = { std::move(bound_body) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_b))); }
	case Statement::type_case_label_statement: {
		const auto &params = source_opt->get<Statement::S_case_label_statement>();
		return statement_out.reset(std::make_shared<Statement>(params)); }
	case Statement::type_default_label_statement: {
		const auto &params = source_opt->get<Statement::S_default_label_statement>();
		return statement_out.reset(std::make_shared<Statement>(params)); }
	case Statement::type_break_statement: {
		const auto &params = source_opt->get<Statement::S_break_statement>();
		return statement_out.reset(std::make_shared<Statement>(params)); }
	case Statement::type_continue_statement: {
		const auto &params = source_opt->get<Statement::S_continue_statement>();
		return statement_out.reset(std::make_shared<Statement>(params)); }
	case Statement::type_throw_statement: {
		const auto &params = source_opt->get<Statement::S_throw_statement>();
		Xptr<Expression> bound_operand;
		bind_expression(bound_operand, params.operand_opt, scope);
		Statement::S_throw_statement stmt_t = { std::move(bound_operand) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_t))); }
	case Statement::type_return_statement: {
		const auto &params = source_opt->get<Statement::S_return_statement>();
		Xptr<Expression> bound_operand;
		bind_expression(bound_operand, params.operand_opt, scope);
		Statement::S_return_statement stmt_r = { std::move(bound_operand) };
		return statement_out.reset(std::make_shared<Statement>(std::move(stmt_r))); }
	default:
		ASTERIA_DEBUG_LOG("Unknown reference node type enumeration `", type, "`. This is probably a bug, please report.");
		std::terminate();
	}
}
void bind_statement(Xptr<Statement> &statement_out, Spcref<const Statement> source_opt, Spcref<const Scope> scope){
	if(source_opt == nullptr){
		return statement_out.reset();
	} else {
		auto scope_next = std::make_shared<Scope>(Scope::type_lexical, scope);
		return bind_statement_reusing_scope(statement_out, scope_next, source_opt);
	}
}
Statement::Execute_result execute_statement(Xptr<Reference> &returned_reference_out, Spcref<Scope> scope, Spcref<Recycler> recycler, Spcref<const Statement> statement_opt){
	(void)returned_reference_out;
	(void)scope;
	(void)recycler;
	(void)statement_opt;
	return Statement::execute_result_next;
}

}
