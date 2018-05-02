// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "variable.hpp"
#include "stored_value.hpp"
#include "reference.hpp"
#include "stored_reference.hpp"
#include "recycler.hpp"
#include "scope.hpp"
#include "function_base.hpp"
#include "block.hpp"
#include "instantiated_function.hpp"
#include "utilities.hpp"

namespace Asteria {

Expression::Expression(Expression &&) noexcept = default;
Expression &Expression::operator=(Expression &&) = default;
Expression::~Expression() = default;

namespace {
	const char *op_name_of(const Expression_node::S_operator_rpn &params){
		return get_operator_name_generic(params.operator_generic);
	}
	const char *type_name_of(Variable::Type type){
		return get_type_name(type);
	}

	void do_push_reference(Xptr_vector<Reference> &stack, Xptr<Reference> &&ref){
		ASTERIA_DEBUG_LOG("Pushing: ", ref);
		stack.emplace_back(std::move(ref));
	}
	Xptr<Reference> do_pop_reference(Xptr_vector<Reference> &stack){
		if(stack.empty()){
			ASTERIA_THROW_RUNTIME_ERROR("The reference stack is empty (this is probably due to an unbalanced expression)");
		}
		auto ref = std::move(stack.back());
		ASTERIA_DEBUG_LOG("Popping: ", ref);
		stack.pop_back();
		return ref;
	}

	template<typename ResultT>
	void do_set_result(Xptr<Reference> &ref_inout_opt, Spcref<Recycler> recycler, bool compound_assignment, ResultT &&result){
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

	std::int64_t do_negate(std::int64_t rhs){
		if(rhs == INT64_MIN){
			ASTERIA_THROW_RUNTIME_ERROR("Integral negation would result in overflow: rhs = ", rhs);
		}
		return -rhs;
	}
	double do_negate(double rhs){
		return -rhs;
	}

	std::int64_t do_add(std::int64_t lhs, std::int64_t rhs){
		if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral addition would result in overflow: lhs = ", lhs, ", rhs = ", rhs);
		}
		return lhs + rhs;
	}
	double do_add(double lhs, double rhs){
		return lhs + rhs;
	}

	std::int64_t do_subtract(std::int64_t lhs, std::int64_t rhs){
		if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral subtraction would result in overflow: lhs = ", lhs, ", rhs = ", rhs);
		}
		return lhs - rhs;
	}
	double do_subtract(double lhs, double rhs){
		return lhs - rhs;
	}

	std::int64_t do_multiply(std::int64_t lhs, std::int64_t rhs){
		if((lhs == 0) || (rhs == 0)){
			return 0;
		}
		if((lhs == 1) || (rhs == 1)){
			return lhs ^ rhs ^ 1;
		}
		if((lhs == INT64_MIN) || (rhs == INT64_MIN)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication would result in overflow: lhs = ", lhs, ", rhs = ", rhs);
		}
		if((lhs == -1) || (rhs == -1)){
			return -(lhs ^ rhs ^ -1);
		}
		const auto alhs = std::abs(lhs);
		const auto arhs = std::abs(rhs);
		const auto srhs = ((lhs >= 0) == (rhs >= 0)) ? arhs : -arhs;
		if((srhs >= 0) ? (alhs > INT64_MAX / srhs) : (alhs > INT64_MIN / srhs)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication would result in overflow: lhs = ", lhs, ", rhs = ", rhs);
		}
		return alhs * srhs;
	}
	double do_multiply(double lhs, double rhs){
		return lhs * rhs;
	}

	std::int64_t do_divide(std::int64_t lhs, std::int64_t rhs){
		if(rhs == 0){
			ASTERIA_THROW_RUNTIME_ERROR("The divisor was zero: lhs = ", lhs, ", rhs = ", rhs);
		}
		if((lhs == INT64_MIN) && (rhs == -1)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral division would result in overflow: lhs = ", lhs, ", rhs = ", rhs);
		}
		return lhs / rhs;
	}
	double do_divide(double lhs, double rhs){
		return lhs / rhs;
	}

	std::int64_t do_modulo(std::int64_t lhs, std::int64_t rhs){
		if(rhs == 0){
			ASTERIA_THROW_RUNTIME_ERROR("The divisor was zero: lhs = ", lhs, ", rhs = ", rhs);
		}
		if((lhs == INT64_MIN) && (rhs == -1)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral division would result in overflow: lhs = ", lhs, ", rhs = ", rhs);
		}
		return lhs % rhs;
	}
	double do_modulo(double lhs, double rhs){
		return std::fmod(lhs, rhs);
	}

