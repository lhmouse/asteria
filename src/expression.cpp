// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "variable.hpp"
#include "reference.hpp"
#include "stored_value.hpp"
#include "recycler.hpp"
#include "scope.hpp"
#include "utilities.hpp"
#include <cmath> // isless(), isgreater(), etc.

namespace Asteria {

Expression::Expression(Expression &&) = default;
Expression &Expression::operator=(Expression &&) = default;
Expression::~Expression() = default;

namespace {
	
}

Xptr<Reference> evaluate_expression_recursive_opt(Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Expression> expression_opt){
	// Return a null reference only when a null expression is given.
	if(!expression_opt){
		return nullptr;
	}
	// Parameters are pushed from right to left, in lexical order.
	boost::container::vector<Xptr<Reference>> stack;
	// Evaluate nodes in reverse-polish order.
	for(const auto &node : *expression_opt){
		const auto type = node.get_type();
		switch(type){
		case Expression_node::type_literal: {
			const auto &params = node.get<Expression_node::S_literal>();
			// Create an immutable reference to the constant.
			Xptr<Reference> ref;
			Reference::S_constant ref_c = { params.source_opt };
			ref.reset(std::make_shared<Reference>(std::move(ref_c)));
			stack.emplace_back(std::move(ref));
			break; }
		case Expression_node::type_named_reference: {
			const auto &params = node.get<Expression_node::S_named_reference>();
			// Look up the reference in the enclosing scope.
			Xptr<Reference> ref;
			auto scope_cur = scope;
			for(;;){
				if(!scope_cur){
					ASTERIA_THROW_RUNTIME_ERROR("Undeclared identifier `", params.identifier, "`");
				}
				copy_reference(ref, scope_cur->get_local_reference_opt(params.identifier));
				if(ref){
					break;
				}
				scope_cur = scope_cur->get_parent_opt();
			}
			// Push the reference onto the stack as-is.
			stack.emplace_back(std::move(ref));
			break; }
		case Expression_node::type_subexpression: {
			const auto &params = node.get<Expression_node::S_subexpression>();
			// Evaluate the subexpression recursively.
			auto ref = evaluate_expression_recursive_opt(recycler, scope, params.subexpression_opt);
			// Push the result reference onto the stack as-is.
			stack.emplace_back(std::move(ref));
			break; }
		case Expression_node::type_lambda_definition: {
//			const auto &params = node.get<Expression_node::S_lambda_definition>();
ASTERIA_THROW_RUNTIME_ERROR("TODO TODO not implemented");
			break; }
		case Expression_node::type_pruning: {
			const auto &params = node.get<Expression_node::S_pruning>();
			// Pop references requested.
			ASTERIA_VERIFY(stack.size() >= params.count_to_pop, ASTERIA_THROW_RUNTIME_ERROR("No enough arguments have been pushed"));
			for(std::size_t i = 0; i < params.count_to_pop; ++i){
				stack.pop_back();
			}
			break; }
		case Expression_node::type_branch: {
			const auto &params = node.get<Expression_node::S_branch>();
			// Pop the condition off the stack.
			ASTERIA_VERIFY(stack.size() >= 1, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for branch node"));
			auto condition_ref = std::move(stack.back());
			stack.pop_back();
			// Pick a branch basing on the condition.
			const auto condition_var = read_reference_opt(condition_ref);
			const auto branch_taken = test_variable(condition_var) ? params.branch_true_opt.share() : params.branch_false_opt.share();
			// If the branch exists, evaluate it. Otherwise, pick the condition instead.
			auto ref = evaluate_expression_recursive_opt(recycler, scope, branch_taken);
			if(!ref){
				ref = std::move(condition_ref);
			}
			stack.emplace_back(std::move(ref));
			break; }
		case Expression_node::type_function_call: {
			const auto &params = node.get<Expression_node::S_function_call>();
			// Pop the function off the stack.
			ASTERIA_VERIFY(stack.size() >= 1, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for function call node"));
			auto function_ref = std::move(stack.back());
			stack.pop_back();
			// Make sure it is really a function.
			const auto function_var = read_reference_opt(function_ref);
			if(get_variable_type(function_var) != Variable::type_function){
				ASTERIA_THROW_RUNTIME_ERROR("Attempting to call something having type `", get_variable_type_name(function_var), "`, which is not a function");
			}
			const auto &function = function_var->get<D_function>();
			// Allocate the argument vector. There will be no fewer arguments than parameters.
			boost::container::vector<Xptr<Reference>> arguments;
			arguments.resize(std::max(params.argument_count, function.default_arguments_opt.size()));
			// Pop arguments off the stack.
			ASTERIA_VERIFY(stack.size() >= params.argument_count, ASTERIA_THROW_RUNTIME_ERROR("No enough arguments have been pushed"));
			for(std::size_t i = 0; i < params.argument_count; ++i){
				arguments.at(i) = std::move(stack.back());
				stack.pop_back();
			}
			// Replace null arguments with default ones.
			for(std::size_t i = 0; i < function.default_arguments_opt.size(); ++i){
				if(arguments.at(i)){
					continue;
				}
				const auto &default_argument = function.default_arguments_opt.at(i);
				if(!default_argument){
					continue;
				}
				Reference::S_constant ref_c = { default_argument };
				arguments.at(i).reset(std::make_shared<Reference>(std::move(ref_c)));
			}
			// Call the function and push the result as-is.
			auto ref = function.function(recycler, std::move(arguments));
			stack.emplace_back(std::move(ref));
			break; }
		case Expression_node::type_operator_rpn: {
			const auto &params = node.get<Expression_node::S_operator_rpn>();
			switch(params.operator_generic){
			case Expression_node::operator_postfix_inc: {
				// Pop the operand off the stack.
				ASTERIA_VERIFY(stack.size() >= 1, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto lhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto lhs_var = read_reference_opt(lhs_ref);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
				switch(get_variable_type(lhs_var)){
				case Variable::type_integer: {
					// Increment the operand.
					const auto wref = drill_reference(lhs_ref);
					const auto lhs = lhs_var->get<D_integer>();
					set_variable(wref, recycler, static_cast<D_integer>(static_cast<std::uint64_t>(lhs) + 1));
					// Save the old value into `lhs_ref`, which will not be null here.
					Xptr<Variable> var;
					set_variable(var, recycler, lhs);
					Reference::S_temporary_value ref_d = { std::move(var) };
					lhs_ref->set(std::move(ref_d));
					break; }
				case Variable::type_double: {
					// Increment the operand.
					const auto wref = drill_reference(lhs_ref);
					const auto lhs = lhs_var->get<D_double>();
					set_variable(wref, recycler, std::isfinite(lhs) ? (lhs + 1) : lhs);
					// Save the old value into `lhs_ref`, which will not be null here.
					Xptr<Variable> var;
					set_variable(var, recycler, lhs);
					Reference::S_temporary_value ref_d = { std::move(var) };
					lhs_ref->set(std::move(ref_d));
					break; }
				default:
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", get_operator_name_generic(params.operator_generic), " operation on type `", get_variable_type_name(lhs_var), "`");
				}
#pragma GCC diagnostic pop
				stack.emplace_back(std::move(lhs_ref));
				break; }
			case Expression_node::operator_postfix_dec: {
				// Pop the operand off the stack.
				ASTERIA_VERIFY(stack.size() >= 1, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto lhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto lhs_var = read_reference_opt(lhs_ref);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
				switch(get_variable_type(lhs_var)){
				case Variable::type_integer: {
					// Decrement the operand.
					const auto wref = drill_reference(lhs_ref);
					const auto lhs = lhs_var->get<D_integer>();
					set_variable(wref, recycler, static_cast<D_integer>(static_cast<std::uint64_t>(lhs) - 1));
					// Save the old value into `lhs_ref`, which will not be null here.
					Xptr<Variable> var;
					set_variable(var, recycler, lhs);
					Reference::S_temporary_value ref_d = { std::move(var) };
					lhs_ref->set(std::move(ref_d));
					break; }
				case Variable::type_double: {
					// Decrement the operand.
					const auto wref = drill_reference(lhs_ref);
					const auto lhs = lhs_var->get<D_double>();
					set_variable(wref, recycler, std::isfinite(lhs) ? (lhs - 1) : lhs);
					// Save the old value into `lhs_ref`, which will not be null here.
					Xptr<Variable> var;
					set_variable(var, recycler, lhs);
					Reference::S_temporary_value ref_d = { std::move(var) };
					lhs_ref->set(std::move(ref_d));
					break; }
				default:
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", get_operator_name_generic(params.operator_generic), " operation on type `", get_variable_type_name(lhs_var), "`");
				}
#pragma GCC diagnostic pop
				stack.emplace_back(std::move(lhs_ref));
				break; }
			case Expression_node::operator_postfix_at: {
				// Pop two operands off the stack.
				ASTERIA_VERIFY(stack.size() >= 2, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto lhs_ref = std::move(stack.back());
				stack.pop_back();
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				// The second operand must have type `integer` (as an array subscript) or `string` (as an object subscript).
				const auto rhs_var = read_reference_opt(rhs_ref);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
				switch(get_variable_type(rhs_var)){
				case Variable::type_integer: {
					// Create an array element reference in `rhs_ref`, which will not be null here.
					Reference::S_array_element ref_a = { std::move(lhs_ref), rhs_var->get<D_integer>() };
					rhs_ref->set(std::move(ref_a));
					break; }
				case Variable::type_string: {
					// Create an object member reference in `rhs_ref`, which will not be null here.
					Reference::S_object_member ref_o = { std::move(lhs_ref), rhs_var->get<D_string>() };
					rhs_ref->set(std::move(ref_o));
					break; }
				default:
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", get_operator_name_generic(params.operator_generic), " operation on type `", get_variable_type_name(rhs_var), "`");
				}
#pragma GCC diagnostic pop
				stack.emplace_back(std::move(rhs_ref));
				break; }
			case Expression_node::operator_prefix_add: {
				// Pop the operand off the stack.
				ASTERIA_VERIFY(stack.size() >= 1, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				// N.B. This is one of the few operators that work on all types.
				if(rhs_ref){
					// Copy the operand to create an rvalue, then save it into `rhs_ref`.
					Xptr<Variable> value_new;
					extract_variable_from_reference(value_new, recycler, std::move(rhs_ref));
					Reference::S_temporary_value ref_d = { std::move(value_new) };
					rhs_ref->set(std::move(ref_d));
				}
				stack.emplace_back(std::move(rhs_ref));
				break; }
			case Expression_node::operator_prefix_sub: {
				// Pop the operand off the stack.
				ASTERIA_VERIFY(stack.size() >= 1, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto rhs_var = read_reference_opt(rhs_ref);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
				switch(get_variable_type(rhs_var)){
				case Variable::type_integer: {
					// Negate the operand to create an rvalue, then save it into `rhs_ref`.
					const auto rhs = rhs_var->get<D_integer>();
					Xptr<Variable> value_new;
					set_variable(value_new, recycler, static_cast<D_integer>(-(static_cast<std::uint64_t>(rhs))));
					Reference::S_temporary_value ref_d = { std::move(value_new) };
					rhs_ref->set(std::move(ref_d));
					break; }
				case Variable::type_double: {
					// Negate the operand to create an rvalue, then save it into `rhs_ref`.
					const auto rhs = rhs_var->get<D_double>();
					Xptr<Variable> value_new;
					set_variable(value_new, recycler, std::copysign(rhs, -1));
					Reference::S_temporary_value ref_d = { std::move(value_new) };
					rhs_ref->set(std::move(ref_d));
					break; }
				default:
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", get_operator_name_generic(params.operator_generic), " operation on type `", get_variable_type_name(rhs_var), "`");
				}
#pragma GCC diagnostic pop
				stack.emplace_back(std::move(rhs_ref));
				break; }
			case Expression_node::operator_prefix_not_b: {
				// Pop the operand off the stack.
				ASTERIA_VERIFY(stack.size() >= 1, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto rhs_var = read_reference_opt(rhs_ref);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
				switch(get_variable_type(rhs_var)){
				case Variable::type_boolean: {
					// Flip the operand to create an rvalue, then save it into `rhs_ref`.
					const auto rhs = rhs_var->get<D_boolean>();
					Xptr<Variable> value_new;
					set_variable(value_new, recycler, !rhs);
					Reference::S_temporary_value ref_d = { std::move(value_new) };
					rhs_ref->set(std::move(ref_d));
					break; }
				case Variable::type_integer: {
					// Flip all bits in the operand to create an rvalue, then save it into `rhs_ref`.
					const auto rhs = rhs_var->get<D_integer>();
					Xptr<Variable> value_new;
					set_variable(value_new, recycler, ~rhs);
					Reference::S_temporary_value ref_d = { std::move(value_new) };
					rhs_ref->set(std::move(ref_d));
					break; }
				default:
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", get_operator_name_generic(params.operator_generic), " operation on type `", get_variable_type_name(rhs_var), "`");
				}
#pragma GCC diagnostic pop
				stack.emplace_back(std::move(rhs_ref));
				break; }
			case Expression_node::operator_prefix_not_l: {
				// Pop the operand off the stack.
				ASTERIA_VERIFY(stack.size() >= 1, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				// N.B. This is one of the few operators that work on all types.
				const auto rhs_var = read_reference_opt(rhs_ref);
				// Convert the operand to a boolean value, negate it and return it.
				Xptr<Variable> value_new;
				const bool generalized_true = test_variable(rhs_var);
				set_variable(value_new, recycler, generalized_true == false);
				Reference::S_temporary_value ref_d = { std::move(value_new) };
				if(!rhs_ref){
					rhs_ref.reset(std::make_shared<Reference>(std::move(ref_d)));
				} else {
					rhs_ref->set(std::move(ref_d));
				}
				stack.emplace_back(std::move(rhs_ref));
				break; }
			case Expression_node::operator_prefix_inc: {
				// Pop the operand off the stack.
				ASTERIA_VERIFY(stack.size() >= 1, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto rhs_var = read_reference_opt(rhs_ref);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
				switch(get_variable_type(rhs_var)){
				case Variable::type_integer: {
					// Increment the operand.
					const auto wref = drill_reference(rhs_ref);
					const auto rhs = rhs_var->get<D_integer>();
					set_variable(wref, recycler, static_cast<D_integer>(static_cast<std::uint64_t>(rhs) + 1));
					break; }
				case Variable::type_double: {
					// Increment the operand.
					const auto wref = drill_reference(rhs_ref);
					const auto rhs = rhs_var->get<D_double>();
					set_variable(wref, recycler, std::isfinite(rhs) ? (rhs + 1) : rhs);
					break; }
				default:
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", get_operator_name_generic(params.operator_generic), " operation on type `", get_variable_type_name(rhs_var), "`");
				}
#pragma GCC diagnostic pop
				stack.emplace_back(std::move(rhs_ref));
				break; }
			case Expression_node::operator_prefix_dec: {
				// Pop the operand off the stack.
				ASTERIA_VERIFY(stack.size() >= 1, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto rhs_var = read_reference_opt(rhs_ref);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
				switch(get_variable_type(rhs_var)){
				case Variable::type_integer: {
					// Decrement the operand.
					const auto wref = drill_reference(rhs_ref);
					const auto rhs = rhs_var->get<D_integer>();
					set_variable(wref, recycler, static_cast<D_integer>(static_cast<std::uint64_t>(rhs) - 1));
					break; }
				case Variable::type_double: {
					// Decrement the operand.
					const auto wref = drill_reference(rhs_ref);
					const auto rhs = rhs_var->get<D_double>();
					set_variable(wref, recycler, std::isfinite(rhs) ? (rhs - 1) : rhs);
					break; }
				default:
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", get_operator_name_generic(params.operator_generic), " operation on type `", get_variable_type_name(rhs_var), "`");
				}
#pragma GCC diagnostic pop
				stack.emplace_back(std::move(rhs_ref));
				break; }
			case Expression_node::operator_infix_cmp_eq: {
				// Pop two operands off the stack.
				ASTERIA_VERIFY(stack.size() >= 2, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto lhs_ref = std::move(stack.back());
				stack.pop_back();
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				// Report unordered operands as being unequal.
				// N.B. This is one of the few operators that work on all types.
				Xptr<Variable> value_new;
				const auto comparison_result = compare_variables(lhs_var, rhs_var);
				set_variable(value_new, recycler, comparison_result == comparison_result_equal);
				Reference::S_temporary_value ref_d = { std::move(value_new) };
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				if(!lhs_ref){
					lhs_ref.reset(std::make_shared<Reference>(std::move(ref_d)));
				} else {
					lhs_ref->set(std::move(ref_d));
				}
				stack.emplace_back(std::move(lhs_ref));
				break; }
			case Expression_node::operator_infix_cmp_ne: {
				// Pop two operands off the stack.
				ASTERIA_VERIFY(stack.size() >= 2, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto lhs_ref = std::move(stack.back());
				stack.pop_back();
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				// Report unordered operands as being unequal.
				// N.B. This is one of the few operators that work on all types.
				Xptr<Variable> value_new;
				const auto comparison_result = compare_variables(lhs_var, rhs_var);
				set_variable(value_new, recycler, comparison_result != comparison_result_equal);
				Reference::S_temporary_value ref_d = { std::move(value_new) };
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				if(!lhs_ref){
					lhs_ref.reset(std::make_shared<Reference>(std::move(ref_d)));
				} else {
					lhs_ref->set(std::move(ref_d));
				}
				stack.emplace_back(std::move(lhs_ref));
				break; }
			case Expression_node::operator_infix_cmp_lt: {
				// Pop two operands off the stack.
				ASTERIA_VERIFY(stack.size() >= 2, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto lhs_ref = std::move(stack.back());
				stack.pop_back();
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				// Throw an exception if the operands are unordered.
				Xptr<Variable> value_new;
				const auto comparison_result = compare_variables(lhs_var, rhs_var);
				if(comparison_result == comparison_result_unordered){
					ASTERIA_THROW_RUNTIME_ERROR("Unordered operands for ", get_operator_name_generic(params.operator_generic));
				}
				set_variable(value_new, recycler, comparison_result == comparison_result_less);
				Reference::S_temporary_value ref_d = { std::move(value_new) };
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				if(!lhs_ref){
					lhs_ref.reset(std::make_shared<Reference>(std::move(ref_d)));
				} else {
					lhs_ref->set(std::move(ref_d));
				}
				stack.emplace_back(std::move(lhs_ref));
				break; }
			case Expression_node::operator_infix_cmp_gt: {
				// Pop two operands off the stack.
				ASTERIA_VERIFY(stack.size() >= 2, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto lhs_ref = std::move(stack.back());
				stack.pop_back();
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				// Throw an exception if the operands are unordered.
				Xptr<Variable> value_new;
				const auto comparison_result = compare_variables(lhs_var, rhs_var);
				if(comparison_result == comparison_result_unordered){
					ASTERIA_THROW_RUNTIME_ERROR("Unordered operands for ", get_operator_name_generic(params.operator_generic));
				}
				set_variable(value_new, recycler, comparison_result == comparison_result_greater);
				Reference::S_temporary_value ref_d = { std::move(value_new) };
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				if(!lhs_ref){
					lhs_ref.reset(std::make_shared<Reference>(std::move(ref_d)));
				} else {
					lhs_ref->set(std::move(ref_d));
				}
				stack.emplace_back(std::move(lhs_ref));
				break; }
			case Expression_node::operator_infix_cmp_lte: {
				// Pop two operands off the stack.
				ASTERIA_VERIFY(stack.size() >= 2, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto lhs_ref = std::move(stack.back());
				stack.pop_back();
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				// Throw an exception if the operands are unordered.
				Xptr<Variable> value_new;
				const auto comparison_result = compare_variables(lhs_var, rhs_var);
				if(comparison_result == comparison_result_unordered){
					ASTERIA_THROW_RUNTIME_ERROR("Unordered operands for ", get_operator_name_generic(params.operator_generic));
				}
				set_variable(value_new, recycler, comparison_result != comparison_result_greater);
				Reference::S_temporary_value ref_d = { std::move(value_new) };
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				if(!lhs_ref){
					lhs_ref.reset(std::make_shared<Reference>(std::move(ref_d)));
				} else {
					lhs_ref->set(std::move(ref_d));
				}
				stack.emplace_back(std::move(lhs_ref));
				break; }
			case Expression_node::operator_infix_cmp_gte: {
				// Pop two operands off the stack.
				ASTERIA_VERIFY(stack.size() >= 2, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto lhs_ref = std::move(stack.back());
				stack.pop_back();
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				// Throw an exception if the operands are unordered.
				Xptr<Variable> value_new;
				const auto comparison_result = compare_variables(lhs_var, rhs_var);
				if(comparison_result == comparison_result_unordered){
					ASTERIA_THROW_RUNTIME_ERROR("Unordered operands for ", get_operator_name_generic(params.operator_generic));
				}
				set_variable(value_new, recycler, comparison_result != comparison_result_less);
				Reference::S_temporary_value ref_d = { std::move(value_new) };
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				if(!lhs_ref){
					lhs_ref.reset(std::make_shared<Reference>(std::move(ref_d)));
				} else {
					lhs_ref->set(std::move(ref_d));
				}
				stack.emplace_back(std::move(lhs_ref));
				break; }
/*			case Expression_node::operator_infix_add: {
				stack.emplace_back(std::move(result_ref));
				break; }
			case Expression_node::operator_infix_sub: {
				stack.emplace_back(std::move(result_ref));
				break; }
			case Expression_node::operator_infix_mul: {
				stack.emplace_back(std::move(result_ref));
				break; }
			case Expression_node::operator_infix_div: {
				stack.emplace_back(std::move(result_ref));
				break; }
			case Expression_node::operator_infix_mod: {
				stack.emplace_back(std::move(result_ref));
				break; }
			case Expression_node::operator_infix_sll: {
				stack.emplace_back(std::move(result_ref));
				break; }
			case Expression_node::operator_infix_srl: {
				stack.emplace_back(std::move(result_ref));
				break; }
			case Expression_node::operator_infix_sra: {
				stack.emplace_back(std::move(result_ref));
				break; }
			case Expression_node::operator_infix_and: {
				stack.emplace_back(std::move(result_ref));
				break; }
			case Expression_node::operator_infix_or: {
				stack.emplace_back(std::move(result_ref));
				break; }
			case Expression_node::operator_infix_xor: {
				stack.emplace_back(std::move(result_ref));
				break; }
*/			case Expression_node::operator_infix_assign: {
				// Pop two operands off the stack.
				ASTERIA_VERIFY(stack.size() >= 2, ASTERIA_THROW_RUNTIME_ERROR("Missing operand for ", get_operator_name_generic(params.operator_generic)));
				auto lhs_ref = std::move(stack.back());
				stack.pop_back();
				auto rhs_ref = std::move(stack.back());
				stack.pop_back();
				const auto wref = drill_reference(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				// N.B. This is one of the few operators that work on all types.
				copy_variable(wref, recycler, rhs_var);
				stack.emplace_back(std::move(lhs_ref));
				break; }
			default:
				ASTERIA_DEBUG_LOG("Unknown operator enumeration `", params.operator_generic, "`. This is probably a bug, please report.");
				std::terminate();
			}
			break; }
		default:
			ASTERIA_DEBUG_LOG("Unknown type enumeration `", type, "`. This is probably a bug, please report.");
			std::terminate();
		}
	}
	// Get the result. If the stack is empty or has more than one elements, the expression is unbalanced.
	if(stack.size() != 1){
		ASTERIA_THROW_RUNTIME_ERROR("Unbalanced expression, number of references remaining is `", stack.size(), "`");
	}
	return std::move(stack.front());
}

}
