// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "statement.hpp"
#include "scope.hpp"
#include "expression_node.hpp"
#include "initializer.hpp"
#include "value.hpp"
#include "stored_reference.hpp"
#include "function.hpp"
#include "exception.hpp"
#include "utilities.hpp"

namespace Asteria {

Statement::Statement(Statement &&) noexcept = default;
Statement & Statement::operator=(Statement &&) noexcept = default;
Statement::~Statement() = default;

namespace {
	void do_skip_statement_in_place(Sp_cref<Scope> scope_inout, const Statement &stmt){
		const auto type = stmt.which();
		switch(type){
		case Statement::index_expression_statement:
			return;

		case Statement::index_variable_definition: {
			const auto &cand = stmt.as<Statement::S_variable_definition>();
			// Create a null reference for the variable.
			const auto wref = scope_inout->mutate_named_reference(cand.id);
			set_reference(wref, nullptr);
			return; }

		case Statement::index_function_definition: {
			const auto &cand = stmt.as<Statement::S_function_definition>();
			// Create a null reference for the function.
			const auto wref = scope_inout->mutate_named_reference(cand.id);
			set_reference(wref, nullptr);
			return; }

		case Statement::index_if_statement:
		case Statement::index_switch_statement:
		case Statement::index_do_while_statement:
		case Statement::index_while_statement:
		case Statement::index_for_statement:
		case Statement::index_for_each_statement:
		case Statement::index_try_statement:
		case Statement::index_defer_statement:
		case Statement::index_break_statement:
		case Statement::index_continue_statement:
		case Statement::index_throw_statement:
		case Statement::index_return_statement:
			return;

		default:
			ASTERIA_DEBUG_LOG("An unknown statement type enumeration `", type, "` is encountered. This is probably a bug. Please report.");
			std::terminate();
		}
	}

