// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression_node.hpp"
#include "value.hpp"
#include "stored_reference.hpp"
#include "scope.hpp"
#include "statement.hpp"
#include "instantiated_function.hpp"
#include "utilities.hpp"
#include <limits>
#include <cmath>

namespace Asteria {

Expression_node::Expression_node(Expression_node &&) noexcept = default;
Expression_node & Expression_node::operator=(Expression_node &&) noexcept = default;
Expression_node::~Expression_node() = default;

const char * get_operator_name(Expression_node::Operator op) noexcept {
	switch(op){
	case Expression_node::operator_postfix_inc:
		return "postfix increment";
	case Expression_node::operator_postfix_dec:
		return "postfix decrement";
	case Expression_node::operator_postfix_at:
		return "subscripting";
	case Expression_node::operator_prefix_pos:
		return "unary plus";
	case Expression_node::operator_prefix_neg:
		return "negation";
	case Expression_node::operator_prefix_notb:
		return "bitwise not";
	case Expression_node::operator_prefix_notl:
		return "logical not";
	case Expression_node::operator_prefix_inc:
		return "prefix increment";
	case Expression_node::operator_prefix_dec:
		return "prefix decrement";
	case Expression_node::operator_infix_cmp_eq:
		return "equality comparison";
	case Expression_node::operator_infix_cmp_ne:
		return "inequality comparison";
	case Expression_node::operator_infix_cmp_lt:
		return "less-than comparison";
	case Expression_node::operator_infix_cmp_gt:
		return "greater-than comparison";
	case Expression_node::operator_infix_cmp_lte:
		return "less-than-or-equal comparison";
	case Expression_node::operator_infix_cmp_gte:
		return "greater-than-or-equal comparison";
	case Expression_node::operator_infix_add:
		return "addition";
	case Expression_node::operator_infix_sub:
		return "subtraction";
	case Expression_node::operator_infix_mul:
		return "multiplication";
	case Expression_node::operator_infix_div:
		return "division";
	case Expression_node::operator_infix_mod:
		return "modulo";
	case Expression_node::operator_infix_sll:
		return "logical left shift";
	case Expression_node::operator_infix_sla:
		return "arithmetic left shift";
	case Expression_node::operator_infix_srl:
		return "logical right shift";
	case Expression_node::operator_infix_sra:
		return "arithmetic right shift";
	case Expression_node::operator_infix_andb:
		return "bitwise and";
	case Expression_node::operator_infix_orb:
		return "bitwise or";
	case Expression_node::operator_infix_xorb:
		return "bitwise xor";
	case Expression_node::operator_infix_assign:
		return "assginment";
	default:
		ASTERIA_DEBUG_LOG("An unknown operator enumeration `", op, "` is encountered. This is probably a bug. Please report.");
		return "<unknown>";
	}
}

namespace {
	Expression_node do_bind_expression_node(const Expression_node &node, Sp_cref<const Scope> scope){
		const auto type = node.which();
		switch(type){
		case Expression_node::index_literal: {
			const auto &cand = node.as<Expression_node::S_literal>();
			// Copy it as is.
			return cand; }

		case Expression_node::index_named_reference: {
			const auto &cand = node.as<Expression_node::S_named_reference>();
			// Look up the reference in the enclosing scope.
			Sp<const Reference> source_ref;
			auto scope_cur = scope;
			for(;;){
				if(!scope_cur){
					ASTERIA_THROW_RUNTIME_ERROR("The identifier `", cand.id, "` has not been declared yet.");
				}
				source_ref = scope_cur->get_named_reference_opt(cand.id);
				if(source_ref){
					break;
				}
				scope_cur = scope_cur->get_parent_opt();
			}
			// If the reference is in a lexical scope rather than a run-time scope, don't bind it.
			if(scope_cur->get_purpose() == Scope::purpose_lexical){
				return cand;
			}
			// Bind the reference.
			Vp<Reference> bound_ref;
			copy_reference(bound_ref, source_ref);
			Expression_node::S_bound_reference node_br = { std::move(bound_ref) };
			return std::move(node_br); }

		case Expression_node::index_bound_reference: {
			const auto &cand = node.as<Expression_node::S_bound_reference>();
			// Copy the reference bound.
			Vp<Reference> bound_ref;
			copy_reference(bound_ref, cand.ref_opt);
			Expression_node::S_bound_reference node_br = { std::move(bound_ref) };
			return std::move(node_br); }

		case Expression_node::index_subexpression: {
			const auto &cand = node.as<Expression_node::S_subexpression>();
			// Bind the subexpression recursively.
			auto bound_expr = bind_expression(cand.subexpr, scope);
			Expression_node::S_subexpression node_s = { std::move(bound_expr) };
			return std::move(node_s); }

		case Expression_node::index_lambda_definition: {
			const auto &cand = node.as<Expression_node::S_lambda_definition>();
			// Bind the function body onto the current scope.
			const auto scope_lexical = std::make_shared<Scope>(Scope::purpose_lexical, scope);
			prepare_function_scope_lexical(scope_lexical, cand.location, cand.params);
			auto bound_body = bind_block_in_place(scope_lexical, cand.body_opt);
			Expression_node::S_lambda_definition node_l = { cand.location, cand.params, std::move(bound_body) };
			return std::move(node_l); }

		case Expression_node::index_branch: {
			const auto &cand = node.as<Expression_node::S_branch>();
			// Bind both branches recursively.
			auto bound_branch_true = bind_expression(cand.branch_true, scope);
			auto bound_branch_false = bind_expression(cand.branch_false, scope);
			Expression_node::S_branch node_b = { std::move(bound_branch_true), std::move(bound_branch_false) };
			return std::move(node_b); }

		case Expression_node::index_function_call: {
			const auto &cand = node.as<Expression_node::S_function_call>();
			return cand; }

		case Expression_node::index_operator_rpn: {
			const auto &cand = node.as<Expression_node::S_operator_rpn>();
			return cand; }

		default:
			ASTERIA_DEBUG_LOG("An unknown expression node type enumeration `", type, "` is encountered. This is probably a bug. Please report.");
			std::terminate();
		}
	}
}

Vector<Expression_node> bind_expression(const Vector<Expression_node> &expr, Sp_cref<const Scope> scope){
	Vector<Expression_node> bound_expr;
	bound_expr.reserve(expr.size());
	// Bind expression nodes recursively.
	for(const auto &node : expr){
		auto bound_node = do_bind_expression_node(node, scope);
		bound_expr.emplace_back(std::move(bound_node));
	}
	return bound_expr;
}

namespace {
	void do_push_reference(Vector<Vp<Reference>> &stack_inout, Vp<Reference> &&ref){
		stack_inout.emplace_back(std::move(ref));
	}
	Vp<Reference> do_pop_reference(Vector<Vp<Reference>> &stack_inout){
		if(stack_inout.empty()){
			ASTERIA_THROW_RUNTIME_ERROR("The evaluation stack was empty.");
		}
		auto ref = std::move(stack_inout.back());
		stack_inout.pop_back();
		return ref;
	}

