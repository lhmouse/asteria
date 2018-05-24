// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "stored_value.hpp"
#include "stored_reference.hpp"
#include "recycler.hpp"
#include "scope.hpp"
#include "block.hpp"
#include "instantiated_function.hpp"
#include "utilities.hpp"
#include <cmath>

namespace Asteria {

Expression::Expression(Expression &&) noexcept = default;
Expression & Expression::operator=(Expression &&) noexcept = default;
Expression::~Expression() = default;

void bind_expression(Xptr<Expression> &bound_result_out, Spparam<const Expression> expression_opt, Spparam<const Scope> scope){
	if(expression_opt == nullptr){
		// Return a null expression.
		return bound_result_out.reset();
	}
	// Bind nodes recursively.
	T_vector<Expression_node> bound_nodes;
	bound_nodes.reserve(expression_opt->size());
	for(std::size_t node_index = 0; node_index < expression_opt->size(); ++node_index){
		const auto &node = expression_opt->at(node_index);
		const auto type = node.get_type();
		switch(type){
		case Expression_node::type_literal: {
			const auto &params = node.get<Expression_node::S_literal>();
			// Copy it as is.
			bound_nodes.emplace_back(params);
			break; }

		case Expression_node::type_named_reference: {
			const auto &params = node.get<Expression_node::S_named_reference>();
			// Look up the reference in the enclosing scope.
			Sptr<const Reference> source_ref;
			auto scope_cur = scope;
			for(;;){
				if(!scope_cur){
					ASTERIA_THROW_RUNTIME_ERROR("The identifier `", params.identifier, "` has not been declared yet.");
				}
				source_ref = scope_cur->get_local_reference_opt(params.identifier);
				if(source_ref){
					break;
				}
				scope_cur = scope_cur->get_parent_opt();
			}
			// If the reference is in a lexical scope rather than a run-time scope, don't bind it.
			if(scope_cur->get_purpose() == Scope::purpose_lexical){
				bound_nodes.emplace_back(params);
				break;
			}
			// Bind the reference.
			Xptr<Reference> bound_ref;
			copy_reference(bound_ref, source_ref);
			Expression_node::S_bound_reference node_b = { std::move(bound_ref) };
			bound_nodes.emplace_back(std::move(node_b));
			break; }

		case Expression_node::type_bound_reference: {
			const auto &params = node.get<Expression_node::S_bound_reference>();
			// Copy the reference bound.
			Xptr<Reference> bound_ref;
			copy_reference(bound_ref, params.reference_opt);
			Expression_node::S_bound_reference node_b = { std::move(bound_ref) };
			bound_nodes.emplace_back(std::move(node_b));
			break; }

		case Expression_node::type_subexpression: {
			const auto &params = node.get<Expression_node::S_subexpression>();
			// Bind the subexpression recursively.
			Xptr<Expression> bound_expr;
			bind_expression(bound_expr, params.subexpression_opt, scope);
			Expression_node::S_subexpression node_s = { std::move(bound_expr) };
			bound_nodes.emplace_back(std::move(node_s));
			break; }

		case Expression_node::type_lambda_definition: {
			const auto &params = node.get<Expression_node::S_lambda_definition>();
			// Bind the function body onto the current scope.
			const auto scope_lexical = std::make_shared<Scope>(Scope::purpose_lexical, scope);
			prepare_function_scope_lexical(scope_lexical, params.source_location, params.parameters_opt);
			Xptr<Block> bound_body;
			bind_block_in_place(bound_body, scope_lexical, params.body_opt);
			Expression_node::S_lambda_definition node_l = { params.source_location, params.parameters_opt, std::move(bound_body) };
			bound_nodes.emplace_back(std::move(node_l));
			break; }

		case Expression_node::type_pruning: {
			const auto &params = node.get<Expression_node::S_pruning>();
			bound_nodes.emplace_back(params);
			break; }

		case Expression_node::type_branch: {
			const auto &params = node.get<Expression_node::S_branch>();
			// Bind both branches recursively.
			Xptr<Expression> bound_branch_true;
			bind_expression(bound_branch_true, params.branch_true_opt, scope);
			Xptr<Expression> bound_branch_false;
			bind_expression(bound_branch_false, params.branch_false_opt, scope);
			Expression_node::S_branch node_b = { std::move(bound_branch_true), std::move(bound_branch_false) };
			bound_nodes.emplace_back(std::move(node_b));
			break; }

		case Expression_node::type_function_call: {
			const auto &params = node.get<Expression_node::S_function_call>();
			bound_nodes.emplace_back(params);
			break; }

		case Expression_node::type_operator_rpn: {
			const auto &params = node.get<Expression_node::S_operator_rpn>();
			bound_nodes.emplace_back(params);
			break; }

		default:
			ASTERIA_DEBUG_LOG("Unknown expression node type enumeration `", type, "` at index `", node_index, "`. This is probably a bug, please report.");
			std::terminate();
		}
	}
	return bound_result_out.emplace(std::move(bound_nodes));
}

namespace {
	const char * op_name_of(const Expression_node::S_operator_rpn &params){
		return get_operator_name_generic(params.operator_generic);
	}
	const char * type_name_of(Variable::Type type){
		return get_type_name(type);
	}

