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

#if 0
	Reference get_default_argument_opt(Spref<Recycler> recycler, const D_function &function, std::size_t index){
		if(index >= function.default_arguments_opt.size()){
			return nullptr;
		}
		const auto &argument_generator = function.default_arguments_opt.at(index);
		if(!argument_generator){
			return nullptr;
		}
		Xptr<Variable> xvar;
		recycler->copy_variable_recursive(xvar, read_reference_opt(argument_generator(recycler)));
		Reference::S_rvalue_generic ref = { std::move(xvar) };
		return std::move(ref);
	}
}

Reference evaluate_expression_recursive(Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Expression> expression_opt){
		case Expression_node::type_literal: {
			const auto &params = node.get<Expression_node::S_literal>();
			// Copy the literal to create a new variable.
			Xptr<Variable> xvar;
			recycler->copy_variable_recursive(xvar, params.source_opt);
			Reference::S_rvalue_generic ref = { std::move(xvar) };
			stack.emplace_back(std::move(ref));
			continue; }
		case Expression_node::type_named_reference: {
			const auto &params = node.get<Expression_node::S_named_reference>();
			// Look up the variable in the enclosing scope.
			auto scoped_var = get_variable_recursive_opt(scope, params.identifier);
			if(!scoped_var){
				ASTERIA_THROW_RUNTIME_ERROR("Referring an undeclared identifier `", params.identifier, "`");
			}
			// Push a reference to it onto the stack. The result is an lvalue.
			Reference::S_lvalue_scoped_variable ref = { recycler, std::move(scoped_var) };
			stack.emplace_back(std::move(ref));
			continue; }
		case Expression_node::type_subexpression: {
			continue; }
		case Expression_node::type_branch: {
			const auto &params = node.get<Expression_node::S_branch>();
			// Pop an operand off the stack.
			if(stack.size() < 1){
				ASTERIA_THROW_RUNTIME_ERROR("No operand found for this branch node");
			}
			auto condition_ref = std::move(stack.back());
			stack.pop_back();
			const auto condition = read_reference_opt(condition_ref);
			// If the condition equals `true`, evaluate the true branch and push the result, otherwise evaluate the false branch and push the result.
			// If the branch selected is absent, push `condition_ref` instead.
			// This type of node implements the `&&`, `||` and `?:` operators.
			auto result_ref = evaluate_expression_recursive(recycler, scope, convert_to_boolean(condition) ? params.branch_true_opt : params.branch_false_opt);
			if(result_ref.get_type() == Reference::type_null){
				result_ref = std::move(condition_ref);
			}
			stack.emplace_back(std::move(result_ref));
			continue; }
		case Expression_node::type_function_call: {
			const auto &params = node.get<Expression_node::S_function_call>();
			// Pop the function off the stack.
			if(stack.size() < 1){
				ASTERIA_THROW_RUNTIME_ERROR("No operand found for this function call node");
			}
			auto callee_ref = std::move(stack.back());
			stack.pop_back();
			const auto callee = read_reference_opt(callee_ref);
			// Ensure this variable has type `D_function`.
			if(get_variable_type(callee) != Variable::type_function){
				ASTERIA_THROW_RUNTIME_ERROR("Attempting to call something having type `", get_variable_type_name(callee), "`, which is not a function");
			}
			const auto &function = callee->get<D_function>();
			// Pop arguments off the stack.
			if(stack.size() < params.number_of_arguments){
				ASTERIA_THROW_RUNTIME_ERROR("No enough arguments provided for this function call node");
			}
			boost::container::vector<Reference> arguments;
			arguments.reserve(std::max(function.default_arguments_opt.size(), params.number_of_arguments));
			// There could be more arguments than parameters.
			for(std::size_t i = 0; i < params.number_of_arguments; ++i){
				auto argument_ref = std::move(stack.back());
				stack.pop_back();
				if(argument_ref.get_type() == Reference::type_null){
					argument_ref = get_default_argument_opt(recycler, function, i);
				}
				arguments.emplace_back(std::move(argument_ref));
			}
			// Ensure there are no fewer arguments than parameters.
			for(std::size_t i = params.number_of_arguments; i < function.default_arguments_opt.size(); ++i){
				auto argument_ref = get_default_argument_opt(recycler, function, i);
				arguments.emplace_back(std::move(argument_ref));
			}
			// Call the function and push the result as-is.
			auto result_ref = function.payload(recycler, std::move(arguments));
			stack.emplace_back(std::move(result_ref));
			continue; }
		case Expression_node::type_operator_rpn:
		case Expression_node::type_lambda_definition:
			continue;

#if 0
	 else {
		// Note: Parameters are pushed from right to left.
		boost::container::vector<Reference> stack;
		for(std::size_t i = 0; i < expression_opt->get_size(); ++i){
			const auto type = expression_opt->get_type_at(i);
			switch(type){
			case Expression::type_literal_generic: {
			case Expression::type_named_reference: {
			case Expression::type_operator_reverse_polish: {
				const auto &params = expression_opt->get_at<Expression::Operator_reverse_polish>(i);
				switch(params.operator_generic){
				case Expression::operator_postfix_inc: {
					// Pop one operand from the stack.
					if(stack.size() < 1){
						ASTERIA_THROW_RUNTIME_ERROR("Insufficient operands for a postfix increment operator. This is probably a bug, please report.");
					}
					const auto reference_one = std::move(stack.back());
					stack.pop_back();
					const auto operand_one = read_reference_opt(reference_one);
					// Make a copy of it, which will be the value of this expression.
					Xptr<Variable> result;
					recycler->copy_variable_recursive(result, operand_one);
					// The operand must have type `D_boolean`, `D_integer` or `D_double`.
					const auto type_one = get_variable_type(operand_one);
					if(type_one == Variable::type_boolean){
						auto &value_one = operand_one->get<D_boolean>();
						value_one = true;
					} else if(type_one == Variable::type_integer){
						auto &value_one = operand_one->get<D_integer>();
						value_one = static_cast<D_integer>(static_cast<std::uint64_t>(value_one) + 1);
					} else if(type_one == Variable::type_double){
						auto &value_one = operand_one->get<D_double>();
						value_one = value_one + 1.0;
					} else {
						ASTERIA_THROW_RUNTIME_ERROR("The operand of a postfix increment operator has an inacceptable type `", operand_one, "`");
					}
					// Push the old value onto the stack. The result is an rvalue.
					Reference::S_rvalue_generic ref = { result.release() };
					stack.emplace_back(std::move(ref));
					break; }
				case Expression::operator_postfix_dec: {
					// Pop one operand from the stack.
					if(stack.size() < 1){
						ASTERIA_THROW_RUNTIME_ERROR("Insufficient operands for a postfix decrement operator. This is probably a bug, please report.");
					}
					const auto reference_one = std::move(stack.back());
					stack.pop_back();
					const auto operand_one = read_reference_opt(reference_one);
					// Make a copy of it, which will be the value of this expression.
					Xptr<Variable> result;
					recycler->copy_variable_recursive(result, operand_one);
					// The operand must have type `D_integer` or `D_double`.
					const auto type_one = get_variable_type(operand_one);
					if(type_one == Variable::type_integer){
						auto &value_one = operand_one->get<D_integer>();
						value_one = static_cast<D_integer>(static_cast<std::uint64_t>(value_one) - 1);
					} else if(type_one == Variable::type_double){
						auto &value_one = operand_one->get<D_double>();
						value_one = value_one - 1.0;
					} else {
						ASTERIA_THROW_RUNTIME_ERROR("The operand of a postfix decrement operator has an inacceptable type `", operand_one, "`");
					}
					// Push the old value onto the stack. The result is an rvalue.
					Reference::S_rvalue_generic ref = { result.release() };
					stack.emplace_back(std::move(ref));
					break; }
//				case Expression::operator_prefix_add: {
//				case Expression::operator_prefix_sub: {
//				case Expression::operator_prefix_not: {
//				case Expression::operator_prefix_not_l: {
//				case Expression::operator_prefix_inc: {
//				case Expression::operator_prefix_dec: {
//				case Expression::operator_infix_cmpeq: {
//				case Expression::operator_infix_cmpne: {
//				case Expression::operator_infix_cmplt: {
//				case Expression::operator_infix_cmpgt: {
//				case Expression::operator_infix_cmplte: {
//				case Expression::operator_infix_cmpgte: {
//				case Expression::operator_infix_add: {
//				case Expression::operator_infix_sub: {
//				case Expression::operator_infix_mul: {
//				case Expression::operator_infix_div: {
//				case Expression::operator_infix_mod: {
//				case Expression::operator_infix_sll: {
//				case Expression::operator_infix_srl: {
//				case Expression::operator_infix_sra: {
//				case Expression::operator_infix_and: {
//				case Expression::operator_infix_or: {
//				case Expression::operator_infix_xor: {
//				case Expression::operator_infix_and_l: {
//				case Expression::operator_infix_or_l: {
//				case Expression::operator_infix_assign: {
//				case Expression::operator_infix_add_a: {
//				case Expression::operator_infix_sub_a: {
//				case Expression::operator_infix_mul_a: {
//				case Expression::operator_infix_div_a: {
//				case Expression::operator_infix_mod_a: {
//				case Expression::operator_infix_sll_a: {
//				case Expression::operator_infix_srl_a: {
//				case Expression::operator_infix_sra_a: {
//				case Expression::operator_infix_and_a: {
//				case Expression::operator_infix_or_a: {
//				case Expression::operator_infix_xor_a: {
//				case Expression::operator_infix_and_l_a: {
//				case Expression::operator_infix_or_l_a: {
				default:
					ASTERIA_DEBUG_LOG("Unknown operator enumeration `", params.operator_generic, "`. This is probably a bug, please report.");
					std::terminate();
				}
				break; }
			case Expression::type_function_call: {
				const auto &params = expression_opt->get_at<Expression::S_function_call>(i);
				// Copy the literal to create a new variable.
				Xptr<Variable> var;
				recycler->copy_variable_recursive(var, params.source_opt);
				// Push it onto the stack. The result is an rvalue.
				Reference::S_rvalue_generic ref = { std::move(var) };
				stack.emplace_back(std::move(ref));
				

				ASTERIA_THROW_RUNTIME_ERROR("TODO");
				
//				const auto &params = expression_opt->get_at<Expression::S_function_call>(i);
//				if(stack.size() < 1){
//					ASTERIA_THROW_RUNTIME_ERROR("Could not find the left operand of a function call expression. This is probably a bug, please report.");
//				}
//				const auto &source = params.
				break; }
			case Expression::type_subscripting_generic: {
				ASTERIA_THROW_RUNTIME_ERROR("TODO");
//				const auto &params = expression_opt->get_at<Expression::Subscripting_generic>(i);
//				if(stack.size() < 1){
//					ASTERIA_THROW_RUNTIME_ERROR("Could not find the left operand of a subscripting expression. This is probably a bug, please report.");
//				}
//				const auto subscript = read_reference_opt(stack.back());
//				if(read_reference_opt
/*
				struct S_lvalue_array_element {
				    Sptr<Recycler> recycler;
				        Sptr<Variable> rvar;
				            bool immutable;
				                std::int64_t index_bidirectional;
				                };
				                struct S_lvalue_object_member {
				                    Sptr<Recycler> recycler;
				                        Sptr<Variable> rvar;
				                            bool immutable;
				                                std::string key;
				                                };

				                                
*/				break; }
			case Expression::type_lambda_definition: {
				ASTERIA_THROW_RUNTIME_ERROR("TODO");
				break; }
		}
	}
#endif
}
#endif
}