	std::int64_t do_shift_left_logical(std::int64_t lhs, std::int64_t rhs){
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("Bit shift count was negative: lhs = ", lhs, ", rhs = ", rhs);
		}
		if(rhs >= 64){
			return 0;
		}
		return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) << rhs);
	}
	std::int64_t do_shift_left_arithmetic(std::int64_t lhs, std::int64_t rhs){
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("Bit shift count was negative: lhs = ", lhs, ", rhs = ", rhs);
		}
		if(rhs >= 64){
			ASTERIA_THROW_RUNTIME_ERROR("Arithmetic bit shift count must be less than 64: lhs = ", lhs, ", rhs = ", rhs);
		}
		const auto mask = INT64_MIN >> rhs;
		if(((lhs & mask) != 0) && ((lhs & mask) != mask)){
			ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift would result in overflow: lhs = ", lhs, ", rhs = ", rhs);
		}
		return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) << rhs);
	}
	std::int64_t do_shift_right_logical(std::int64_t lhs, std::int64_t rhs){
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("Bit shift count was negative: lhs = ", lhs, ", rhs = ", rhs);
		}
		if(rhs >= 64){
			return 0;
		}
		return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) >> rhs);
	}
	std::int64_t do_shift_right_arithmetic(std::int64_t lhs, std::int64_t rhs){
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("Bit shift count was negative: lhs = ", lhs, ", rhs = ", rhs);
		}
		if(rhs >= 64){
			ASTERIA_THROW_RUNTIME_ERROR("Arithmetic bit shift count must be less than 64: lhs = ", lhs, ", rhs = ", rhs);
		}
		return lhs >> rhs;
	}

	bool do_bitwise_not(bool rhs){
		return !rhs;
	}
	std::int64_t do_bitwise_not(std::int64_t rhs){
		return ~rhs;
	}

	bool do_bitwise_and(bool lhs, bool rhs){
		return lhs & rhs;
	}
	std::int64_t do_bitwise_and(std::int64_t lhs, std::int64_t rhs){
		return lhs & rhs;
	}

	bool do_bitwise_or(bool lhs, bool rhs){
		return lhs | rhs;
	}
	std::int64_t do_bitwise_or(std::int64_t lhs, std::int64_t rhs){
		return lhs | rhs;
	}

	bool do_bitwise_xor(bool lhs, bool rhs){
		return lhs ^ rhs;
	}
	std::int64_t do_bitwise_xor(std::int64_t lhs, std::int64_t rhs){
		return lhs ^ rhs;
	}

	std::string do_concatenate(const std::string &lhs, const std::string &rhs){
		std::string result;
		result.reserve(lhs.size() + rhs.size());
		result.append(lhs);
		result.append(rhs);
		return result;
	}
	std::string do_duplicate(const std::string &lhs, std::int64_t rhs){
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("String duplication count was negative: lhs = ", lhs, ", rhs = ", rhs);
		}
		std::string result;
		if(rhs == 0){
			return result;
		}
		const auto count = static_cast<std::uint64_t>(rhs);
		if(lhs.size() > result.max_size() / count){
			ASTERIA_THROW_RUNTIME_ERROR("The result string would be too long and could not be allocated: lhs.length() = ", lhs.length(), ", rhs = ", rhs);
		}
		result.reserve(lhs.size() * static_cast<std::size_t>(count));
		for(auto mask = UINT64_C(1) << 63; mask != 0; mask >>= 1){
			result.append(result);
			if(count & mask){
				result.append(lhs);
			}
		}
		return result;
	}
}