	template<typename ResultT>
	void do_set_result(Vp<Reference> &ref_inout_opt, bool assign, ResultT &&result){
		if(assign){
			// Update the result in-place.
			const auto wref = drill_reference(ref_inout_opt);
			set_value(wref, std::forward<ResultT>(result));
		} else {
			// Create a new variable for the result, then replace `lhs_ref` with an rvalue reference to it.
			Vp<Value> value;
			set_value(value, std::forward<ResultT>(result));
			Reference::S_temporary_value ref_d = { std::move(value) };
			set_reference(ref_inout_opt, std::move(ref_d));
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
		using limits = std::numeric_limits<D_integer>;
		if(rhs == limits::min()){
			ASTERIA_THROW_RUNTIME_ERROR("Integral negation of `", rhs, "` would result in overflow.");
		}
		return -rhs;
	}
	D_integer do_add(D_integer lhs, D_integer rhs){
		using limits = std::numeric_limits<D_integer>;
		if((rhs >= 0) ? (lhs > limits::max() - rhs) : (lhs < limits::min() - rhs)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral addition of `", lhs, "` and `", rhs, "` would result in overflow.");
		}
		return lhs + rhs;
	}
	D_integer do_subtract(D_integer lhs, D_integer rhs){
		using limits = std::numeric_limits<D_integer>;
		if((rhs >= 0) ? (lhs < limits::min() + rhs) : (lhs > limits::max() + rhs)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral subtraction of `", lhs, "` and `", rhs, "` would result in overflow.");
		}
		return lhs - rhs;
	}
	D_integer do_multiply(D_integer lhs, D_integer rhs){
		using limits = std::numeric_limits<D_integer>;
		if((lhs == 0) || (rhs == 0)){
			return 0;
		}
		if((lhs == 1) || (rhs == 1)){
			return lhs ^ rhs ^ 1;
		}
		if((lhs == limits::min()) || (rhs == limits::min())){
			ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication of `", lhs, "` and `", rhs, "` would result in overflow.");
		}
		if((lhs == -1) || (rhs == -1)){
			return -(lhs ^ rhs ^ -1);
		}
		const auto slhs = (rhs >= 0) ? lhs : -lhs;
		const auto arhs = std::abs(rhs);
		if((slhs >= 0) ? (slhs > limits::max() / arhs) : (slhs < limits::min() / arhs)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication of `", lhs, "` and `", rhs, "` would result in overflow.");
		}
		return slhs * arhs;
	}
	D_integer do_divide(D_integer lhs, D_integer rhs){
		using limits = std::numeric_limits<D_integer>;
		if(rhs == 0){
			ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
		}
		if((lhs == limits::min()) && (rhs == -1)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral division of `", lhs, "` and `", rhs, "` would result in overflow.");
		}
		return lhs / rhs;
	}
	D_integer do_modulo(D_integer lhs, D_integer rhs){
		using limits = std::numeric_limits<D_integer>;
		if(rhs == 0){
			ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
		}
		if((lhs == limits::min()) && (rhs == -1)){
			ASTERIA_THROW_RUNTIME_ERROR("Integral division of `", lhs, "` and `", rhs, "` would result in overflow.");
		}
		return lhs % rhs;
	}

	D_integer do_shift_left_logical(D_integer lhs, D_integer rhs){
		using limits = std::numeric_limits<D_integer>;
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` was negative.");
		}
		if(rhs > limits::digits){
			return 0;
		}
		auto reg = Unsigned_integer(lhs);
		reg <<= rhs;
		return D_integer(reg);
	}
	D_integer do_shift_right_logical(D_integer lhs, D_integer rhs){
		using limits = std::numeric_limits<D_integer>;
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` was negative.");
		}
		if(rhs > limits::digits){
			return 0;
		}
		auto reg = Unsigned_integer(lhs);
		reg >>= rhs;
		return D_integer(reg);
	}
	D_integer do_shift_left_arithmetic(D_integer lhs, D_integer rhs){
		using limits = std::numeric_limits<D_integer>;
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` was negative.");
		}
		if(rhs > limits::digits){
			ASTERIA_THROW_RUNTIME_ERROR("Arithmetic bit shift count `", rhs, "` for `", lhs, "` was larger than the width of an `integer`.");
		}
		const auto bits_rem = static_cast<unsigned char>(limits::digits - rhs);
		auto reg = Unsigned_integer(lhs);
		const auto mask_out = (reg >> bits_rem) << bits_rem;
		const auto mask_sign = -(reg >> limits::digits) << bits_rem;
		if(mask_out != mask_sign){
			ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
		}
		reg <<= rhs;
		return D_integer(reg);
	}
	D_integer do_shift_right_arithmetic(D_integer lhs, D_integer rhs){
		using limits = std::numeric_limits<D_integer>;
		if(rhs < 0){
			ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` was negative.");
		}
		if(rhs > limits::digits){
			ASTERIA_THROW_RUNTIME_ERROR("Arithmetic bit shift count `", rhs, "` for `", lhs, "` was larger than the width of an `integer`.");
		}
		const auto bits_rem = static_cast<unsigned char>(limits::digits - rhs);
		auto reg = Unsigned_integer(lhs);
		const auto mask_in = -(reg >> limits::digits) << bits_rem;
		reg >>= rhs;
		reg |= mask_in;
		return D_integer(reg);
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
		const auto count = Unsigned_integer(rhs);
		if(lhs.size() > res.max_size() / count){
			ASTERIA_THROW_RUNTIME_ERROR("Duplication of `", lhs, "` up to `", rhs, "` time(s) would result in an overlong string that cannot be allocated.");
		}
		res.reserve(lhs.size() * static_cast<std::size_t>(count));
		Unsigned_integer mask = 1;
		mask <<= 63;
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

