// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Expression::Expression(Expression &&) noexcept = default;
Expression & Expression::operator=(Expression &&) noexcept = default;
Expression::~Expression() = default;

void bind_expression(Vp<Expression> &bound_expr_out, Spr<const Expression> expression_opt, Spr<const Scope> scope){
	if(expression_opt == nullptr){
		// Return a null expression.
		return bound_expr_out.reset();
	}
	// Bind nodes recursively.
	T_vector<Expression_node> bound_nodes;
	bound_nodes.reserve(expression_opt->size());
	for(const auto &node : *expression_opt){
		bind_expression_node(bound_nodes, node, scope);
	}
	return bound_expr_out.emplace(std::move(bound_nodes));
}
void evaluate_expression(Vp<Reference> &result_out, Spr<Recycler> recycler, Spr<const Expression> expression_opt, Spr<const Scope> scope){
	if(expression_opt == nullptr){
		// Return a null reference only when a null expression is given.
		return move_reference(result_out, nullptr);
	}
	ASTERIA_DEBUG_LOG("------ Beginning of evaluation of expression");
	// Parameters are pushed from right to left, in lexical order.
	Vp_vector<Reference> stack;
	for(const auto &node : *expression_opt){
		evaluate_expression_node(stack, recycler, node, scope);
	}
	// Get the result. If the stack is empty or has more than one elements, the expression is unbalanced.
	if(stack.size() != 1){
		ASTERIA_THROW_RUNTIME_ERROR("There were `", stack.size(), "` elements in the evaluation stack due to imbalance of the expression.");
	}
	ASTERIA_DEBUG_LOG("------- End of evaluation of expression");
	return move_reference(result_out, std::move(stack.front()));
}

}