	void do_push_reference(Xptr_vector<Reference> &stack, Xptr<Reference> &&ref){
		ASTERIA_DEBUG_LOG("Pushing: ", sptr_fmt(ref));
		stack.emplace_back(std::move(ref));
	}
	Xptr<Reference> do_pop_reference(Xptr_vector<Reference> &stack){
		if(stack.empty()){
			ASTERIA_THROW_RUNTIME_ERROR("The evaluation stack was empty.");
		}
		auto ref = std::move(stack.back());
		ASTERIA_DEBUG_LOG("Popping: ", sptr_fmt(ref));
		stack.pop_back();
		return ref;
	}

	template<typename ResultT>
	void do_set_result(Xptr<Reference> &ref_inout_opt, Spparam<Recycler> recycler, bool compound_assignment, ResultT &&result){
		if(compound_assignment){
			// Update the result in-place.
			const auto wref = drill_reference(ref_inout_opt);
			return set_variable(wref, recycler, std::forward<ResultT>(result));
		} else {
			// Create a new variable for the result, then replace `lhs_ref` with an rvalue reference to it.
			Xptr<Variable> var;
			set_variable(var, recycler, std::forward<ResultT>(result));
			Reference::S_temporary_value ref_d = { std::move(var) };
			return set_reference(ref_inout_opt, std::move(ref_d));
		}
	}

	D_boolean do_logical_not(D_boolean rhs){
		return !rhs;
	}
	D_boolean do_logical_and(D_boolean lhs, D_boolean rhs){
		return lhs & rhs;
	}
	D_boolean do_logical_or(D_boolean lhs, D_boolean rhs){
		return lhs | rhs;
	}
	D_boolean do_logical_xor(D_boolean lhs, D_boolean rhs){
		return lhs ^ rhs;
	}