	void do_evaluate_expression_node(Vector<Vp<Reference>> &stack_inout, const Expression_node &node, Sp_cref<const Scope> scope){
		const auto type = node.which();
		switch(type){
		case Expression_node::index_literal: {
			const auto &cand = node.as<Expression_node::S_literal>();
			// Create an constant reference to the constant.
			Vp<Reference> result_ref;
			Reference::S_constant ref_k = { cand.src_opt };
			set_reference(result_ref, std::move(ref_k));
			do_push_reference(stack_inout, std::move(result_ref));
			break; }

		case Expression_node::index_named_reference: {
			const auto &cand = node.as<Expression_node::S_named_reference>();
			// Look up the reference in the enclosing scope.
			Sp<const Reference> source_ref;
			auto scope_cur = scope;
			for(;;){
				if(!scope_cur){
					ASTERIA_THROW_RUNTIME_ERROR("The identifier `", cand.id, "` has not been declared yet.");
				}
				source_ref = scope_cur->get_named_reference_opt(cand.id);
				if(source_ref){
					break;
				}
				scope_cur = scope_cur->get_parent_opt();
			}
			Vp<Reference> result_ref;
			copy_reference(result_ref, source_ref);
			// Push the reference onto the stack as is.
			do_push_reference(stack_inout, std::move(result_ref));
			break; }

		case Expression_node::index_bound_reference: {
			const auto &cand = node.as<Expression_node::S_bound_reference>();
			// Copy the reference bound.
			Vp<Reference> bound_ref;
			copy_reference(bound_ref, cand.ref_opt);
			// Push the reference onto the stack as is.
			do_push_reference(stack_inout, std::move(bound_ref));
			break; }

		case Expression_node::index_subexpression: {
			const auto &cand = node.as<Expression_node::S_subexpression>();
			// Evaluate the subexpression recursively.
			Vp<Reference> result_ref;
			evaluate_expression(result_ref, cand.subexpr, scope);
			// Push the result reference onto the stack as is.
			do_push_reference(stack_inout, std::move(result_ref));
			break; }

		case Expression_node::index_lambda_definition: {
			const auto &cand = node.as<Expression_node::S_lambda_definition>();
			// Bind the function body onto the current scope.
			const auto scope_lexical = std::make_shared<Scope>(Scope::purpose_lexical, scope);
			prepare_function_scope_lexical(scope_lexical, cand.location, cand.params);
			auto bound_body = bind_block_in_place(scope_lexical, cand.body_opt);
			// Create a temporary variable for the function.
			auto func = std::make_shared<Instantiated_function>("lambda", cand.location, cand.params, std::move(bound_body));
			Vp<Value> func_var;
			set_value(func_var, D_function(std::move(func)));
			Vp<Reference> result_ref;
			Reference::S_temporary_value ref_d = { std::move(func_var) };
			set_reference(result_ref, std::move(ref_d));
			// Push the result onto the stack.
			do_push_reference(stack_inout, std::move(result_ref));
			break; }

		case Expression_node::index_branch: {
			const auto &cand = node.as<Expression_node::S_branch>();
			// Pop the condition off the stack.
			auto condition_ref = do_pop_reference(stack_inout);
			// Pick a branch basing on the condition.
			const auto condition_var = read_reference_opt(condition_ref);
			const auto &branch_taken = test_value(condition_var) ? cand.branch_true : cand.branch_false;
			if(branch_taken.empty()){
				// If the branch is empty, push the condition instead.
				do_push_reference(stack_inout, std::move(condition_ref));
				break;
			}
			// Evaluate the branch and push the result.
			Vp<Reference> result_ref;
			evaluate_expression(result_ref, branch_taken, scope);
			do_push_reference(stack_inout, std::move(result_ref));
			break; }

		case Expression_node::index_function_call: {
			const auto &cand = node.as<Expression_node::S_function_call>();
			// Pop the function off the stack.
			auto callee_ref = do_pop_reference(stack_inout);
			const auto callee_var = read_reference_opt(callee_ref);
			// Make sure it is really a function.
			const auto callee_type = get_value_type(callee_var);
			if(callee_type != Value::type_function){
				ASTERIA_THROW_RUNTIME_ERROR("Only functions can be called, while the operand has type `", get_type_name(callee_type), "`.");
			}
			const auto &callee = callee_var->as<D_function>();
			ROCKET_ASSERT(callee);
			// Allocate the argument vector. There will be no fewer arguments than params.
			Vector<Vp<Reference>> args;
			args.reserve(32);
			// Pop arguments off the stack.
			for(std::size_t i = 0; i < cand.argument_count; ++i){
				auto arg_ref = do_pop_reference(stack_inout);
				args.emplace_back(std::move(arg_ref));
			}
			// Get the `this` reference.
			Vp<Reference> this_ref;
			const auto callee_ref_type = get_reference_type(callee_ref);
			if(callee_ref_type == Reference::index_array_element){
				auto &array = callee_ref->as<Reference::S_array_element>();
				this_ref = std::move(array.parent_opt);
			} else if(callee_ref_type == Reference::index_object_member){
				auto &object = callee_ref->as<Reference::S_object_member>();
				this_ref = std::move(object.parent_opt);
			}
			// Call the function and push the result as is.
			callee->invoke(callee_ref, std::move(this_ref), std::move(args));
			do_push_reference(stack_inout, std::move(callee_ref));
			break; }

		case Expression_node::index_operator_rpn: {
			const auto &cand = node.as<Expression_node::S_operator_rpn>();
			switch(cand.op){
			case Expression_node::operator_postfix_inc: {
				// Pop the operand off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				// Increment the operand and return the old value.
				// `assign` is ignored.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				if(lhs_type == Value::type_integer){
					const auto lhs = lhs_var->as<D_integer>();
					do_set_result(lhs_ref, true, do_add(lhs, D_integer(1)));
					do_set_result(lhs_ref, false, lhs);
				} else if(lhs_type == Value::type_double){
					const auto lhs = lhs_var->as<D_double>();
					do_set_result(lhs_ref, true, do_add(lhs, D_double(1)));
					do_set_result(lhs_ref, false, lhs);
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_postfix_dec: {
				// Pop the operand off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				// Decrement the operand and return the old value.
				// `assign` is ignored.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				if(lhs_type == Value::type_integer){
					const auto lhs = lhs_var->as<D_integer>();
					do_set_result(lhs_ref, true, do_subtract(lhs, D_integer(1)));
					do_set_result(lhs_ref, false, lhs);
				} else if(lhs_type == Value::type_double){
					const auto lhs = lhs_var->as<D_double>();
					do_set_result(lhs_ref, true, do_subtract(lhs, D_double(1)));
					do_set_result(lhs_ref, false, lhs);
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_postfix_at: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// The subscript operand shall have type `integer` or `string`. In neither case will `rhs_ref` be null, hence it can be safely reused.
				// `assign` is ignored.
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto rhs_type = get_value_type(rhs_var);
				if(rhs_type == Value::type_integer){
					Reference::S_array_element ref_a = { std::move(lhs_ref), rhs_var->as<D_integer>() };
					rhs_ref->set(std::move(ref_a));
				} else if(rhs_type == Value::type_string){
					Reference::S_object_member ref_o = { std::move(lhs_ref), rhs_var->as<D_string>() };
					rhs_ref->set(std::move(ref_o));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Values having type `", get_type_name(rhs_type), "` cannot be used as subscripts.");
				}
				do_push_reference(stack_inout, std::move(rhs_ref));
				break; }

			case Expression_node::operator_prefix_pos: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack_inout);
				// Copy the referenced variable to create an rvalue, then return it.
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto rhs_type = get_value_type(rhs_var);
				if(rhs_type == Value::type_integer){
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(rhs_ref, cand.assign, rhs);
				} else if(rhs_type == Value::type_double){
					const auto rhs = rhs_var->as<D_double>();
					do_set_result(rhs_ref, cand.assign, rhs);
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(rhs_ref));
				break; }

			case Expression_node::operator_prefix_neg: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack_inout);
				// Negate the operand.
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto rhs_type = get_value_type(rhs_var);
				if(rhs_type == Value::type_integer){
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(rhs_ref, cand.assign, do_negate(rhs));
				} else if(rhs_type == Value::type_double){
					const auto rhs = rhs_var->as<D_double>();
					do_set_result(rhs_ref, cand.assign, do_negate(rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(rhs_ref));
				break; }

			case Expression_node::operator_prefix_notb: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack_inout);
				// Bitwise NOT the operand.
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto rhs_type = get_value_type(rhs_var);
				if(rhs_type == Value::type_boolean){
					const auto rhs = rhs_var->as<D_boolean>();
					do_set_result(rhs_ref, cand.assign, do_logical_not(rhs));
				} else if(rhs_type == Value::type_integer){
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(rhs_ref, cand.assign, do_bitwise_not(rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(rhs_ref));
				break; }

			case Expression_node::operator_prefix_notl: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack_inout);
				// Convert the operand to a `boolean` value, which is an rvalue, negate it, then return it.
				// N.B. This is one of the few operators that work on all types.
				const auto rhs_var = read_reference_opt(rhs_ref);
				do_set_result(rhs_ref, cand.assign, do_logical_not(test_value(rhs_var)));
				do_push_reference(stack_inout, std::move(rhs_ref));
				break; }

			case Expression_node::operator_prefix_inc: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack_inout);
				// Increment the operand and return it.
				// `assign` is ignored.
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto rhs_type = get_value_type(rhs_var);
				if(rhs_type == Value::type_integer){
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(rhs_ref, true, do_add(rhs, D_integer(1)));
				} else if(rhs_type == Value::type_double){
					const auto rhs = rhs_var->as<D_double>();
					do_set_result(rhs_ref, true, do_add(rhs, D_double(1)));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(rhs_ref));
				break; }