	Statement do_bind_statement_in_place(Sp_cref<Scope> scope_inout, const Statement &stmt){
		const auto type = stmt.which();
		switch(type){
		case Statement::index_expression_statement: {
			const auto &cand = stmt.as<Statement::S_expression_statement>();
			// Bind the expression recursively.
			auto bound_expr = bind_expression(cand.expr, scope_inout);
			Statement::S_expression_statement stmt_e = { std::move(bound_expr) };
			return std::move(stmt_e); }

		case Statement::index_variable_definition: {
			const auto &cand = stmt.as<Statement::S_variable_definition>();
			// Bind the initializer recursively.
			auto bound_init = bind_initializer(cand.init, scope_inout);
			Statement::S_variable_definition stmt_v = { cand.id, cand.immutable, std::move(bound_init) };
			return std::move(stmt_v); }

		case Statement::index_function_definition: {
			const auto &cand = stmt.as<Statement::S_function_definition>();
			// Bind the function body recursively.
			const auto scope_func = std::make_shared<Scope>(Scope::purpose_lexical, scope_inout);
			prepare_function_scope_lexical(scope_func, cand.location, cand.params);
			auto bound_body = bind_block_in_place(scope_func, cand.body);
			Statement::S_function_definition stmt_f = { cand.id, cand.location, cand.params, std::move(bound_body) };
			return std::move(stmt_f); }

		case Statement::index_if_statement: {
			const auto &cand = stmt.as<Statement::S_if_statement>();
			// Bind the condition recursively.
			auto bound_cond = bind_expression(cand.cond, scope_inout);
			// Bind both branches recursively.
			auto bound_branch_true = bind_block(cand.branch_true, scope_inout);
			auto bound_branch_false = bind_block(cand.branch_false, scope_inout);
			Statement::S_if_statement stmt_if = { std::move(bound_cond), std::move(bound_branch_true), std::move(bound_branch_false) };
			return std::move(stmt_if); }

		case Statement::index_switch_statement: {
			const auto &cand = stmt.as<Statement::S_switch_statement>();
			// Bind the control expression recursively.
			auto bound_ctrl = bind_expression(cand.ctrl, scope_inout);
			// Bind clauses recursively. A clause consists of a label expression and a body block.
			// Notice that clauses in a `switch` statement share the same scope_inout.
			const auto scope_switch = std::make_shared<Scope>(Scope::purpose_lexical, scope_inout);
			Vector<Statement::Switch_clause> bound_clauses;
			bound_clauses.reserve(cand.clauses.size());
			for(const auto &clause : cand.clauses){
				auto bound_pred = bind_expression(clause.pred, scope_switch);
				auto bound_body = bind_block_in_place(scope_switch, clause.body);
				Statement::Switch_clause bound_clause = { std::move(bound_pred), std::move(bound_body) };
				bound_clauses.emplace_back(std::move(bound_clause));
			}
			Statement::S_switch_statement stmt_sw = { std::move(bound_ctrl), std::move(bound_clauses) };
			return std::move(stmt_sw); }

		case Statement::index_do_while_statement: {
			const auto &cand = stmt.as<Statement::S_do_while_statement>();
			// Bind the body and the condition recursively.
			auto bound_body = bind_block(cand.body, scope_inout);
			auto bound_cond = bind_expression(cand.cond, scope_inout);
			Statement::S_do_while_statement stmt_dw = { std::move(bound_body), std::move(bound_cond) };
			return std::move(stmt_dw); }

		case Statement::index_while_statement: {
			const auto &cand = stmt.as<Statement::S_while_statement>();
			// Bind the condition and the body recursively.
			auto bound_cond = bind_expression(cand.cond, scope_inout);
			auto bound_body = bind_block(cand.body, scope_inout);
			Statement::S_while_statement stmt_wh = { std::move(bound_cond), std::move(bound_body) };
			return std::move(stmt_wh); }

		case Statement::index_for_statement: {
			const auto &cand = stmt.as<Statement::S_for_statement>();
			// The scope_inout of the lopp initialization outlasts the scope_inout of the loop body, which will be
			// created and destroyed upon entrance and exit of each iteration.
			const auto scope_for = std::make_shared<Scope>(Scope::purpose_lexical, scope_inout);
			// Bind the loop initialization recursively.
			auto bound_init = bind_block_in_place(scope_for, cand.init);
			// Bind the condition, step and body recursively.
			auto bound_cond = bind_expression(cand.cond, scope_for);
			auto bound_step = bind_expression(cand.step, scope_for);
			auto bound_body = bind_block_in_place(scope_for, cand.body);
			Statement::S_for_statement stmt_for = { std::move(bound_init), std::move(bound_cond), std::move(bound_step), std::move(bound_body) };
			return std::move(stmt_for); }

		case Statement::index_for_each_statement: {
			const auto &cand = stmt.as<Statement::S_for_each_statement>();
			// The scope_inout of the lopp initialization outlasts the scope_inout of the loop body, which will be
			// created and destroyed upon entrance and exit of each iteration.
			const auto scope_for = std::make_shared<Scope>(Scope::purpose_lexical, scope_inout);
			// Bind the loop range initializer recursively.
			auto bound_range_init = bind_initializer(cand.range_init, scope_for);
			// Create null references for the key and the value.
			const auto key_wref = scope_inout->mutate_named_reference(cand.key_id);
			const auto value_wref = scope_inout->mutate_named_reference(cand.value_id);
			set_reference(key_wref, nullptr);
			set_reference(value_wref, nullptr);
			// Bind the body recursively.
			auto bound_body = bind_block_in_place(scope_for, cand.body);
			Statement::S_for_each_statement stmt_fe = { cand.key_id, cand.value_id, std::move(bound_range_init), std::move(bound_body) };
			return std::move(stmt_fe); }

		case Statement::index_try_statement: {
			const auto &cand = stmt.as<Statement::S_try_statement>();
			// Bind the `try` branch recursively.
			auto bound_branch_try = bind_block(cand.branch_try, scope_inout);
			// Bind the `catch` branch recursively.
			const auto scope_catch = std::make_shared<Scope>(Scope::purpose_lexical, scope_inout);
			// Create null references for the exception.
			const auto wref = scope_inout->mutate_named_reference(cand.except_id);
			set_reference(wref, nullptr);
			auto bound_branch_catch = bind_block_in_place(scope_catch, cand.branch_catch);
			Statement::S_try_statement stmt_try = { std::move(bound_branch_try), cand.except_id, std::move(bound_branch_catch) };
			return std::move(stmt_try); }

		case Statement::index_defer_statement: {
			const auto &cand = stmt.as<Statement::S_defer_statement>();
			// Bind the body recursively.
			auto bound_body = bind_block(cand.body, scope_inout);
			Statement::S_defer_statement stmt_de = { cand.location, std::move(bound_body) };
			return std::move(stmt_de); }

		case Statement::index_break_statement: {
			const auto &cand = stmt.as<Statement::S_break_statement>();
			// Copy it as is.
			return cand; }

		case Statement::index_continue_statement: {
			const auto &cand = stmt.as<Statement::S_continue_statement>();
			// Copy it as is.
			return cand; }

		case Statement::index_throw_statement: {
			const auto &cand = stmt.as<Statement::S_throw_statement>();
			// Bind the operand recursively.
			auto bound_operand = bind_expression(cand.operand, scope_inout);
			Statement::S_throw_statement stmt_th = { std::move(bound_operand) };
			return std::move(stmt_th); }

		case Statement::index_return_statement: {
			const auto &cand = stmt.as<Statement::S_return_statement>();
			// Bind the operand recursively.
			auto bound_operand = bind_expression(cand.operand, scope_inout);
			Statement::S_return_statement stmt_r = { std::move(bound_operand) };
			return std::move(stmt_r); }

		default:
			ASTERIA_DEBUG_LOG("An unknown statement type enumeration `", type, "` is encountered. This is probably a bug. Please report.");
			std::terminate();
		}
	}
}

Block bind_block_in_place(Sp_cref<Scope> scope_inout, const Block &block){
	Block bound_block;
	bound_block.reserve(block.size());
	// Bind statements recursively.
	for(const auto &stmt : block){
		auto bound_stmt = do_bind_statement_in_place(scope_inout, stmt);
		do_skip_statement_in_place(scope_inout, stmt);
		bound_block.emplace_back(std::move(bound_stmt));
	}
	return bound_block;
}

namespace {
	bool do_check_loop_condition(Vp<Reference> &result_out, const Expression &cond, Sp_cref<const Scope> scope_inout){
		// Overwrite `result_out` unconditionally, even when `cond` is empty.
		evaluate_expression(result_out, cond, scope_inout);
		if(cond.empty()){
			// An empty condition yields `true`.
			return true;
		}
		const auto cond_var = read_reference_opt(result_out);
		return test_value(cond_var);
	}