	D_integer do_negate(D_integer rhs){
		if(rhs == INT64_MIN){
			ASTERIA_THROW_RUNTIME_ERROR("Integral negation of `", rhs, "` would result in overflow.");
		}
		return -rhs;
	}
	D_integer do_add(D_integer lhs, D_integer rhs){
		if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral addition of `", lhs, "` and `", rhs, "` would result in overflow.");
		}
		return lhs + rhs;
	}
	D_integer do_subtract(D_integer lhs, D_integer rhs){
		if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral subtraction of `", lhs, "` and `", rhs, "` would result in overflow.");
		}
		return lhs - rhs;
	}
	D_integer do_multiply(D_integer lhs, D_integer rhs){
		if((lhs == 0) || (rhs == 0)){
			return 0;
		}
		if((lhs == 1) || (rhs == 1)){
			return lhs ^ rhs ^ 1;
		}
		if((lhs == INT64_MIN) || (rhs == INT64_MIN)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication of `", lhs, "` and `", rhs, "` would result in overflow.");
		}
		if((lhs == -1) || (rhs == -1)){
			return -(lhs ^ rhs ^ -1);
		}
		const auto alhs = std::abs(lhs);
		const auto arhs = std::abs(rhs);
		const auto srhs = ((lhs >= 0) == (rhs >= 0)) ? arhs : -arhs;
		if((srhs >= 0) ? (alhs > INT64_MAX / srhs) : (alhs > INT64_MIN / srhs)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication of `", lhs, "` and `", rhs, "` would result in overflow.");
		}
		return alhs * srhs;
	}
	D_integer do_divide(D_integer lhs, D_integer rhs){
		if(rhs == 0){
			ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
		}
		if((lhs == INT64_MIN) && (rhs == -1)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral division of `", lhs, "` and `", rhs, "` would result in overflow.");
		}
		return lhs / rhs;
	}
	D_integer do_modulo(D_integer lhs, D_integer rhs){
		if(rhs == 0){
			ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
		}
		if((lhs == INT64_MIN) && (rhs == -1)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral division of `", lhs, "` and `", rhs, "` would result in overflow.");
		}
		return lhs % rhs;
	}

	D_integer do_shift_left_logical(D_integer lhs, D_integer rhs){
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` was negative.");
		}
		if(rhs >= 64){
			return 0;
		}
		return static_cast<D_integer>(static_cast<std::uint64_t>(lhs) << rhs);
	}
	D_integer do_shift_left_arithmetic(D_integer lhs, D_integer rhs){
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` was negative.");
		}
		if(rhs >= 64){
			ASTERIA_THROW_RUNTIME_ERROR("Arithmetic bit shift count `", rhs, "` for `", lhs, "` was larger than the width of an `integer`.");
		}
		const auto mask = INT64_MIN >> rhs;
		if(((lhs & mask) != 0) && ((lhs & mask) != mask)){
			ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
		}
		return static_cast<D_integer>(static_cast<std::uint64_t>(lhs) << rhs);
	}
	D_integer do_shift_right_logical(D_integer lhs, D_integer rhs){
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` was negative.");
		}
		if(rhs >= 64){
			return 0;
		}
		return static_cast<D_integer>(static_cast<std::uint64_t>(lhs) >> rhs);
	}
	D_integer do_shift_right_arithmetic(D_integer lhs, D_integer rhs){
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` was negative.");
		}
		if(rhs >= 64){
			ASTERIA_THROW_RUNTIME_ERROR("Arithmetic bit shift count `", rhs, "` for `", lhs, "` was larger than the width of an `integer`.");
		}
		return lhs >> rhs;
	}

	D_integer do_bitwise_not(D_integer rhs){
		return ~rhs;
	}
	D_integer do_bitwise_and(D_integer lhs, D_integer rhs){
		return lhs & rhs;
	}
	D_integer do_bitwise_or(D_integer lhs, D_integer rhs){
		return lhs | rhs;
	}
	D_integer do_bitwise_xor(D_integer lhs, D_integer rhs){
		return lhs ^ rhs;
	}

	D_double do_negate(D_double rhs){
		return -rhs;
	}
	D_double do_add(D_double lhs, D_double rhs){
		return lhs + rhs;
	}
	D_double do_subtract(D_double lhs, D_double rhs){
		return lhs - rhs;
	}
	D_double do_multiply(D_double lhs, D_double rhs){
		return lhs * rhs;
	}
	D_double do_divide(D_double lhs, D_double rhs){
		return lhs / rhs;
	}
	D_double do_modulo(D_double lhs, D_double rhs){
		return std::fmod(lhs, rhs);
	}

	D_string do_concatenate(const D_string &lhs, const D_string &rhs){
		D_string res;
		res.reserve(lhs.size() + rhs.size());
		res.append(lhs);
		res.append(rhs);
		return res;
	}
	D_string do_duplicate(const D_string &lhs, D_integer rhs){
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("String duplication count `", rhs, "` for `", lhs, "` was negative.");
		}
		D_string res;
		if(rhs == 0){
			return res;
		}
		const auto count = static_cast<std::uint64_t>(rhs);
		if(lhs.size() > res.max_size() / count){
			ASTERIA_THROW_RUNTIME_ERROR("Duplication of `", lhs, "` up to `", rhs, "` time(s) would result in an overlong string that cannot be allocated.");
		}
		res.reserve(lhs.size() * static_cast<std::size_t>(count));
		auto mask = static_cast<std::uint64_t>(1) << 63;
		for(;;){
			if(count & mask){
				res.append(lhs);
			}
			mask >>= 1;
			if(mask == 0){
				break;
			}
			res.append(res);
		}
		return res;
	}
}