			case Expression_node::operator_prefix_dec: {
				// Pop the operand off the stack.
				auto rhs_ref = do_pop_reference(stack_inout);
				// Decrement the operand and return it.
				// `assign` is ignored.
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto rhs_type = get_value_type(rhs_var);
				if(rhs_type == Value::type_integer){
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(rhs_ref, true, do_subtract(rhs, D_integer(1)));
				} else if(rhs_type == Value::type_double){
					const auto rhs = rhs_var->as<D_double>();
					do_set_result(rhs_ref, true, do_subtract(rhs, D_double(1)));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(rhs_ref));
				break; }

			case Expression_node::operator_infix_cmp_eq: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Report unordered operands as being unequal.
				// N.B. This is one of the few operators that work on all types.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto comparison = compare_values(lhs_var, rhs_var);
				// Try reusing source operands.
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				do_set_result(lhs_ref, false, comparison == Value::comparison_equal);
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_cmp_ne: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Report unordered operands as being unequal.
				// N.B. This is one of the few operators that work on all types.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto comparison = compare_values(lhs_var, rhs_var);
				// Try reusing source operands.
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				do_set_result(lhs_ref, false, comparison != Value::comparison_equal);
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_cmp_lt: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Throw an exception in case of unordered operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto comparison = compare_values(lhs_var, rhs_var);
				if(comparison == Value::comparison_unordered){
					ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_var, "` and `", rhs_var, "` are unordered.");
				}
				// Try reusing source operands.
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				do_set_result(lhs_ref, false, comparison == Value::comparison_less);
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_cmp_gt: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Throw an exception in case of unordered operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto comparison = compare_values(lhs_var, rhs_var);
				if(comparison == Value::comparison_unordered){
					ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_var, "` and `", rhs_var, "` are unordered.");
				}
				// Try reusing source operands.
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				do_set_result(lhs_ref, false, comparison == Value::comparison_greater);
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_cmp_lte: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Throw an exception in case of unordered operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto comparison = compare_values(lhs_var, rhs_var);
				if(comparison == Value::comparison_unordered){
					ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_var, "` and `", rhs_var, "` are unordered.");
				}
				// Try reusing source operands.
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				do_set_result(lhs_ref, false, comparison != Value::comparison_greater);
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_cmp_gte: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Throw an exception in case of unordered operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto comparison = compare_values(lhs_var, rhs_var);
				if(comparison == Value::comparison_unordered){
					ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_var, "` and `", rhs_var, "` are unordered.");
				}
				// Try reusing source operands.
				if(!lhs_ref){
					lhs_ref = std::move(rhs_ref);
				}
				do_set_result(lhs_ref, false, comparison != Value::comparison_less);
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_add: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// For the boolean type, return the logical OR'd result of both operands.
				// For the integer and double types, return the sum of both operands.
				// For the string type, concatenate the operands in lexical order to create a new string, then return it.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				const auto rhs_type = get_value_type(rhs_var);
				if((lhs_type == Value::type_boolean) && (rhs_type == Value::type_boolean)){
					const auto lhs = lhs_var->as<D_boolean>();
					const auto rhs = rhs_var->as<D_boolean>();
					do_set_result(lhs_ref, cand.assign, do_logical_or(lhs, rhs));
				} else if((lhs_type == Value::type_integer) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_add(lhs, rhs));
				} else if((lhs_type == Value::type_double) && (rhs_type == Value::type_double)){
					const auto lhs = lhs_var->as<D_double>();
					const auto rhs = rhs_var->as<D_double>();
					do_set_result(lhs_ref, cand.assign, do_add(lhs, rhs));
				} else if((lhs_type == Value::type_string) && (rhs_type == Value::type_string)){
					const auto lhs = lhs_var->as<D_string>();
					const auto rhs = rhs_var->as<D_string>();
					do_set_result(lhs_ref, cand.assign, do_concatenate(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` and type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_sub: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// For the boolean type, return the logical XOR'd result of both operands.
				// For the integer and double types, return the difference of both operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				const auto rhs_type = get_value_type(rhs_var);
				if((lhs_type == Value::type_boolean) && (rhs_type == Value::type_boolean)){
					const auto lhs = lhs_var->as<D_boolean>();
					const auto rhs = rhs_var->as<D_boolean>();
					do_set_result(lhs_ref, cand.assign, do_logical_xor(lhs, rhs));
				} else if((lhs_type == Value::type_integer) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_subtract(lhs, rhs));
				} else if((lhs_type == Value::type_double) && (rhs_type == Value::type_double)){
					const auto lhs = lhs_var->as<D_double>();
					const auto rhs = rhs_var->as<D_double>();
					do_set_result(lhs_ref, cand.assign, do_subtract(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` and type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_mul: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// For the boolean type, return the logical AND'd result of both operands.
				// For the integer and double types, return the product of both operands.
				// If either operand has the integer type and the other has the string type, duplicate the string up to the specified number of times.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				const auto rhs_type = get_value_type(rhs_var);
				if((lhs_type == Value::type_boolean) && (rhs_type == Value::type_boolean)){
					const auto lhs = lhs_var->as<D_boolean>();
					const auto rhs = rhs_var->as<D_boolean>();
					do_set_result(lhs_ref, cand.assign, do_logical_and(lhs, rhs));
				} else if((lhs_type == Value::type_integer) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_multiply(lhs, rhs));
				} else if((lhs_type == Value::type_double) && (rhs_type == Value::type_double)){
					const auto lhs = lhs_var->as<D_double>();
					const auto rhs = rhs_var->as<D_double>();
					do_set_result(lhs_ref, cand.assign, do_multiply(lhs, rhs));
				} else if((lhs_type == Value::type_string) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_string>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_duplicate(lhs, rhs));
				} else if((lhs_type == Value::type_integer) && (rhs_type == Value::type_string)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_string>();
					do_set_result(lhs_ref, cand.assign, do_duplicate(rhs, lhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` and type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_div: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Divide the first operand by the second operand and return the quotient.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				const auto rhs_type = get_value_type(rhs_var);
				if((lhs_type == Value::type_integer) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_divide(lhs, rhs));
				} else if((lhs_type == Value::type_double) && (rhs_type == Value::type_double)){
					const auto lhs = lhs_var->as<D_double>();
					const auto rhs = rhs_var->as<D_double>();
					do_set_result(lhs_ref, cand.assign, do_divide(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` and type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_mod: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Divide the first operand by the second operand and return the remainder.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				const auto rhs_type = get_value_type(rhs_var);
				if((lhs_type == Value::type_integer) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_modulo(lhs, rhs));
				} else if((lhs_type == Value::type_double) && (rhs_type == Value::type_double)){
					const auto lhs = lhs_var->as<D_double>();
					const auto rhs = rhs_var->as<D_double>();
					do_set_result(lhs_ref, cand.assign, do_modulo(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` and type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_sll: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Shift the first operand to the left by the number of bits specified by the second operand
				// Bits shifted out are discarded. Bits shifted in are filled with zeroes.
				// Both operands have to be integers.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				const auto rhs_type = get_value_type(rhs_var);
				if((lhs_type == Value::type_integer) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_shift_left_logical(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` and type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_srl: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Shift the first operand to the right by the number of bits specified by the second operand
				// Bits shifted out are discarded. Bits shifted in are filled with zeroes.
				// Both operands have to be integers.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				const auto rhs_type = get_value_type(rhs_var);
				if((lhs_type == Value::type_integer) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_shift_right_logical(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` and type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_sla: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Shift the first operand to the left by the number of bits specified by the second operand
				// Bits shifted out that equal the sign bit are dicarded. Bits shifted in are filled with zeroes.
				// If a bit unequal to the sign bit would be shifted into or across the sign bit, an exception is thrown.
				// Both operands have to be integers.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				const auto rhs_type = get_value_type(rhs_var);
				if((lhs_type == Value::type_integer) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_shift_left_arithmetic(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` and type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_sra: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Shift the first operand to the right by the number of bits specified by the second operand
				// Bits shifted out are discarded. Bits shifted in are filled with the sign bit.
				// Both operands have to be integers.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				const auto rhs_type = get_value_type(rhs_var);
				if((lhs_type == Value::type_integer) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_shift_right_arithmetic(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` and type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_andb: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Perform bitwise and on both operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				const auto rhs_type = get_value_type(rhs_var);
				if((lhs_type == Value::type_boolean) && (rhs_type == Value::type_boolean)){
					const auto lhs = lhs_var->as<D_boolean>();
					const auto rhs = rhs_var->as<D_boolean>();
					do_set_result(lhs_ref, cand.assign, do_logical_and(lhs, rhs));
				} else if((lhs_type == Value::type_integer) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_bitwise_and(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` and type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_orb: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Perform bitwise or on both operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				const auto rhs_type = get_value_type(rhs_var);
				if((lhs_type == Value::type_boolean) && (rhs_type == Value::type_boolean)){
					const auto lhs = lhs_var->as<D_boolean>();
					const auto rhs = rhs_var->as<D_boolean>();
					do_set_result(lhs_ref, cand.assign, do_logical_or(lhs, rhs));
				} else if((lhs_type == Value::type_integer) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_bitwise_or(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` and type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_xorb: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Perform bitwise xor on both operands.
				const auto lhs_var = read_reference_opt(lhs_ref);
				const auto rhs_var = read_reference_opt(rhs_ref);
				const auto lhs_type = get_value_type(lhs_var);
				const auto rhs_type = get_value_type(rhs_var);
				if((lhs_type == Value::type_boolean) && (rhs_type == Value::type_boolean)){
					const auto lhs = lhs_var->as<D_boolean>();
					const auto rhs = rhs_var->as<D_boolean>();
					do_set_result(lhs_ref, cand.assign, do_logical_xor(lhs, rhs));
				} else if((lhs_type == Value::type_integer) && (rhs_type == Value::type_integer)){
					const auto lhs = lhs_var->as<D_integer>();
					const auto rhs = rhs_var->as<D_integer>();
					do_set_result(lhs_ref, cand.assign, do_bitwise_xor(lhs, rhs));
				} else {
					ASTERIA_THROW_RUNTIME_ERROR("Operation `", get_operator_name(cand.op), "` on type `", get_type_name(lhs_type), "` and type `", get_type_name(rhs_type), "` is undefined.");
				}
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			case Expression_node::operator_infix_assign: {
				// Pop two operands off the stack.
				auto lhs_ref = do_pop_reference(stack_inout);
				auto rhs_ref = do_pop_reference(stack_inout);
				// Copy the variable referenced by `rhs_ref` into `lhs_ref`, then return it.
				// `assign` is ignored.
				// N.B. This is one of the few operators that work on all types.
				const auto wref = drill_reference(lhs_ref);
				extract_value_from_reference(wref, std::move(rhs_ref));
				do_push_reference(stack_inout, std::move(lhs_ref));
				break; }

			default:
				ASTERIA_DEBUG_LOG("An unknown operator enumeration `", cand.op, "` is encountered. This is probably a bug. Please report.");
				std::terminate();
			}
			break; }

		default:
			ASTERIA_DEBUG_LOG("An unknown expression node type enumeration `", type, "` is encountered. This is probably a bug. Please report.");
			std::terminate();
		}
	}
}

void evaluate_expression(Vp<Reference> &result_out, const Vector<Expression_node> &expr, Sp_cref<const Scope> scope){
	move_reference(result_out, nullptr);
	Vector<Vp<Reference>> stack;
	// Parameters are pushed from right to left, in lexical order.
	for(const auto &node : expr){
		do_evaluate_expression_node(stack, node, scope);
	}
	// Get the result. If the stack is empty or has more than one element, the expression is unbalanced.
	if(stack.size() != 1){
		ASTERIA_THROW_RUNTIME_ERROR("The expression was unbalanced. There should be exactly one reference left in the evaluation stack, but there were `", stack.size(), "`.");
	}
	move_reference(result_out, std::move(stack.front()));
}

}
