// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "block.hpp"
#include "statement.hpp"
#include "scope.hpp"
#include "expression.hpp"
#include "initializer.hpp"
#include "stored_value.hpp"
#include "stored_reference.hpp"
#include "local_variable.hpp"
#include "instantiated_function.hpp"
#include "exception.hpp"
#include "utilities.hpp"

namespace Asteria {

Block::Block(Block &&) noexcept = default;
Block &Block::operator=(Block &&) = default;
Block::~Block() = default;

namespace {
	bool do_check_loop_condition(Xptr<Reference> &reference_out, Spcref<Recycler> recycler, Spcref<const Expression> condition_opt, Spcref<const Scope> scope){
		if(condition_opt == nullptr){
			reference_out.reset();
			return true;
		} else {
			evaluate_expression(reference_out, recycler, condition_opt, scope);
			const auto condition_var = read_reference_opt(reference_out);
			return test_variable(condition_var);
		}
	}
}

void bind_block_in_place(Xptr<Block> &bound_result_out, Spcref<Scope> scope, Spcref<const Block> block_opt){
	if(block_opt == nullptr){
		// Return a null block.
		return bound_result_out.reset();
	}
	// Bind statements recursively.
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
			bind_block_in_place(bound_body, scope_lexical, params.body_opt);
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
			// Notice that clauses in a `switch` statement share the same scope.
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
		case Statement::type_for_each_statement: {
			const auto &params = stmt.get<Statement::S_for_each_statement>();
			// The scope of the lopp initialization outlasts the scope of the loop body, which will be
			// created and destroyed upon entrance and exit of each iteration.
			const auto scope_for = std::make_shared<Scope>(Scope::purpose_lexical, scope);
			// Bind the loop range initializer recursively.
			Xptr<Initializer> bound_range_init;
			bind_initializer(bound_range_init, params.range_initializer_opt, scope_for);
			// Create null local references for the key and the value.
			const auto key_wref = scope->drill_for_local_reference(params.key_identifier);
			const auto value_wref = scope->drill_for_local_reference(params.value_identifier);
			set_reference(key_wref, nullptr);
			set_reference(value_wref, nullptr);
			// Bind the body recursively.
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope);
			Statement::S_for_each_statement stmt_f = { params.key_identifier, params.value_identifier, std::move(bound_range_init), std::move(bound_body) };
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
	reference_out.reset();
	if(block_opt == nullptr){
		// Nothing to do.
		return Block::execution_result_end_of_block;
	}
	// Execute statements in the order defined.
	for(std::size_t stmt_index = 0; stmt_index < block_opt->size(); ++stmt_index){
		const auto &stmt = block_opt->at(stmt_index);
		const auto type = stmt.get_type();
		switch(type){
		case Statement::type_expression_statement: {
			const auto &params = stmt.get<Statement::S_expression_statement>();
			// Evaluate the expression, storing the result into `reference_out`.
			evaluate_expression(reference_out, recycler, params.expression_opt, scope);
			break; }
		case Statement::type_variable_definition: {
			const auto &params = stmt.get<Statement::S_variable_definition>();
			// Create a local variable for the variable.
			Xptr<Variable> var;
			initialize_variable(var, recycler, params.initializer_opt, scope);
			auto local_var = std::make_shared<Local_variable>(std::move(var), params.immutable);
			const auto wref = scope->drill_for_local_reference(params.identifier);
			Reference::S_local_variable ref_l = { std::move(local_var) };
			set_reference(wref, std::move(ref_l));
			copy_reference(reference_out, wref.get());
			break; }
		case Statement::type_function_definition: {
			const auto &params = stmt.get<Statement::S_function_definition>();
			// Bind the function body onto the current scope.
			const auto scope_lexical = std::make_shared<Scope>(Scope::purpose_lexical, scope);
			prepare_function_scope_lexical(scope_lexical, params.parameters_opt);
			Xptr<Block> bound_body;
			bind_block_in_place(bound_body, scope_lexical, params.body_opt);
			// Create a local variable for the function.
			Xptr<Variable> var;
			auto func = std::make_shared<Instantiated_function>(params.parameters_opt, scope, std::move(bound_body));
			set_variable(var, recycler, D_function(std::move(func)));
			auto local_var = std::make_shared<Local_variable>(std::move(var), true);
			const auto wref = scope->drill_for_local_reference(params.identifier);
			Reference::S_local_variable ref_l = { std::move(local_var) };
			set_reference(wref, std::move(ref_l));
			copy_reference(reference_out, wref.get());
			break; }
		case Statement::type_if_statement: {
			const auto &params = stmt.get<Statement::S_if_statement>();
			// Evaluate the condition expression and select a branch basing on the result.
			evaluate_expression(reference_out, recycler, params.condition_opt, scope);
			const auto condition_var = read_reference_opt(reference_out);
			const auto branch_taken = test_variable(condition_var) ? params.branch_true_opt.share() : params.branch_false_opt.share();
			if(!branch_taken){
				// If the branch is absent, don't execute anything further.
				break;
			}
			// Execute the branch recursively.
			const auto result = execute_block(reference_out, recycler, branch_taken, scope);
			if(result != Block::execution_result_end_of_block){
				// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
				return result;
			}
			break; }
		case Statement::type_switch_statement: {
			const auto &params = stmt.get<Statement::S_switch_statement>();
			// Evaluate the control expression.
			evaluate_expression(reference_out, recycler, params.control_opt, scope);
			const auto control_var = read_reference_opt(reference_out);
			ASTERIA_DEBUG_LOG("Switching: ", control_var);
			// Traverse the clause list to find one that matches the result.
			// Notice that clauses in a `switch` statement share the same scope.
			const auto scope_switch = std::make_shared<Scope>(Scope::purpose_plain, scope);
			auto match_it = params.clauses_opt.end();
			for(auto it = params.clauses_opt.begin(); it != params.clauses_opt.end(); ++it){
				if(it->first){
					// Deal with a `case` label.
					evaluate_expression(reference_out, recycler, it->first, scope_switch);
					const auto case_var = read_reference_opt(reference_out);
					if(compare_variables(control_var, case_var) == Variable::comparison_result_equal){
						match_it = it;
						break;
					}
				} else {
					// Deal with a `default` label.
					if(match_it != params.clauses_opt.end()){
						ASTERIA_THROW_RUNTIME_ERROR("More than one `default` labels found in this `switch` statement");
					}
					match_it = it;
				}
			}
			// Iterate from the match clause to the end of the body, falling through clauses ends if any.
			for(auto it = match_it; it != params.clauses_opt.end(); ++it){
				// Execute the clause recursively.
				const auto result = execute_block_in_place(reference_out, scope_switch, recycler, it->second);
				if((result == Block::execution_result_break_unspecified) || (result == Block::execution_result_break_switch)){
					// Break out of the body as requested.
					break;
				}
				if(result != Block::execution_result_end_of_block){
					// Forward anything unexpected to the caller.
					return result;
				}
			}
			break; }
		case Statement::type_do_while_statement: {
			const auto &params = stmt.get<Statement::S_do_while_statement>();
			do {
				// Execute the loop body recursively.
				const auto result = execute_block(reference_out, recycler, params.body_opt, scope);
				if((result == Block::execution_result_break_unspecified) || (result == Block::execution_result_break_while)){
					// Break out of the body as requested.
					break;
				}
				if((result != Block::execution_result_end_of_block) && (result != Block::execution_result_continue_unspecified) && (result != Block::execution_result_continue_while)){
					// Forward anything unexpected to the caller.
					return result;
				}
				// Evaluate the condition expression and decide whether to start a new loop basing on the result.
			} while(do_check_loop_condition(reference_out, recycler, params.condition_opt, scope));
			break; }
		case Statement::type_while_statement: {
			const auto &params = stmt.get<Statement::S_while_statement>();
			// Evaluate the condition expression and decide whether to start a new loop basing on the result.
			while(do_check_loop_condition(reference_out, recycler, params.condition_opt, scope)){
				// Execute the loop body recursively.
				const auto result = execute_block(reference_out, recycler, params.body_opt, scope);
				if((result == Block::execution_result_break_unspecified) || (result == Block::execution_result_break_while)){
					// Break out of the body as requested.
					break;
				}
				if((result != Block::execution_result_end_of_block) && (result != Block::execution_result_continue_unspecified) && (result != Block::execution_result_continue_while)){
					// Forward anything unexpected to the caller.
					return result;
				}
			}
			break; }
		case Statement::type_for_statement: {
			const auto &params = stmt.get<Statement::S_for_statement>();
			// The scope of the lopp initialization outlasts the scope of the loop body, which will be
			// created and destroyed upon entrance and exit of each iteration.
			const auto scope_for = std::make_shared<Scope>(Scope::purpose_plain, scope);
			// Perform loop initialization.
			auto result = execute_block_in_place(reference_out, scope_for, recycler, params.initialization_opt);
			if(result != Block::execution_result_end_of_block){
				// The initialization is considered to be outside the loop body.
				// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
				return result;
			}
			// Evaluate the condition expression and decide whether to start a new loop basing on the result.
			while(do_check_loop_condition(reference_out, recycler, params.condition_opt, scope)){
				// Execute the loop body recursively.
				result = execute_block(reference_out, recycler, params.body_opt, scope_for);
				if((result == Block::execution_result_break_unspecified) || (result == Block::execution_result_break_for)){
					// Break out of the body as requested.
					break;
				}
				if((result != Block::execution_result_end_of_block) && (result != Block::execution_result_continue_unspecified) && (result != Block::execution_result_continue_for)){
					// Forward anything unexpected to the caller.
					return result;
				}
				// Perform loop increment.
				evaluate_expression(reference_out, recycler, params.increment_opt, scope_for);
			}
			break; }
		case Statement::type_for_each_statement: {
			const auto &params = stmt.get<Statement::S_for_each_statement>();
			// The scope of the loop initialization outlasts the scope of the loop body, which will be
			// created and destroyed upon entrance and exit of each iteration.
			const auto scope_for = std::make_shared<Scope>(Scope::purpose_plain, scope);
			// Perform loop initialization.
			Xptr<Variable> range_var;
			initialize_variable(range_var, recycler, params.range_initializer_opt, scope_for);
			const auto range_type = get_variable_type(range_var);
			// Create the range reference that will be used to create references to values.
			Xptr<Reference> range_ref_backup, range_ref;
			Reference::S_temporary_value ref_r = { std::move(range_var) };
			set_reference(range_ref_backup, std::move(ref_r));
			materialize_reference(range_ref_backup, recycler, true);
			if(range_type == Variable::type_array){
				// Save the size. This is necessary because the array might be subsequently altered.
				std::ptrdiff_t size;
				{
					const auto &range_array = range_var->get<D_array>();
					size = static_cast<std::ptrdiff_t>(range_array.size());
				}
				// Prepare the key and value references.
				const auto key_wref = scope_for->drill_for_local_reference(params.key_identifier);
				const auto value_wref = scope_for->drill_for_local_reference(params.value_identifier);
				for(std::ptrdiff_t index = 0; index < size; ++index){
					// Set the key, which is an integer.
					set_variable(range_var, recycler, D_integer(index));
					Reference::S_constant ref_k = { range_var };
					set_reference(key_wref, std::move(ref_k));
					// Set the value, which is an array element.
					copy_reference(range_ref, range_ref_backup);
					Reference::S_array_element ref_v = { std::move(range_ref), index };
					set_reference(value_wref, std::move(ref_v));
					// Execute the loop body recursively.
					const auto result = execute_block(reference_out, recycler, params.body_opt, scope_for);
					if((result == Block::execution_result_break_unspecified) || (result == Block::execution_result_break_for)){
						// Break out of the body as requested.
						break;
					}
					if((result != Block::execution_result_end_of_block) && (result != Block::execution_result_continue_unspecified) && (result != Block::execution_result_continue_for)){
						// Forward anything unexpected to the caller.
						return result;
					}
				}
			} else if(range_type == Variable::type_object){
				// Save the keys. This is necessary because the object might be subsequently altered.
				std::vector<std::string> backup_keys;
				{
					const auto &range_object = range_var->get<D_object>();
					backup_keys.reserve(range_object.size());
					for(const auto &pair : range_object){
						backup_keys.emplace_back(pair.first);
					}
				}
				// Prepare the key and value references.
				const auto key_wref = scope_for->drill_for_local_reference(params.key_identifier);
				const auto value_wref = scope_for->drill_for_local_reference(params.value_identifier);
				for(auto &key : backup_keys){
					// Set the key, which is a string.
					set_variable(range_var, recycler, D_string(key));
					Reference::S_constant ref_k = { range_var };
					set_reference(key_wref, std::move(ref_k));
					// Set the value, which is an object member.
					copy_reference(range_ref, range_ref_backup);
					Reference::S_object_member ref_v = { std::move(range_ref), std::move(key) };
					set_reference(value_wref, std::move(ref_v));
					// Execute the loop body recursively.
					const auto result = execute_block(reference_out, recycler, params.body_opt, scope_for);
					if((result == Block::execution_result_break_unspecified) || (result == Block::execution_result_break_for)){
						// Break out of the body as requested.
						break;
					}
					if((result != Block::execution_result_end_of_block) && (result != Block::execution_result_continue_unspecified) && (result != Block::execution_result_continue_for)){
						// Forward anything unexpected to the caller.
						return result;
					}
				}
			} else {
				ASTERIA_THROW_RUNTIME_ERROR("Invalid ranged `for` statement on something having type `", get_type_name(range_type), "`");
			}
			break; }
		case Statement::type_try_statement: {
			const auto &params = stmt.get<Statement::S_try_statement>();
			// Execute the try branch in a C++ `try...catch` statement.
			try {
				const auto scope_try = std::make_shared<Scope>(Scope::purpose_plain, scope);
				const auto result = execute_block_in_place(reference_out, scope_try, recycler, params.branch_try_opt);
				if(result != Block::execution_result_end_of_block){
					// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
					return result;
				}
			} catch(Exception &e){
				ASTERIA_DEBUG_LOG("Caught `Asteria::Exception`: ", e.get_reference_opt());
				const auto scope_catch = std::make_shared<Scope>(Scope::purpose_plain, scope);
				// Move the reference into the catch scope, then execute the handler.
				const auto wref = scope_catch->drill_for_local_reference(params.exception_identifier);
				move_reference(wref, std::move(e.get_reference_opt()));
				const auto result = execute_block_in_place(reference_out, scope_catch, recycler, params.branch_catch_opt);
				if(result != Block::execution_result_end_of_block){
					// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
					return result;
				}
			} catch(std::exception &e){
				ASTERIA_DEBUG_LOG("Caught `std::exception`: ", e.what());
				const auto scope_catch = std::make_shared<Scope>(Scope::purpose_plain, scope);
				// Create a string containing the error message in the catch scope, then execute the handler.
				Xptr<Variable> what_var;
				set_variable(what_var, recycler, D_string(e.what()));
				const auto wref = scope_catch->drill_for_local_reference(params.exception_identifier);
				Reference::S_temporary_value ref_t = { std::move(what_var) };
				set_reference(wref, std::move(ref_t));
				const auto result = execute_block_in_place(reference_out, scope_catch, recycler, params.branch_catch_opt);
				if(result != Block::execution_result_end_of_block){
					// If `break`, `continue` or `return` is encountered inside the branch, forward it to the caller.
					return result;
				}
			}
			break; }
		case Statement::type_defer_statement: {
			const auto &params = stmt.get<Statement::S_defer_statement>();
			// Bind the function body onto the current scope. There are no parameters.
			Xptr<Block> bound_body;
			bind_block(bound_body, params.body_opt, scope);
			// Register the function as a deferred callback of the current scope.
			auto func = std::make_shared<Instantiated_function>(nullptr, scope, std::move(bound_body));
			scope->defer_callback(std::move(func));
			break; }
		case Statement::type_break_statement: {
			const auto &params = stmt.get<Statement::S_break_statement>();
			switch(params.target_scope){
			case Statement::target_scope_unspecified:
				return Block::execution_result_break_unspecified;
			case Statement::target_scope_switch:
				return Block::execution_result_break_switch;
			case Statement::target_scope_while:
				return Block::execution_result_break_while;
			case Statement::target_scope_for:
				return Block::execution_result_break_for;
			default:
				ASTERIA_DEBUG_LOG("Unsupported target scope enumeration `", params.target_scope, "` at index `", stmt_index, "`. This is probably a bug, please report.");
				std::terminate();
			}
			break; }
		case Statement::type_continue_statement: {
			const auto &params = stmt.get<Statement::S_continue_statement>();
			switch(params.target_scope){
			case Statement::target_scope_unspecified:
				return Block::execution_result_continue_unspecified;
			case Statement::target_scope_switch:
				ASTERIA_DEBUG_LOG("`switch` is not allowed to follow `continue` at index `", stmt_index, "`. This is probably a bug, please report.");
				std::terminate();
			case Statement::target_scope_while:
				return Block::execution_result_continue_while;
			case Statement::target_scope_for:
				return Block::execution_result_continue_for;
			default:
				ASTERIA_DEBUG_LOG("Unsupported target scope enumeration `", params.target_scope, "` at index `", stmt_index, "`. This is probably a bug, please report.");
				std::terminate();
			}
			break; }
		case Statement::type_throw_statement: {
			const auto &params = stmt.get<Statement::S_throw_statement>();
			// Evaluate the operand, then throw the exception constructed from the result of it.
			evaluate_expression(reference_out, recycler, params.operand_opt, scope);
			ASTERIA_DEBUG_LOG("Throwing exception: ", reference_out);
			throw Exception(std::move(reference_out)); }
		case Statement::type_return_statement: {
			const auto &params = stmt.get<Statement::S_return_statement>();
			// Evaluate the operand, then return because the value is stored outside this function.
			evaluate_expression(reference_out, recycler, params.operand_opt, scope);
			return Block::execution_result_return; }
		default:
			ASTERIA_DEBUG_LOG("Unknown statement type enumeration `", type, "` at index `", stmt_index, "`. This is probably a bug, please report.");
			std::terminate();
		}
	}
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