void evaluate_expression(Xptr<Reference> &result_out, Spparam<Recycler> recycler, Spparam<const Expression> expression_opt, Spparam<const Scope> scope){
	if(expression_opt == nullptr){
		// Return a null reference only when a null expression is given.
		return move_reference(result_out, nullptr);
	}
	ASTERIA_DEBUG_LOG("------ Beginning of evaluation of expression");
	// Parameters are pushed from right to left, in lexical order.
	Xptr_vector<Reference> stack;
	// Evaluate nodes in reverse-polish order.
	for(std::size_t node_index = 0; node_index < expression_opt->size(); ++node_index){
		const auto &node = expression_opt->at(node_index);
		const auto type = node.get_type();
		switch(type){
		case Expression_node::type_literal: {
			const auto &params = node.get<Expression_node::S_literal>();
			// Create an constant reference to the constant.
			Xptr<Reference> result_ref;
			Reference::S_constant ref_k = { params.source_opt };
			set_reference(result_ref, std::move(ref_k));
			do_push_reference(stack, std::move(result_ref));
			break; }

		case Expression_node::type_named_reference: {
			const auto &params = node.get<Expression_node::S_named_reference>();
			// Look up the reference in the enclosing scope.
			Sptr<const Reference> source_ref;
			auto scope_cur = scope;
			for(;;){
				if(!scope_cur){
					ASTERIA_THROW_RUNTIME_ERROR("The identifier `", params.identifier, "` has not been declared yet.");
				}
				source_ref = scope_cur->get_local_reference_opt(params.identifier);
				if(source_ref){
					break;
				}
				scope_cur = scope_cur->get_parent_opt();
			}
			Xptr<Reference> result_ref;
			copy_reference(result_ref, source_ref);
			// Push the reference onto the stack as is.
			do_push_reference(stack, std::move(result_ref));
			break; }

		case Expression_node::type_bound_reference: {
			const auto &params = node.get<Expression_node::S_bound_reference>();
			// Copy the reference bound.
			Xptr<Reference> bound_ref;
			copy_reference(bound_ref, params.reference_opt);
			// Push the reference onto the stack as is.
			do_push_reference(stack, std::move(bound_ref));
			break; }

		case Expression_node::type_subexpression: {
			const auto &params = node.get<Expression_node::S_subexpression>();
			// Evaluate the subexpression recursively.
			Xptr<Reference> result_ref;
			evaluate_expression(result_ref, recycler, params.subexpression_opt, scope);
			// Push the result reference onto the stack as is.
			do_push_reference(stack, std::move(result_ref));
			break; }

		case Expression_node::type_lambda_definition: {
			const auto &params = node.get<Expression_node::S_lambda_definition>();
			// Bind the function body onto the current scope.
			const auto scope_lexical = std::make_shared<Scope>(Scope::purpose_lexical, scope);
			prepare_function_scope_lexical(scope_lexical, params.source_location, params.parameters_opt);
			Xptr<Block> bound_body;
			bind_block_in_place(bound_body, scope_lexical, params.body_opt);
			// Create a temporary variable for the function.
			Xptr<Variable> func_var;
			auto description = ASTERIA_FORMAT_STRING("lambda defined at \'", params.source_location, "\'");
			set_variable(func_var, recycler, D_function(std::make_shared<Instantiated_function>(std::move(description), params.parameters_opt, scope, std::move(bound_body))));
			Xptr<Reference> result_ref;
			Reference::S_temporary_value ref_d = { std::move(func_var) };
			set_reference(result_ref, std::move(ref_d));
			// Push the result onto the stack.
			do_push_reference(stack, std::move(result_ref));
			break; }

		case Expression_node::type_pruning: {
			const auto &params = node.get<Expression_node::S_pruning>();
			// Pop references requested.
			for(std::size_t i = 0; i < params.count_to_pop; ++i){
				do_pop_reference(stack);
			}
			break; }

		case Expression_node::type_branch: {
			const auto &params = node.get<Expression_node::S_branch>();
			// Pop the condition off the stack.
			auto condition_ref = do_pop_reference(stack);
			// Pick a branch basing on the condition.
			const auto condition_var = read_reference_opt(condition_ref);
			const auto branch_taken = test_variable(condition_var) ? params.branch_true_opt.share() : params.branch_false_opt.share();
			if(!branch_taken){
				// If the branch does not exist, push the condition instead.
				do_push_reference(stack, std::move(condition_ref));
				break;
			}
			// Evaluate the branch and push the result.
			Xptr<Reference> result_ref;
			evaluate_expression(result_ref, recycler, branch_taken, scope);
			do_push_reference(stack, std::move(result_ref));
			break; }

		case Expression_node::type_function_call: {
			const auto &params = node.get<Expression_node::S_function_call>();
			// Pop the function off the stack.
			auto callee_ref = do_pop_reference(stack);
			const auto callee_var = read_reference_opt(callee_ref);
			// Make sure it is really a function.
			const auto callee_type = get_variable_type(callee_var);
			if(callee_type != Variable::type_function){
				ASTERIA_THROW_RUNTIME_ERROR("Only functions can be called, while the operand has type `", type_name_of(callee_type), "`.");
			}
			const auto &callee = callee_var->get<D_function>();
			// Allocate the argument vector. There will be no fewer arguments than parameters.
			Xptr_vector<Reference> arguments;
			arguments.reserve(32);
			// Pop arguments off the stack.
			for(std::size_t i = 0; i < params.argument_count; ++i){
				auto arg_ref = do_pop_reference(stack);
				arguments.emplace_back(std::move(arg_ref));
			}
			// Get the `this` reference.
			Xptr<Reference> this_ref;
			const auto callee_ref_type = get_reference_type(callee_ref);
			if(callee_ref_type == Reference::type_array_element){
				auto &callee_params = callee_ref->get<Reference::S_array_element>();
				this_ref = std::move(callee_params.parent_opt);
			} else if(callee_ref_type == Reference::type_object_member){
				auto &callee_params = callee_ref->get<Reference::S_object_member>();
				this_ref = std::move(callee_params.parent_opt);
			}
			// Call the function and push the result as is.
			callee->invoke(callee_ref, recycler, std::move(this_ref), std::move(arguments));
			do_push_reference(stack, std::move(callee_ref));
			break; }

		case Expression_node::type_operator_rpn: {
			const auto &params = node.get<Expression_node::S_operator_rpn>();
			switch(params.operator_generic){
			case Expression_node::operator_postfix_inc: {
				// Pop the operand off the stack.
				auto lhs_ref = do_pop_reference(stack);
				// Increment the operand and return the old value.
				// `compound_assignment` is ignored.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				if(lhs_type == Variable::type_integer){
					const auto lhs = lhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, true, do_add(lhs, D_integer(1)));
					do_set_result(lhs_ref, recycler, false, lhs);
				} else if(lhs_type == Variable::type_double){
					const auto lhs = lhs_var->get<D_double>();
					do_set_result(lhs_ref, recycler, true, do_add(lhs, D_double(1)));
					do_set_result(lhs_ref, recycler, false, lhs);
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_postfix_dec: {
				// Pop the operand off the stack.
				auto lhs_ref = do_pop_reference(stack);
				// Decrement the operand and return the old value.
				// `compound_assignment` is ignored.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				if(lhs_type == Variable::type_integer){
					const auto lhs = lhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, true, do_subtract(lhs, D_integer(1)));
					do_set_result(lhs_ref, recycler, false, lhs);
				} else if(lhs_type == Variable::type_double){
					const auto lhs = lhs_var->get<D_double>();
					do_set_result(lhs_ref, recycler, true, do_subtract(lhs, D_double(1)));
					do_set_result(lhs_ref, recycler, false, lhs);
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_postfix_at: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// The subscript operand shall have type `integer` or `string`. In neither case will `rhs_ref` be null, hence it can be safely reused.
				// `compound_assignment` is ignored.
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto rhs_type = get_variable_type(rhs_var);
				if(rhs_type == Variable::type_integer){
					Reference::S_array_element ref_a = { std::move(lhs_ref), rhs_var->get<D_integer>() };
					rhs_ref->set(std::move(ref_a));
				} else if(rhs_type == Variable::type_string){
					Reference::S_object_member ref_o = { std::move(lhs_ref), rhs_var->get<D_string>() };
					rhs_ref->set(std::move(ref_o));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Values having type `", type_name_of(rhs_type), "` cannot be used as subscripts.");
				}
				do_push_reference(stack, std::move(rhs_ref));
				break; }

			case Expression_node::operator_prefix_pos: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack);
				// Copy the referenced variable to create an rvalue, then return it.
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto rhs_type = get_variable_type(rhs_var);
				if(rhs_type == Variable::type_integer){
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(rhs_ref, recycler, params.compound_assignment, rhs);
				} else if(rhs_type == Variable::type_double){
					const auto rhs = rhs_var->get<D_double>();
					do_set_result(rhs_ref, recycler, params.compound_assignment, rhs);
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(rhs_ref));
				break; }

			case Expression_node::operator_prefix_neg: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack);
				// Negate the operand.
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto rhs_type = get_variable_type(rhs_var);
				if(rhs_type == Variable::type_integer){
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(rhs_ref, recycler, params.compound_assignment, do_negate(rhs));
				} else if(rhs_type == Variable::type_double){
					const auto rhs = rhs_var->get<D_double>();
					do_set_result(rhs_ref, recycler, params.compound_assignment, do_negate(rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(rhs_ref));
				break; }

			case Expression_node::operator_prefix_not_b: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack);
				// Bitwise NOT the operand.
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto rhs_type = get_variable_type(rhs_var);
				if(rhs_type == Variable::type_boolean){
					const auto rhs = rhs_var->get<D_boolean>();
					do_set_result(rhs_ref, recycler, params.compound_assignment, do_logical_not(rhs));
				} else if(rhs_type == Variable::type_integer){
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(rhs_ref, recycler, params.compound_assignment, do_bitwise_not(rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(rhs_ref));
				break; }

			case Expression_node::operator_prefix_not_l: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack);
				// Convert the operand to a `boolean` value, which is an rvalue, negate it, then return it.
				// N.B. This is one of the few operators that work on all types.
				const auto rhs_var = read_reference_opt(rhs_ref);
				do_set_result(rhs_ref, recycler, params.compound_assignment, do_logical_not(test_variable(rhs_var)));
				do_push_reference(stack, std::move(rhs_ref));
				break; }

			case Expression_node::operator_prefix_inc: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack);
				// Increment the operand and return it.
				// `compound_assignment` is ignored.
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto rhs_type = get_variable_type(rhs_var);
				if(rhs_type == Variable::type_integer){
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(rhs_ref, recycler, true, do_add(rhs, D_integer(1)));
				} else if(rhs_type == Variable::type_double){
					const auto rhs = rhs_var->get<D_double>();
					do_set_result(rhs_ref, recycler, true, do_add(rhs, D_double(1)));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(rhs_ref));
				break; }

			case Expression_node::operator_prefix_dec: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack);
				// Decrement the operand and return it.
				// `compound_assignment` is ignored.
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto rhs_type = get_variable_type(rhs_var);
				if(rhs_type == Variable::type_integer){
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(rhs_ref, recycler, true, do_subtract(rhs, D_integer(1)));
				} else if(rhs_type == Variable::type_double){
					const auto rhs = rhs_var->get<D_double>();
					do_set_result(rhs_ref, recycler, true, do_subtract(rhs, D_double(1)));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(rhs_ref));
				break; }

			case Expression_node::operator_infix_cmp_eq: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Report unordered operands as being unequal.
				// N.B. This is one of the few operators that work on all types.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto comparison_result = compare_variables(lhs_var, rhs_var);
				// Try reusing source operands.
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				do_set_result(lhs_ref, recycler, false, comparison_result == Variable::comparison_result_equal);
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_cmp_ne: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Report unordered operands as being unequal.
				// N.B. This is one of the few operators that work on all types.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto comparison_result = compare_variables(lhs_var, rhs_var);
				// Try reusing source operands.
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				do_set_result(lhs_ref, recycler, false, comparison_result != Variable::comparison_result_equal);
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_cmp_lt: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Throw an exception in case of unordered operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto comparison_result = compare_variables(lhs_var, rhs_var);
				if(comparison_result == Variable::comparison_result_unordered){
					ASTERIA_THROW_RUNTIME_ERROR("The operands `", sptr_fmt(lhs_var), "` and `", sptr_fmt(rhs_var), "` are unordered.");
				}
				// Try reusing source operands.
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				do_set_result(lhs_ref, recycler, false, comparison_result == Variable::comparison_result_less);
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_cmp_gt: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Throw an exception in case of unordered operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto comparison_result = compare_variables(lhs_var, rhs_var);
				if(comparison_result == Variable::comparison_result_unordered){
					ASTERIA_THROW_RUNTIME_ERROR("The operands `", sptr_fmt(lhs_var), "` and `", sptr_fmt(rhs_var), "` are unordered.");
				}
				// Try reusing source operands.
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				do_set_result(lhs_ref, recycler, false, comparison_result == Variable::comparison_result_greater);
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_cmp_lte: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Throw an exception in case of unordered operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto comparison_result = compare_variables(lhs_var, rhs_var);
				if(comparison_result == Variable::comparison_result_unordered){
					ASTERIA_THROW_RUNTIME_ERROR("The operands `", sptr_fmt(lhs_var), "` and `", sptr_fmt(rhs_var), "` are unordered.");
				}
				// Try reusing source operands.
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				do_set_result(lhs_ref, recycler, false, comparison_result != Variable::comparison_result_greater);
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_cmp_gte: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Throw an exception in case of unordered operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto comparison_result = compare_variables(lhs_var, rhs_var);
				if(comparison_result == Variable::comparison_result_unordered){
					ASTERIA_THROW_RUNTIME_ERROR("The operands `", sptr_fmt(lhs_var), "` and `", sptr_fmt(rhs_var), "` are unordered.");
				}
				// Try reusing source operands.
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				do_set_result(lhs_ref, recycler, false, comparison_result != Variable::comparison_result_less);
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_add: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// For the boolean type, return the logical OR'd result of both operands.
				// For the integer and double types, return the sum of both operands.
				// For the string type, concatenate the operands in lexical order to create a new string, then return it.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				const auto rhs_type = get_variable_type(rhs_var);
				if((lhs_type == Variable::type_boolean) && (rhs_type == Variable::type_boolean)){
					const auto lhs = lhs_var->get<D_boolean>();
					const auto rhs = rhs_var->get<D_boolean>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_logical_or(lhs, rhs));
				} else if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_add(lhs, rhs));
				} else if((lhs_type == Variable::type_double) && (rhs_type == Variable::type_double)){
					const auto lhs = lhs_var->get<D_double>();
					const auto rhs = rhs_var->get<D_double>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_add(lhs, rhs));
				} else if((lhs_type == Variable::type_string) && (rhs_type == Variable::type_string)){
					const auto lhs = lhs_var->get<D_string>();
					const auto rhs = rhs_var->get<D_string>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_concatenate(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` and type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_sub: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// For the boolean type, return the logical XOR'd result of both operands.
				// For the integer and double types, return the difference of both operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				const auto rhs_type = get_variable_type(rhs_var);
				if((lhs_type == Variable::type_boolean) && (rhs_type == Variable::type_boolean)){
					const auto lhs = lhs_var->get<D_boolean>();
					const auto rhs = rhs_var->get<D_boolean>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_logical_xor(lhs, rhs));
				} else if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_subtract(lhs, rhs));
				} else if((lhs_type == Variable::type_double) && (rhs_type == Variable::type_double)){
					const auto lhs = lhs_var->get<D_double>();
					const auto rhs = rhs_var->get<D_double>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_subtract(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` and type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_mul: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// For the boolean type, return the logical AND'd result of both operands.
				// For the integer and double types, return the product of both operands.
				// If either operand has the integer type and the other has the string type, duplicate the string up to the specified number of times.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				const auto rhs_type = get_variable_type(rhs_var);
				if((lhs_type == Variable::type_boolean) && (rhs_type == Variable::type_boolean)){
					const auto lhs = lhs_var->get<D_boolean>();
					const auto rhs = rhs_var->get<D_boolean>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_logical_and(lhs, rhs));
				} else if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_multiply(lhs, rhs));
				} else if((lhs_type == Variable::type_double) && (rhs_type == Variable::type_double)){
					const auto lhs = lhs_var->get<D_double>();
					const auto rhs = rhs_var->get<D_double>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_multiply(lhs, rhs));
				} else if((lhs_type == Variable::type_string) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_string>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_duplicate(lhs, rhs));
				} else if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_string)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_string>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_duplicate(rhs, lhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` and type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_div: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Divide the first operand by the second operand and return the quotient.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				const auto rhs_type = get_variable_type(rhs_var);
				if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_divide(lhs, rhs));
				} else if((lhs_type == Variable::type_double) && (rhs_type == Variable::type_double)){
					const auto lhs = lhs_var->get<D_double>();
					const auto rhs = rhs_var->get<D_double>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_divide(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` and type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_mod: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Divide the first operand by the second operand and return the remainder.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				const auto rhs_type = get_variable_type(rhs_var);
				if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_modulo(lhs, rhs));
				} else if((lhs_type == Variable::type_double) && (rhs_type == Variable::type_double)){
					const auto lhs = lhs_var->get<D_double>();
					const auto rhs = rhs_var->get<D_double>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_modulo(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` and type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_sll: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Shift the first operand to the left by the number of bits specified by the second operand
				// Bits shifted out are discarded. Bits shifted in are filled with zeroes.
				// Both operands have to be integers.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				const auto rhs_type = get_variable_type(rhs_var);
				if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_shift_left_logical(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` and type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_sla: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Shift the first operand to the left by the number of bits specified by the second operand
				// Bits shifted out that equal the sign bit are dicarded. Bits shifted in are filled with zeroes.
				// If a bit unequal to the sign bit would be shifted into or across the sign bit, an exception is thrown.
				// Both operands have to be integers.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				const auto rhs_type = get_variable_type(rhs_var);
				if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_shift_left_arithmetic(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` and type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_srl: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Shift the first operand to the right by the number of bits specified by the second operand
				// Bits shifted out are discarded. Bits shifted in are filled with zeroes.
				// Both operands have to be integers.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				const auto rhs_type = get_variable_type(rhs_var);
				if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_shift_right_logical(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` and type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_sra: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Shift the first operand to the right by the number of bits specified by the second operand
				// Bits shifted out are discarded. Bits shifted in are filled with the sign bit.
				// Both operands have to be integers.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				const auto rhs_type = get_variable_type(rhs_var);
				if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_shift_right_arithmetic(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` and type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_and_b: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Perform bitwise and on both operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				const auto rhs_type = get_variable_type(rhs_var);
				if((lhs_type == Variable::type_boolean) && (rhs_type == Variable::type_boolean)){
					const auto lhs = lhs_var->get<D_boolean>();
					const auto rhs = rhs_var->get<D_boolean>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_logical_and(lhs, rhs));
				} else if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_bitwise_and(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` and type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_or_b: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Perform bitwise or on both operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				const auto rhs_type = get_variable_type(rhs_var);
				if((lhs_type == Variable::type_boolean) && (rhs_type == Variable::type_boolean)){
					const auto lhs = lhs_var->get<D_boolean>();
					const auto rhs = rhs_var->get<D_boolean>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_logical_or(lhs, rhs));
				} else if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_bitwise_or(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` and type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_xor_b: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Perform bitwise xor on both operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_variable_type(lhs_var);
				const auto rhs_type = get_variable_type(rhs_var);
				if((lhs_type == Variable::type_boolean) && (rhs_type == Variable::type_boolean)){
					const auto lhs = lhs_var->get<D_boolean>();
					const auto rhs = rhs_var->get<D_boolean>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_logical_xor(lhs, rhs));
				} else if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_bitwise_xor(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", op_name_of(params), "` on type `", type_name_of(lhs_type), "` and type `", type_name_of(rhs_type), "` is undefined.");
				}
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_assign: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack);
				auto rhs_ref = do_pop_reference(stack);
				// Copy the variable referenced by `rhs_ref` into `lhs_ref`, then return it.
				// `compound_assignment` is ignored.
				// N.B. This is one of the few operators that work on all types.
				Xptr<Variable> var;
				extract_variable_from_reference(var, recycler, std::move(rhs_ref));
				const auto wref = drill_reference(lhs_ref);
				move_variable(wref, recycler, std::move(var));
				do_push_reference(stack, std::move(lhs_ref));
				break; }

			default:
				ASTERIA_DEBUG_LOG("Unknown operator enumeration `", params.operator_generic, "` at index `", node_index, "`. This is probably a bug, please report.");
				std::terminate();
			}
			break; }

		default:
			ASTERIA_DEBUG_LOG("Unknown expression node type enumeration `", type, "` at index `", node_index, "`. This is probably a bug, please report.");
			std::terminate();
		}
	}
	// Get the result. If the stack is empty or has more than one elements, the expression is unbalanced.
	if(stack.size() != 1){
		ASTERIA_THROW_RUNTIME_ERROR("There were `", stack.size(), "` elements in the evaluation stack due to imbalance of the expression.");
	}
	ASTERIA_DEBUG_LOG("------- End of evaluation of expression");
	return move_reference(result_out, std::move(stack.front()));
}

}