	Statement::Execution_result do_execute_statement_in_place(Vp<Reference> &result_out, Sp_cref<Scope> scope_inout, const Statement &stmt){
		const auto type = stmt.which();
		switch(type){
		case Statement::index_expression_statement: {
			const auto &cand = stmt.as<Statement::S_expression_statement>();
			// Evaluate the expression, storing the result into `result_out`.
			evaluate_expression(result_out, cand.expr, scope_inout);
			return Statement::execution_result_next; }

		case Statement::index_variable_definition: {
			const auto &cand = stmt.as<Statement::S_variable_definition>();
			// Evaluate the initializer and move the result into a variable.
			evaluate_initializer(result_out, cand.init, scope_inout);
			Vp<Value> value;
			extract_value_from_reference(value, std::move(result_out));
			// Create a reference to a temporary value, then materialize it.
			// This results in a variable.
			Reference::S_temporary_value ref_t = { std::move(value) };
			set_reference(result_out, std::move(ref_t));
			materialize_reference(result_out, cand.immutable);
			const auto wref = scope_inout->mutate_named_reference(cand.id);
			copy_reference(wref, result_out);
			return Statement::execution_result_next; }

		case Statement::index_function_definition: {
			const auto &cand = stmt.as<Statement::S_function_definition>();
			// Bind the function body onto the current scope_inout.
			const auto scope_func = std::make_shared<Scope>(Scope::purpose_lexical, scope_inout);
			prepare_function_scope_lexical(scope_func, cand.location, cand.params);
			auto bound_body = bind_block_in_place(scope_func, cand.body);
			// Create a reference for the function.
			auto func = std::make_shared<Function>("function", cand.location, cand.params, std::move(bound_body));
			Vp<Value> func_var;
			set_value(func_var, D_function(std::move(func)));
			Reference::S_temporary_value ref_t = { std::move(func_var) };
			set_reference(result_out, std::move(ref_t));
			materialize_reference(result_out, true);
			const auto wref = scope_inout->mutate_named_reference(cand.id);
			copy_reference(wref, result_out);
			return Statement::execution_result_next; }

		case Statement::index_if_statement: {
			const auto &cand = stmt.as<Statement::S_if_statement>();
			// Evaluate the condition expression and select a branch basing on the result.
			evaluate_expression(result_out, cand.cond, scope_inout);
			const auto cond_var = read_reference_opt(result_out);
			const auto &branch_taken = test_value(cond_var) ? cand.branch_true : cand.branch_false;
			if(branch_taken.empty() == false){
				// Execute the branch recursively.
				const auto result = execute_block(result_out, branch_taken, scope_inout);
				if(result != Statement::execution_result_next){
					// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
					return result;
				}
			}
			return Statement::execution_result_next; }

		case Statement::index_switch_statement: {
			const auto &cand = stmt.as<Statement::S_switch_statement>();
			// Evaluate the control expression.
			evaluate_expression(result_out, cand.ctrl, scope_inout);
			const auto ctrl_var = read_reference_opt(result_out);
			ASTERIA_DEBUG_LOG("Switching on `", ctrl_var, "`...");
			// Traverse the clause list to find one that matches the result.
			// Notice that clauses in a `switch` statement share the same scope_inout.
			const auto scope_switch = std::make_shared<Scope>(Scope::purpose_plain, scope_inout);
			auto match_it = cand.clauses.end();
			for(auto it = cand.clauses.begin(); it != cand.clauses.end(); ++it){
				if(it->pred.empty()){
					// Deal with a `default` label.
					if(match_it != cand.clauses.end()){
						ASTERIA_THROW_RUNTIME_ERROR("More than one `default` labels have been found in this `switch` statement.");
					}
					match_it = it;
				} else {
					// Deal with a `case` label.
					evaluate_expression(result_out, it->pred, scope_switch);
					const auto case_var = read_reference_opt(result_out);
					if(compare_values(ctrl_var, case_var) == Value::comparison_equal){
						match_it = it;
						break;
					}
				}
				for(const auto &stmt_skipped : it->body){
					do_skip_statement_in_place(scope_switch, stmt_skipped);
				}
			}
			// Iterate from the match clause to the end of the body, falling through clause ends if any.
			for(auto it = match_it; it != cand.clauses.end(); ++it){
				// Execute the clause recursively.
				const auto result = execute_block_in_place(result_out, scope_switch, it->body);
				if((result == Statement::execution_result_break_unspecified) || (result == Statement::execution_result_break_switch)){
					// Break out of the body as requested.
					break;
				}
				if(result != Statement::execution_result_next){
					// Forward anything unexpected to the caller.
					return result;
				}
			}
			return Statement::execution_result_next; }

		case Statement::index_do_while_statement: {
			const auto &cand = stmt.as<Statement::S_do_while_statement>();
			do {
				// Execute the loop body recursively.
				const auto result = execute_block(result_out, cand.body, scope_inout);
				if((result == Statement::execution_result_break_unspecified) || (result == Statement::execution_result_break_while)){
					// Break out of the body as requested.
					break;
				}
				if((result != Statement::execution_result_next) && (result != Statement::execution_result_continue_unspecified) && (result != Statement::execution_result_continue_while)){
					// Forward anything unexpected to the caller.
					return result;
				}
				// Evaluate the condition expression and decide whether to start a new loop basing on the result.
			} while(do_check_loop_condition(result_out, cand.cond, scope_inout));
			return Statement::execution_result_next; }

		case Statement::index_while_statement: {
			const auto &cand = stmt.as<Statement::S_while_statement>();
			// Evaluate the condition expression and decide whether to start a new loop basing on the result.
			while(do_check_loop_condition(result_out, cand.cond, scope_inout)){
				// Execute the loop body recursively.
				const auto result = execute_block(result_out, cand.body, scope_inout);
				if((result == Statement::execution_result_break_unspecified) || (result == Statement::execution_result_break_while)){
					// Break out of the body as requested.
					break;
				}
				if((result != Statement::execution_result_next) && (result != Statement::execution_result_continue_unspecified) && (result != Statement::execution_result_continue_while)){
					// Forward anything unexpected to the caller.
					return result;
				}
			}
			return Statement::execution_result_next; }

		case Statement::index_for_statement: {
			const auto &cand = stmt.as<Statement::S_for_statement>();
			// The scope_inout of the lopp initialization outlasts the scope_inout of the loop body, which will be
			// created and destroyed upon entrance and exit of each iteration.
			const auto scope_for = std::make_shared<Scope>(Scope::purpose_plain, scope_inout);
			// Perform loop initialization.
			auto result = execute_block_in_place(result_out, scope_for, cand.init);
			if(result != Statement::execution_result_next){
				// The initialization is considered to be outside the loop body.
				// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
				return result;
			}
			// Evaluate the condition expression and decide whether to start a new loop basing on the result.
			while(do_check_loop_condition(result_out, cand.cond, scope_for)){
				// Execute the loop body recursively.
				result = execute_block(result_out, cand.body, scope_for);
				if((result == Statement::execution_result_break_unspecified) || (result == Statement::execution_result_break_for)){
					// Break out of the body as requested.
					break;
				}
				if((result != Statement::execution_result_next) && (result != Statement::execution_result_continue_unspecified) && (result != Statement::execution_result_continue_for)){
					// Forward anything unexpected to the caller.
					return result;
				}
				// Step to the next iteration.
				evaluate_expression(result_out, cand.step, scope_for);
			}
			return Statement::execution_result_next; }

		case Statement::index_for_each_statement: {
			const auto &cand = stmt.as<Statement::S_for_each_statement>();
			// The scope_inout of the loop initialization outlasts the scope_inout of the loop body, which will be
			// created and destroyed upon entrance and exit of each iteration.
			const auto scope_for = std::make_shared<Scope>(Scope::purpose_plain, scope_inout);
			// Perform loop initialization.
			evaluate_initializer(result_out, cand.range_init, scope_for);
			materialize_reference(result_out, true);
			Vp<Reference> range_ref;
			move_reference(range_ref, std::move(result_out));
			const auto range_var = read_reference_opt(range_ref);
			const auto range_type = get_value_type(range_var);
			if(range_type == Value::type_array){
				// Save the size. This is necessary because the array might be subsequently altered.
				std::ptrdiff_t size;
				{
					const auto &range_array = range_var->as<D_array>();
					size = static_cast<std::ptrdiff_t>(range_array.size());
				}
				Vp<Value> key_var;
				Vp<Reference> temp_ref;
				for(std::ptrdiff_t index = 0; index < size; ++index){
					// Set the key, which is an integer.
					set_value(key_var, D_integer(index));
					const auto key_wref = scope_for->mutate_named_reference(cand.key_id);
					Reference::S_constant ref_k = { key_var.cshare() };
					set_reference(key_wref, std::move(ref_k));
					// Set the value, which is an array element.
					copy_reference(temp_ref, range_ref);
					const auto value_wref = scope_for->mutate_named_reference(cand.value_id);
					Reference::S_array_element ref_ae = { std::move(temp_ref), index };
					set_reference(value_wref, std::move(ref_ae));
					// Execute the loop body recursively.
					const auto result = execute_block(result_out, cand.body, scope_for);
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
				Vector<D_string> backup_keys;
				{
					const auto &range_object = range_var->as<D_object>();
					backup_keys.reserve(range_object.size());
					for(const auto &pair : range_object){
						backup_keys.emplace_back(pair.first);
					}
				}
				Vp<Value> key_var;
				Vp<Reference> temp_ref;
				for(auto &key : backup_keys){
					// Set the key, which is an integer.
					set_value(key_var, D_string(key));
					const auto key_wref = scope_for->mutate_named_reference(cand.key_id);
					Reference::S_constant ref_k = { key_var.cshare() };
					set_reference(key_wref, std::move(ref_k));
					// Set the value, which is an object member.
					copy_reference(temp_ref, range_ref);
					const auto value_wref = scope_for->mutate_named_reference(cand.value_id);
					Reference::S_object_member ref_om = { std::move(temp_ref), std::move(key) };
					set_reference(value_wref, std::move(ref_om));
					// Execute the loop body recursively.
					const auto result = execute_block(result_out, cand.body, scope_for);
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
			return Statement::execution_result_next; }

		case Statement::index_try_statement: {
			const auto &cand = stmt.as<Statement::S_try_statement>();
			try {
				// Execute the `try` branch in a C++ `try...catch` statement.
				const auto scope_try = std::make_shared<Scope>(Scope::purpose_plain, scope_inout);
				const auto result = execute_block_in_place(result_out, scope_try, cand.branch_try);
				if(result != Statement::execution_result_next){
					// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
					return result;
				}
			} catch(...){
				// Execute the `catch` branch.
				const auto scope_catch = std::make_shared<Scope>(Scope::purpose_plain, scope_inout);
				// Translate the exception object.
				try {
					throw;
				} catch(Exception &e){
					ASTERIA_DEBUG_LOG("Caught `Asteria::Exception`: ", e.get_reference_opt());
					// Print exceptions nested, if any.
					auto nested_eptr = e.nested_ptr();
					if(nested_eptr){
						static constexpr char s_prefix[] = "which contains a nested ";
						int prefix_width = static_cast<int>(std::strlen(s_prefix));
						do {
							prefix_width += 2;
							try {
								std::rethrow_exception(nested_eptr);
							} catch(Exception &ne){
								ASTERIA_DEBUG_LOG(std::setw(prefix_width), s_prefix, "`Asteria::Exception`: ", ne.get_reference_opt());
								nested_eptr = ne.nested_ptr();
							} catch(std::exception &ne){
								ASTERIA_DEBUG_LOG(std::setw(prefix_width), s_prefix, "`std::exception`: ", ne.what());
								nested_eptr = nullptr;
							}
						} while(nested_eptr);
					}
					// Copy the reference into the scope.
					copy_reference(result_out, e.get_reference_opt());
					materialize_reference(result_out, true);
					const auto wref = scope_catch->mutate_named_reference(cand.except_id);
					copy_reference(wref, result_out);
				} catch(std::exception &e){
					ASTERIA_DEBUG_LOG("Caught `std::exception`: ", e.what());
					// Create a string containing the error message in the scope.
					Vp<Value> what_var;
					set_value(what_var, D_string(e.what()));
					Reference::S_temporary_value ref_t = { std::move(what_var) };
					set_reference(result_out, std::move(ref_t));
					materialize_reference(result_out, true);
					const auto wref = scope_catch->mutate_named_reference(cand.except_id);
					copy_reference(wref, result_out);
				} catch(...){
					ASTERIA_DEBUG_LOG("Caught an unknown exception...");
					// Create a null reference in the scope.
					const auto wref = scope_catch->mutate_named_reference(cand.except_id);
					set_reference(wref, nullptr);
				}
				const auto result = execute_block_in_place(result_out, scope_catch, cand.branch_catch);
				if(result != Statement::execution_result_next){
					// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
					return result;
				}
			}
			return Statement::execution_result_next; }

		case Statement::index_defer_statement: {
			const auto &cand = stmt.as<Statement::S_defer_statement>();
			// Bind the function body onto the current scope_inout. There are no params.
			auto bound_body = bind_block(cand.body, scope_inout);
			// Register the function as a deferred callback of the current scope_inout.
			auto func = std::make_shared<Function>("deferred block", cand.location, Vector<Cow_string>(), std::move(bound_body));
			scope_inout->defer_callback(std::move(func));
			return Statement::execution_result_next; }

		case Statement::index_break_statement: {
			const auto &cand = stmt.as<Statement::S_break_statement>();
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
				ASTERIA_DEBUG_LOG("Unsupported target scope_inout enumeration `", cand.target_scope, "`. This is probably a bug. Please report.");
				std::terminate();
			}
			return Statement::execution_result_next; }

		case Statement::index_continue_statement: {
			const auto &cand = stmt.as<Statement::S_continue_statement>();
			switch(cand.target_scope){
			case Statement::target_scope_unspecified:
				return Statement::execution_result_continue_unspecified;
			case Statement::target_scope_switch:
				ASTERIA_DEBUG_LOG("`switch` is not allowed to follow `continue`. This is probably a bug. Please report.");
				std::terminate();
			case Statement::target_scope_while:
				return Statement::execution_result_continue_while;
			case Statement::target_scope_for:
				return Statement::execution_result_continue_for;
			default:
				ASTERIA_DEBUG_LOG("Unsupported target scope_inout enumeration `", cand.target_scope, "`. This is probably a bug. Please report.");
				std::terminate();
			}
			return Statement::execution_result_next; }

		case Statement::index_throw_statement: {
			const auto &cand = stmt.as<Statement::S_throw_statement>();
			// Evaluate the operand, then throw the exception constructed from the result of it.
			evaluate_expression(result_out, cand.operand, scope_inout);
			ASTERIA_DEBUG_LOG("Throwing exception: ", result_out);
			throw Exception(result_out.cshare()); }

		case Statement::index_return_statement: {
			const auto &cand = stmt.as<Statement::S_return_statement>();
			// Evaluate the operand, then return because the value is stored outside this function.
			evaluate_expression(result_out, cand.operand, scope_inout);
			return Statement::execution_result_return; }

		default:
			ASTERIA_DEBUG_LOG("An unknown statement type enumeration `", type, "` is encountered. This is probably a bug. Please report.");
			std::terminate();
		}
	}
}

Statement::Execution_result execute_block_in_place(Vp<Reference> &ref_out, Sp_cref<Scope> scope_inout, const Block &block){
	// Execute statements recursively.
	for(const auto &stmt : block){
		const auto result = do_execute_statement_in_place(ref_out, scope_inout, stmt);
		if(result != Statement::execution_result_next){
			// Forward anything unexpected to the caller.
			return result;
		}
	}
	return Statement::execution_result_next;
}

Block bind_block(const Block &block, Sp_cref<const Scope> scope){
	if(block.empty()){
		return Block();
	}
	const auto scope_working = std::make_shared<Scope>(Scope::purpose_lexical, scope);
	return bind_block_in_place(scope_working, block);
}
Statement::Execution_result execute_block(Vp<Reference> &ref_out, const Block &block, Sp_cref<const Scope> scope){
	if(block.empty()){
		return Statement::execution_result_return;
	}
	const auto scope_working = std::make_shared<Scope>(Scope::purpose_plain, scope);
	return execute_block_in_place(ref_out, scope_working, block);
}

}
