// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "variable.hpp"
#include "reference.hpp"
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
			// Push it onto the stack. The result is an rvalue.
			Reference::S_rvalue_static ref_s = { params.source_opt };
			auto ref = Xptr<Reference>(std::make_shared<Reference>(std::move(ref_s)));
			stack.emplace_back(std::move(ref));
			continue; }
		case Expression_node::type_named_reference: {
			const auto &params = node.get<Expression_node::S_named_reference>();
			// Look up the reference in the enclosing scope.
			auto ref = scope->get_reference_recursive_opt(params.identifier);
			if(!ref){
				ASTERIA_THROW_RUNTIME_ERROR("Referring an undeclared identifier `", params.identifier, "`");
			}
			// Push the reference onto the stack as-is.
			stack.emplace_back(std::move(ref));
			continue; }
		case Expression_node::type_subexpression: {
			const auto &params = node.get<Expression_node::S_subexpression>();
			// Evaluate the subexpression recursively.
			auto ref = evaluate_expression_recursive_opt(recycler, scope, params.subexpression_opt);
			// Push the result reference onto the stack as-is.
			stack.emplace_back(std::move(ref));
			continue; }
		case Expression_node::type_branch: {
			const auto &params = node.get<Expression_node::S_branch>();
			// Pop the condition off the stack.
			if(stack.size() < 1){
				ASTERIA_THROW_RUNTIME_ERROR("No operand found for this branch node");
			}
			auto condition_ref = std::move(stack.back());
			stack.pop_back();
			const auto condition = read_reference_opt(condition_ref);
			// Pick a branch basing on the condition.
			auto ref = evaluate_expression_recursive_opt(recycler, scope, test_variable(condition) ? params.branch_true_opt : params.branch_false_opt);
			if(!ref){
				// If the branch selected is absent, push `condition_ref` instead.
				ref = std::move(condition_ref);
			}
			stack.emplace_back(std::move(ref));
			continue; }
		case Expression_node::type_function_call: {
			const auto &params = node.get<Expression_node::S_function_call>();
			// Pop the function off the stack.
			if(stack.size() < 1){
				ASTERIA_THROW_RUNTIME_ERROR("No operand found for this function call node");
			}
			auto function_ref = std::move(stack.back());
			stack.pop_back();
			const auto function_variable = read_reference_opt(function_ref);
			// Make sure it is really a function.
			if(get_variable_type(function_variable) != Variable::type_function){
				ASTERIA_THROW_RUNTIME_ERROR("Attempting to call something having type `", get_variable_type_name(function_variable), "`, which is not a function");
			}
			const auto &function = function_variable->get<D_function>();
			// Reserve size for the argument vector. There will not be fewer arguments than parameters.
			boost::container::vector<Xptr<Reference>> arguments;
			arguments.resize(std::max(params.number_of_arguments, function.default_argument_generators_opt.size()));
			// Pop arguments off the stack.
			if(stack.size() < params.number_of_arguments){
				ASTERIA_THROW_RUNTIME_ERROR("No enough arguments pushed for this function call node");
			}
			for(std::size_t i = 0; i < params.number_of_arguments; ++i){
				auto argument_ref = std::move(stack.back());
				stack.pop_back();
				arguments.at(i) = std::move(argument_ref);
			}
			// If a null argument is specified, replace it with its corresponding default argument.
			for(std::size_t i = 0; i < function.default_argument_generators_opt.size(); ++i){
				if(arguments.at(i)){
					continue;
				}
				const auto &generator = function.default_argument_generators_opt.at(i);
				if(!generator){
					continue;
				}
				arguments.at(i) = generator(recycler);
			}
			// Call the function and push the result as-is.
			auto ref = function.function(recycler, std::move(arguments));
			stack.emplace_back(std::move(ref));
			continue; }
		case Expression_node::type_operator_rpn: {
			continue; }
		case Expression_node::type_lambda_definition: {
			ASTERIA_THROW_RUNTIME_ERROR("TODO TODO not implemented");
			continue; }
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
