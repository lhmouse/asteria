// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "statement.hpp"
#include "block.hpp"
#include "scope.hpp"
#include "expression.hpp"
#include "initializer.hpp"
#include "stored_value.hpp"
#include "stored_reference.hpp"
#include "instantiated_function.hpp"
#include "exception.hpp"
#include "utilities.hpp"

namespace Asteria {

Statement::Statement(Statement &&) noexcept = default;
Statement & Statement::operator=(Statement &&) noexcept = default;
Statement::~Statement() = default;

void bind_statement(T_vector<Statement> &bound_stmts_out, Spr<Scope> scope_inout, const Statement &stmt){
	const auto type = stmt.get_type();
	switch(type){
	case Statement::type_expression_statement: {
		const auto &cand = stmt.get<Statement::S_expression_statement>();
		// Bind the expression recursively.
		Vp<Expression> bound_expr;
		bind_expression(bound_expr, cand.expr_opt, scope_inout);
		Statement::S_expression_statement stmt_e = { std::move(bound_expr) };
		return bound_stmts_out.emplace_back(std::move(stmt_e)); }

	case Statement::type_variable_definition: {
		const auto &cand = stmt.get<Statement::S_variable_definition>();
		// Create a null named reference for the variable.
		const auto wref = scope_inout->drill_for_named_reference(cand.id);
		set_reference(wref, nullptr);
		// Bind the initializer recursively.
		Vp<Initializer> bound_init;
		bind_initializer(bound_init, cand.init_opt, scope_inout);
		Statement::S_variable_definition stmt_v = { cand.id, cand.constant, std::move(bound_init) };
		return bound_stmts_out.emplace_back(std::move(stmt_v)); }

	case Statement::type_function_definition: {
		const auto &cand = stmt.get<Statement::S_function_definition>();
		// Create a null named reference for the function.
		const auto wref = scope_inout->drill_for_named_reference(cand.id);
		set_reference(wref, nullptr);
		// Bind the function body recursively.
		const auto scope_lexical = std::make_shared<Scope>(Scope::purpose_lexical, scope_inout);
		prepare_function_scope_lexical(scope_lexical, cand.location, cand.params_opt);
		Vp<Block> bound_body;
		bind_block_in_place(bound_body, scope_lexical, cand.body_opt);
		Statement::S_function_definition stmt_f = { cand.id, cand.location, cand.params_opt, std::move(bound_body) };
		return bound_stmts_out.emplace_back(std::move(stmt_f)); }

	case Statement::type_if_statement: {
		const auto &cand = stmt.get<Statement::S_if_statement>();
		// Bind the condition recursively.
		Vp<Expression> bound_cond;
		bind_expression(bound_cond, cand.cond_opt, scope_inout);
		// Bind both branches recursively.
		Vp<Block> bound_branch_true;
		bind_block(bound_branch_true, cand.branch_true_opt, scope_inout);
		Vp<Block> bound_branch_false;
		bind_block(bound_branch_false, cand.branch_false_opt, scope_inout);
		Statement::S_if_statement stmt_i = { std::move(bound_cond), std::move(bound_branch_true), std::move(bound_branch_false) };
		return bound_stmts_out.emplace_back(std::move(stmt_i)); }

	case Statement::type_switch_statement: {
		const auto &cand = stmt.get<Statement::S_switch_statement>();
		// Bind the control expression recursively.
		Vp<Expression> bound_ctrl;
		bind_expression(bound_ctrl, cand.ctrl_opt, scope_inout);
		// Bind clauses recursively. A clause consists of a label expression and a body block.
		// Notice that clauses in a `switch` statement share the same scope_inout.
		const auto scope_switch = std::make_shared<Scope>(Scope::purpose_lexical, scope_inout);
		T_vector<T_pair<Vp<Expression>, Vp<Block>>> bound_clauses;
		bound_clauses.reserve(cand.clauses_opt.size());
		for(const auto &pair : cand.clauses_opt){
			Vp<Expression> bound_label;
			bind_expression(bound_label, pair.first, scope_switch);
			Vp<Block> bound_body;
			bind_block_in_place(bound_body, scope_switch, pair.second);
			bound_clauses.emplace_back(std::move(bound_label), std::move(bound_body));
		}
		Statement::S_switch_statement stmt_s = { std::move(bound_ctrl), std::move(bound_clauses) };
		return bound_stmts_out.emplace_back(std::move(stmt_s)); }

	case Statement::type_do_while_statement: {
		const auto &cand = stmt.get<Statement::S_do_while_statement>();
		// Bind the body and the condition recursively.
		Vp<Block> bound_body;
		bind_block(bound_body, cand.body_opt, scope_inout);
		Vp<Expression> bound_cond;
		bind_expression(bound_cond, cand.cond_opt, scope_inout);
		Statement::S_do_while_statement stmt_d = { std::move(bound_body), std::move(bound_cond) };
		return bound_stmts_out.emplace_back(std::move(stmt_d)); }

	case Statement::type_while_statement: {
		const auto &cand = stmt.get<Statement::S_while_statement>();
		// Bind the condition and the body recursively.
		Vp<Expression> bound_cond;
		bind_expression(bound_cond, cand.cond_opt, scope_inout);
		Vp<Block> bound_body;
		bind_block(bound_body, cand.body_opt, scope_inout);
		Statement::S_while_statement stmt_w = { std::move(bound_cond), std::move(bound_body) };
		return bound_stmts_out.emplace_back(std::move(stmt_w)); }

	case Statement::type_for_statement: {
		const auto &cand = stmt.get<Statement::S_for_statement>();
		// The scope_inout of the lopp initialization outlasts the scope_inout of the loop body, which will be
		// created and destroyed upon entrance and exit of each iteration.
		const auto scope_for = std::make_shared<Scope>(Scope::purpose_lexical, scope_inout);
		// Bind the loop initialization recursively.
		Vp<Block> bound_init;
		bind_block_in_place(bound_init, scope_for, cand.init_opt);
		// Bind the condition, step and body recursively.
		Vp<Expression> bound_cond;
		bind_expression(bound_cond, cand.cond_opt, scope_for);
		Vp<Expression> bound_step;
		bind_expression(bound_step, cand.step_opt, scope_for);
		Vp<Block> bound_body;
		bind_block(bound_body, cand.body_opt, scope_for);
		Statement::S_for_statement stmt_f = { std::move(bound_init), std::move(bound_cond), std::move(bound_step), std::move(bound_body) };
		return bound_stmts_out.emplace_back(std::move(stmt_f)); }

	case Statement::type_for_each_statement: {
		const auto &cand = stmt.get<Statement::S_for_each_statement>();
		// The scope_inout of the lopp initialization outlasts the scope_inout of the loop body, which will be
		// created and destroyed upon entrance and exit of each iteration.
		const auto scope_for = std::make_shared<Scope>(Scope::purpose_lexical, scope_inout);
		// Bind the loop range initializer recursively.
		Vp<Initializer> bound_range_init;
		bind_initializer(bound_range_init, cand.range_init_opt, scope_for);
		// Create null named references for the key and the value.
		const auto key_wref = scope_inout->drill_for_named_reference(cand.key_id);
		const auto value_wref = scope_inout->drill_for_named_reference(cand.value_id);
		set_reference(key_wref, nullptr);
		set_reference(value_wref, nullptr);
		// Bind the body recursively.
		Vp<Block> bound_body;
		bind_block(bound_body, cand.body_opt, scope_inout);
		Statement::S_for_each_statement stmt_f = { cand.key_id, cand.value_id, std::move(bound_range_init), std::move(bound_body) };
		return bound_stmts_out.emplace_back(std::move(stmt_f)); }

	case Statement::type_try_statement: {
		const auto &cand = stmt.get<Statement::S_try_statement>();
		// Bind both branches recursively.
		Vp<Block> bound_branch_try;
		bind_block(bound_branch_try, cand.branch_try_opt, scope_inout);
		Vp<Block> bound_branch_catch;
		bind_block(bound_branch_catch, cand.branch_catch_opt, scope_inout);
		Statement::S_try_statement stmt_t = { std::move(bound_branch_try), cand.except_id, std::move(bound_branch_catch) };
		return bound_stmts_out.emplace_back(std::move(stmt_t)); }

	case Statement::type_defer_statement: {
		const auto &cand = stmt.get<Statement::S_defer_statement>();
		// Bind the body recursively.
		Vp<Block> bound_body;
		bind_block(bound_body, cand.body_opt, scope_inout);
		Statement::S_defer_statement stmt_d = { cand.location, std::move(bound_body) };
		return bound_stmts_out.emplace_back(std::move(stmt_d)); }

	case Statement::type_break_statement: {
		const auto &cand = stmt.get<Statement::S_break_statement>();
		// Copy it as is.
		return bound_stmts_out.emplace_back(cand); }

	case Statement::type_continue_statement: {
		const auto &cand = stmt.get<Statement::S_continue_statement>();
		// Copy it as is.
		return bound_stmts_out.emplace_back(cand); }

	case Statement::type_throw_statement: {
		const auto &cand = stmt.get<Statement::S_throw_statement>();
		// Bind the operand recursively.
		Vp<Expression> bound_operand;
		bind_expression(bound_operand, cand.operand_opt, scope_inout);
		Statement::S_throw_statement stmt_t = { std::move(bound_operand) };
		return bound_stmts_out.emplace_back(std::move(stmt_t)); }

	case Statement::type_return_statement: {
		const auto &cand = stmt.get<Statement::S_return_statement>();
		// Bind the operand recursively.
		Vp<Expression> bound_operand;
		bind_expression(bound_operand, cand.operand_opt, scope_inout);
		Statement::S_return_statement stmt_r = { std::move(bound_operand) };
		return bound_stmts_out.emplace_back(std::move(stmt_r)); }

	default:
		ASTERIA_DEBUG_LOG("Unknown statement type enumeration `", type, "`. This is probably a bug, please report.");
		std::terminate();
	}
}

namespace {
	bool do_check_loop_condition(Vp<Reference> &result_out, Spr<Recycler> recycler, Spr<const Expression> cond_opt, Spr<const Scope> scope_inout){
		// Overwrite `result_out` unconditionally, even when `cond_opt` is null.
		evaluate_expression(result_out, recycler, cond_opt, scope_inout);
		bool result = true;
		if(cond_opt != nullptr){
			const auto condition_var = read_reference_opt(result_out);
			result = test_value(condition_var);
		}
		return result;
	}
}

Statement::Execution_result execute_statement(Vp<Reference> &result_out, Spr<Scope> scope_inout, Spr<Recycler> recycler, const Statement &stmt){
	const auto type = stmt.get_type();
	switch(type){
	case Statement::type_expression_statement: {
		const auto &cand = stmt.get<Statement::S_expression_statement>();
		// Evaluate the expression, storing the result into `result_out`.
		evaluate_expression(result_out, recycler, cand.expr_opt, scope_inout);
		break; }

	case Statement::type_variable_definition: {
		const auto &cand = stmt.get<Statement::S_variable_definition>();
		// Evaluate the initializer and move the result into a variable.
		evaluate_initializer(result_out, recycler, cand.init_opt, scope_inout);
		Vp<Value> value;
		extract_value_from_reference(value, recycler, std::move(result_out));
		// Create a reference to a temporary value, then materialize it.
		// This results in a variable.
		Reference::S_temporary_value ref_t = { std::move(value) };
		set_reference(result_out, std::move(ref_t));
		materialize_reference(result_out, recycler, cand.constant);
		const auto wref = scope_inout->drill_for_named_reference(cand.id);
		copy_reference(wref, result_out);
		break; }

	case Statement::type_function_definition: {
		const auto &cand = stmt.get<Statement::S_function_definition>();
		// Bind the function body onto the current scope_inout.
		const auto scope_lexical = std::make_shared<Scope>(Scope::purpose_lexical, scope_inout);
		prepare_function_scope_lexical(scope_lexical, cand.location, cand.params_opt);
		Vp<Block> bound_body;
		bind_block_in_place(bound_body, scope_lexical, cand.body_opt);
		// Create a named reference for the function.
		auto func = std::make_shared<Instantiated_function>("function", cand.location, cand.params_opt, scope_inout, std::move(bound_body));
		Vp<Value> func_var;
		set_value(func_var, recycler, D_function(std::move(func)));
		Reference::S_temporary_value ref_t = { std::move(func_var) };
		set_reference(result_out, std::move(ref_t));
		materialize_reference(result_out, recycler, true);
		const auto wref = scope_inout->drill_for_named_reference(cand.id);
		copy_reference(wref, result_out);
		break; }

	case Statement::type_if_statement: {
		const auto &cand = stmt.get<Statement::S_if_statement>();
		// Evaluate the condition expression and select a branch basing on the result.
		evaluate_expression(result_out, recycler, cand.cond_opt, scope_inout);
		const auto condition_var = read_reference_opt(result_out);
		const auto branch_taken = test_value(condition_var) ? cand.branch_true_opt.share() : cand.branch_false_opt.share();
		if(!branch_taken){
			// If the branch is absent, don't execute anything further.
			break;
		}
		// Execute the branch recursively.
		const auto result = execute_block(result_out, recycler, branch_taken, scope_inout);
		if(result != Statement::execution_result_next){
			// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
			return result;
		}
		break; }

	case Statement::type_switch_statement: {
		const auto &cand = stmt.get<Statement::S_switch_statement>();
		// Evaluate the control expression.
		evaluate_expression(result_out, recycler, cand.ctrl_opt, scope_inout);
		const auto control_var = read_reference_opt(result_out);
		ASTERIA_DEBUG_LOG("Switching on `", sp_fmt(control_var), "`...");
		// Traverse the clause list to find one that matches the result.
		// Notice that clauses in a `switch` statement share the same scope_inout.
		const auto scope_switch = std::make_shared<Scope>(Scope::purpose_plain, scope_inout);
		auto match_it = cand.clauses_opt.end();
		for(auto it = cand.clauses_opt.begin(); it != cand.clauses_opt.end(); ++it){
			if(it->first){
				// Deal with a `case` label.
				evaluate_expression(result_out, recycler, it->first, scope_switch);
				const auto case_var = read_reference_opt(result_out);
				if(compare_values(control_var, case_var) == Value::comparison_result_equal){
					match_it = it;
					break;
				}
			} else {
				// Deal with a `default` label.
				if(match_it != cand.clauses_opt.end()){
					ASTERIA_THROW_RUNTIME_ERROR("More than one `default` labels have been found in this `switch` statement.");
				}
				match_it = it;
			}
		}
		// Iterate from the match clause to the end of the body, falling through clause ends if any.
		for(auto it = match_it; it != cand.clauses_opt.end(); ++it){
			// Execute the clause recursively.
			const auto result = execute_block_in_place(result_out, scope_switch, recycler, it->second);
			if((result == Statement::execution_result_break_unspecified) || (result == Statement::execution_result_break_switch)){
				// Break out of the body as requested.
				break;
			}
			if(result != Statement::execution_result_next){
				// Forward anything unexpected to the caller.
				return result;
			}
		}
		break; }

	case Statement::type_do_while_statement: {
		const auto &cand = stmt.get<Statement::S_do_while_statement>();
		do {
			// Execute the loop body recursively.
			const auto result = execute_block(result_out, recycler, cand.body_opt, scope_inout);
			if((result == Statement::execution_result_break_unspecified) || (result == Statement::execution_result_break_while)){
				// Break out of the body as requested.
				break;
			}
			if((result != Statement::execution_result_next) && (result != Statement::execution_result_continue_unspecified) && (result != Statement::execution_result_continue_while)){
				// Forward anything unexpected to the caller.
				return result;
			}
			// Evaluate the condition expression and decide whether to start a new loop basing on the result.
		} while(do_check_loop_condition(result_out, recycler, cand.cond_opt, scope_inout));
		break; }

	case Statement::type_while_statement: {
		const auto &cand = stmt.get<Statement::S_while_statement>();
		// Evaluate the condition expression and decide whether to start a new loop basing on the result.
		while(do_check_loop_condition(result_out, recycler, cand.cond_opt, scope_inout)){
			// Execute the loop body recursively.
			const auto result = execute_block(result_out, recycler, cand.body_opt, scope_inout);
			if((result == Statement::execution_result_break_unspecified) || (result == Statement::execution_result_break_while)){
				// Break out of the body as requested.
				break;
			}
			if((result != Statement::execution_result_next) && (result != Statement::execution_result_continue_unspecified) && (result != Statement::execution_result_continue_while)){
				// Forward anything unexpected to the caller.
				return result;
			}
		}
		break; }

	case Statement::type_for_statement: {
		const auto &cand = stmt.get<Statement::S_for_statement>();
		// The scope_inout of the lopp initialization outlasts the scope_inout of the loop body, which will be
		// created and destroyed upon entrance and exit of each iteration.
		const auto scope_for = std::make_shared<Scope>(Scope::purpose_plain, scope_inout);
		// Perform loop initialization.
		auto result = execute_block_in_place(result_out, scope_for, recycler, cand.init_opt);
		if(result != Statement::execution_result_next){
			// The initialization is considered to be outside the loop body.
			// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
			return result;
		}
		// Evaluate the condition expression and decide whether to start a new loop basing on the result.
		while(do_check_loop_condition(result_out, recycler, cand.cond_opt, scope_for)){
			// Execute the loop body recursively.
			result = execute_block(result_out, recycler, cand.body_opt, scope_for);
			if((result == Statement::execution_result_break_unspecified) || (result == Statement::execution_result_break_for)){
				// Break out of the body as requested.
				break;
			}
			if((result != Statement::execution_result_next) && (result != Statement::execution_result_continue_unspecified) && (result != Statement::execution_result_continue_for)){
				// Forward anything unexpected to the caller.
				return result;
			}
			// Step to the next iteration.
			evaluate_expression(result_out, recycler, cand.step_opt, scope_for);
		}
		break; }

	case Statement::type_for_each_statement: {
		const auto &cand = stmt.get<Statement::S_for_each_statement>();
		// The scope_inout of the loop initialization outlasts the scope_inout of the loop body, which will be
		// created and destroyed upon entrance and exit of each iteration.
		const auto scope_for = std::make_shared<Scope>(Scope::purpose_plain, scope_inout);
		// Perform loop initialization.
		evaluate_initializer(result_out, recycler, cand.range_init_opt, scope_for);
		materialize_reference(result_out, recycler, true);
		Vp<Reference> range_ref;
		move_reference(range_ref, std::move(result_out));
		const auto range_var = read_reference_opt(range_ref);
		const auto range_type = get_value_type(range_var);
		if(range_type == Value::type_array){
			// Save the size. This is necessary because the array might be subsequently altered.
			std::ptrdiff_t size;
			{
				const auto &range_array = range_var->get<D_array>();
				size = static_cast<std::ptrdiff_t>(range_array.size());
			}
			Vp<Value> key_var;
			Vp<Reference> temp_ref;
			for(std::ptrdiff_t index = 0; index < size; ++index){
				// Set the key, which is an integer.
				set_value(key_var, recycler, D_integer(index));
				const auto key_wref = scope_for->drill_for_named_reference(cand.key_id);
				Reference::S_constant ref_k = { key_var.share_c() };
				set_reference(key_wref, std::move(ref_k));
				// Set the value, which is an array element.
				copy_reference(temp_ref, range_ref);
				const auto value_wref = scope_for->drill_for_named_reference(cand.value_id);
				Reference::S_array_element ref_v = { std::move(temp_ref), index };
				set_reference(value_wref, std::move(ref_v));
				// Execute the loop body recursively.
				const auto result = execute_block(result_out, recycler, cand.body_opt, scope_for);
				if((result == Statement::execution_result_break_unspecified) || (result == Statement::execution_result_break_for)){
					// Break out of the body as requested.
					break;
				}
				if((result != Statement::execution_result_next) && (result != Statement::execution_result_continue_unspecified) && (result != Statement::execution_result_continue_for)){
					// Forward anything unexpected to the caller.
					return result;
				}
			}
		} else if(range_type == Value::type_object){
			// Save the keys. This is necessary because the object might be subsequently altered.
			T_vector<D_string> backup_keys;
			{
				const auto &range_object = range_var->get<D_object>();
				backup_keys.reserve(range_object.size());
				for(const auto &pair : range_object){
					backup_keys.emplace_back(pair.first);
				}
			}
			Vp<Value> key_var;
			Vp<Reference> temp_ref;
			for(auto &key : backup_keys){
				// Set the key, which is an integer.
				set_value(key_var, recycler, D_string(key));
				const auto key_wref = scope_for->drill_for_named_reference(cand.key_id);
				Reference::S_constant ref_k = { key_var.share_c() };
				set_reference(key_wref, std::move(ref_k));
				// Set the value, which is an object member.
				copy_reference(temp_ref, range_ref);
				const auto value_wref = scope_for->drill_for_named_reference(cand.value_id);
				Reference::S_object_member ref_v = { std::move(temp_ref), std::move(key) };
				set_reference(value_wref, std::move(ref_v));
				// Execute the loop body recursively.
				const auto result = execute_block(result_out, recycler, cand.body_opt, scope_for);
				if((result == Statement::execution_result_break_unspecified) || (result == Statement::execution_result_break_for)){
					// Break out of the body as requested.
					break;
				}
				if((result != Statement::execution_result_next) && (result != Statement::execution_result_continue_unspecified) && (result != Statement::execution_result_continue_for)){
					// Forward anything unexpected to the caller.
					return result;
				}
			}
		} else {
			ASTERIA_THROW_RUNTIME_ERROR("`for each` statements do not accept something having type `", get_type_name(range_type), "`.");
		}
		break; }

	case Statement::type_try_statement: {
		const auto &cand = stmt.get<Statement::S_try_statement>();
		// Execute the `try` branch in a C++ `try...catch` statement.
		Sp<Scope> scope_catch;
		try {
			try {
				const auto scope_try = std::make_shared<Scope>(Scope::purpose_plain, scope_inout);
				const auto result = execute_block_in_place(result_out, scope_try, recycler, cand.branch_try_opt);
				if(result != Statement::execution_result_next){
					// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
					return result;
				}
			} catch(Exception &e){
				ASTERIA_DEBUG_LOG("Caught `Asteria::Exception`: ", sp_fmt(e.get_reference_opt()));
				// Print exceptions nested, if any.
				auto nested_eptr = e.nested_ptr();
				if(nested_eptr){
					static constexpr char s_prefix[] = "which contains a nested ";
					auto prefix_width = static_cast<int>(sizeof(s_prefix) - 1);
					do {
						prefix_width += 2;
						try {
							std::rethrow_exception(nested_eptr);
						} catch(Exception &ne){
							ASTERIA_DEBUG_LOG(std::setw(prefix_width), s_prefix, "`Asteria::Exception`: ", sp_fmt(ne.get_reference_opt()));
							nested_eptr = ne.nested_ptr();
						} catch(std::exception &ne){
							ASTERIA_DEBUG_LOG(std::setw(prefix_width), s_prefix, "`std::exception`: ", ne.what());
							nested_eptr = nullptr;
						}
					} while(nested_eptr);
				}
				// Move the reference into the `catch` scope_inout, then execute the `catch` branch.
				scope_catch = std::make_shared<Scope>(Scope::purpose_plain, scope_inout);
				copy_reference(result_out, e.get_reference_opt());
				const auto wref = scope_catch->drill_for_named_reference(cand.except_id);
				copy_reference(wref, result_out);
				throw;
			} catch(std::exception &e){
				ASTERIA_DEBUG_LOG("Caught `std::exception`: ", e.what());
				// Create a string containing the error message in the `catch` scope_inout, then execute the `catch` branch.
				scope_catch = std::make_shared<Scope>(Scope::purpose_plain, scope_inout);
				Vp<Value> what_var;
				set_value(what_var, recycler, D_string(e.what()));
				Reference::S_temporary_value ref_t = { std::move(what_var) };
				set_reference(result_out, std::move(ref_t));
				materialize_reference(result_out, recycler, true);
				const auto wref = scope_catch->drill_for_named_reference(cand.except_id);
				copy_reference(wref, result_out);
				throw;
			}
		} catch(std::exception &){
			const auto result = execute_block_in_place(result_out, scope_catch, recycler, cand.branch_catch_opt);
			if(result != Statement::execution_result_next){
				// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
				return result;
			}
		}
		break; }

	case Statement::type_defer_statement: {
		const auto &cand = stmt.get<Statement::S_defer_statement>();
		// Bind the function body onto the current scope_inout. There are no parameters.
		Vp<Block> bound_body;
		bind_block(bound_body, cand.body_opt, scope_inout);
		// Register the function as a deferred callback of the current scope_inout.
		auto func = std::make_shared<Instantiated_function>("deferred block", cand.location, Sp_vector<const Parameter>(), scope_inout, std::move(bound_body));
		scope_inout->defer_callback(std::move(func));
		break; }

	case Statement::type_break_statement: {
		const auto &cand = stmt.get<Statement::S_break_statement>();
		switch(cand.target_scope){
		case Statement::target_scope_unspecified:
			return Statement::execution_result_break_unspecified;
		case Statement::target_scope_switch:
			return Statement::execution_result_break_switch;
		case Statement::target_scope_while:
			return Statement::execution_result_break_while;
		case Statement::target_scope_for:
			return Statement::execution_result_break_for;
		default:
			ASTERIA_DEBUG_LOG("Unsupported target scope_inout enumeration `", cand.target_scope, "`. This is probably a bug, please report.");
			std::terminate();
		}
		break; }

	case Statement::type_continue_statement: {
		const auto &cand = stmt.get<Statement::S_continue_statement>();
		switch(cand.target_scope){
		case Statement::target_scope_unspecified:
			return Statement::execution_result_continue_unspecified;
		case Statement::target_scope_switch:
			ASTERIA_DEBUG_LOG("`switch` is not allowed to follow `continue`. This is probably a bug, please report.");
			std::terminate();
		case Statement::target_scope_while:
			return Statement::execution_result_continue_while;
		case Statement::target_scope_for:
			return Statement::execution_result_continue_for;
		default:
			ASTERIA_DEBUG_LOG("Unsupported target scope_inout enumeration `", cand.target_scope, "`. This is probably a bug, please report.");
			std::terminate();
		}
		break; }

	case Statement::type_throw_statement: {
		const auto &cand = stmt.get<Statement::S_throw_statement>();
		// Evaluate the operand, then throw the exception constructed from the result of it.
		evaluate_expression(result_out, recycler, cand.operand_opt, scope_inout);
		ASTERIA_DEBUG_LOG("Throwing exception: ", result_out);
		materialize_reference(result_out, recycler, true);
		throw Exception(result_out.share_c()); }

	case Statement::type_return_statement: {
		const auto &cand = stmt.get<Statement::S_return_statement>();
		// Evaluate the operand, then return because the value is stored outside this function.
		evaluate_expression(result_out, recycler, cand.operand_opt, scope_inout);
		return Statement::execution_result_return; }

	default:
		ASTERIA_DEBUG_LOG("Unknown statement type enumeration `", type, "`. This is probably a bug, please report.");
		std::terminate();
	}
	return Statement::execution_result_next;
}

}