void evaluate_expression(Xptr<Reference> &result_out, Spcref<Recycler> recycler, Spcref<const Expression> expression_opt, Spcref<const Scope> scope){
	if(expression_opt == nullptr){
		// Return a null reference only when a null expression is given.
		return move_reference(result_out, nullptr);
	}
	ASTERIA_DEBUG_LOG(">>>>>>> Beginning of evaluation of expression >>>>>>>");
	// Parameters are pushed from right to left, in lexical order.
	Xptr_vector<Reference> stack;
	// Evaluate nodes in reverse-polish order.
	for(const auto &node : *expression_opt){
		const auto type = node.get_type();
		switch(type){
		case Expression_node::type_literal: {
			const auto &params = node.get<Expression_node::S_literal>();
			// Create an immutable reference to the constant.
			Xptr<Reference> ref;
			Reference::S_constant ref_c = { params.source_opt };
			set_reference(ref, std::move(ref_c));
			do_push_reference(stack, std::move(ref));
			break; }
		case Expression_node::type_named_reference: {
			const auto &params = node.get<Expression_node::S_named_reference>();
			// Look up the reference in the enclosing scope.
			Xptr<Reference> ref;
			for(auto scope_cur = scope; scope_cur && (ref == nullptr); scope_cur = scope_cur->get_parent_opt()){
				const auto ref_found = scope_cur->get_local_reference_opt(params.identifier);
				copy_reference(ref, ref_found);
			}
			if(ref == nullptr){
				ASTERIA_THROW_RUNTIME_ERROR("Undeclared identifier `", params.identifier, "`");
			}
			// Push the reference onto the stack as-is.
			do_push_reference(stack, std::move(ref));
			break; }
		case Expression_node::type_bound_reference: {
			const auto &params = node.get<Expression_node::S_bound_reference>();
			// Look up the reference in the enclosing scope.
			Xptr<Reference> ref;
			copy_reference(ref, params.reference_opt);
			// Push the reference onto the stack as-is.
			do_push_reference(stack, std::move(ref));
			break; }
		case Expression_node::type_subexpression: {
			const auto &params = node.get<Expression_node::S_subexpression>();
			// Evaluate the subexpression recursively.
			Xptr<Reference> result_ref;
			evaluate_expression(result_ref, recycler, params.subexpression_opt, scope);
			// Push the result reference onto the stack as-is.
			do_push_reference(stack, std::move(result_ref));
			break; }
		case Expression_node::type_lambda_definition: {
			const auto &params = node.get<Expression_node::S_lambda_definition>();
			// Create a temporary variable for the function.
			Xptr<Variable> var;
			auto func = std::make_shared<Instantiated_function>(scope, params.parameters_opt, params.body_opt);
			set_variable(var, recycler, D_function(std::move(func)));
			Xptr<Reference> result_ref;
			Reference::S_temporary_value ref_d = { std::move(var) };
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
			// If the branch exists, evaluate it. Otherwise, grab the condition instead.
			Xptr<Reference> result_ref;
			if(branch_taken){
				evaluate_expression(result_ref, recycler, branch_taken, scope);
			} else {
				move_reference(result_ref, std::move(condition_ref));
			}
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
				ASTERIA_THROW_RUNTIME_ERROR("Attempting to call something having type `", type_name_of(callee_type), "`, which is not a function");
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
			// Call the function and push the result as-is.
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "`");
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "`");
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined subscript type `", type_name_of(rhs_type), "`");
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(rhs_type), "`");
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(rhs_type), "`");
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
					do_set_result(rhs_ref, recycler, params.compound_assignment, do_bitwise_not(rhs));
				} else if(rhs_type == Variable::type_integer){
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(rhs_ref, recycler, params.compound_assignment, do_bitwise_not(rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(rhs_type), "`");
				}
				do_push_reference(stack, std::move(rhs_ref));
				break; }
			case Expression_node::operator_prefix_not_l: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack);
				// Convert the operand to a `boolean` value, which is an rvalue, negate it, then return it.
				// N.B. This is one of the few operators that work on all types.
				const auto rhs_var = read_reference_opt(rhs_ref);
				do_set_result(rhs_ref, recycler, params.compound_assignment, do_bitwise_not(test_variable(rhs_var)));
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(rhs_type), "`");
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(rhs_type), "`");
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
					ASTERIA_THROW_RUNTIME_ERROR("Unordered operands for ", op_name_of(params));
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
					ASTERIA_THROW_RUNTIME_ERROR("Unordered operands for ", op_name_of(params));
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
					ASTERIA_THROW_RUNTIME_ERROR("Unordered operands for ", op_name_of(params));
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
					ASTERIA_THROW_RUNTIME_ERROR("Unordered operands for ", op_name_of(params));
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
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_bitwise_or(lhs, rhs));
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "` and `", type_name_of(rhs_type), "`");
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
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_bitwise_xor(lhs, rhs));
				} else if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_subtract(lhs, rhs));
				} else if((lhs_type == Variable::type_double) && (rhs_type == Variable::type_double)){
					const auto lhs = lhs_var->get<D_double>();
					const auto rhs = rhs_var->get<D_double>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_subtract(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "` and `", type_name_of(rhs_type), "`");
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
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_bitwise_and(lhs, rhs));
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "` and `", type_name_of(rhs_type), "`");
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "` and `", type_name_of(rhs_type), "`");
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "` and `", type_name_of(rhs_type), "`");
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "` and `", type_name_of(rhs_type), "`");
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "` and `", type_name_of(rhs_type), "`");
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "` and `", type_name_of(rhs_type), "`");
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
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "` and `", type_name_of(rhs_type), "`");
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
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_bitwise_and(lhs, rhs));
				} else if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_bitwise_and(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "` and `", type_name_of(rhs_type), "`");
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
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_bitwise_or(lhs, rhs));
				} else if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_bitwise_or(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "` and `", type_name_of(rhs_type), "`");
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
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_bitwise_xor(lhs, rhs));
				} else if((lhs_type == Variable::type_integer) && (rhs_type == Variable::type_integer)){
					const auto lhs = lhs_var->get<D_integer>();
					const auto rhs = rhs_var->get<D_integer>();
					do_set_result(lhs_ref, recycler, params.compound_assignment, do_bitwise_xor(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Undefined ", op_name_of(params), " operation on type `", type_name_of(lhs_type), "` and `", type_name_of(rhs_type), "`");
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
				ASTERIA_DEBUG_LOG("Unknown operator enumeration `", params.operator_generic, "`. This is probably a bug, please report.");
				std::terminate();
			}
			break; }
		default:
			ASTERIA_DEBUG_LOG("Unknown expression node type enumeration `", type, "`. This is probably a bug, please report.");
			std::terminate();
		}
	}
	// Get the result. If the stack is empty or has more than one elements, the expression is unbalanced.
	if(stack.size() != 1){
		ASTERIA_THROW_RUNTIME_ERROR("Unbalanced expression, number of references remaining is `", stack.size(), "`");
	}
	ASTERIA_DEBUG_LOG("<<<<<<< End of evaluation of expression <<<<<<<<<<<<");
	return move_reference(result_out, std::move(stack.front()));
}

}
