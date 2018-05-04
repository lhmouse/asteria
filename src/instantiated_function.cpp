// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "instantiated_function.hpp"
#include "reference.hpp"
#include "stored_reference.hpp"
#include "block.hpp"
#include "parameter.hpp"
#include "scope.hpp"
#include "utilities.hpp"

namespace Asteria {

Instantiated_function::~Instantiated_function() = default;

const char *Instantiated_function::describe() const noexcept {
	return "instantiated function";
}
void Instantiated_function::invoke(Xptr<Reference> &result_out, Spcref<Recycler> recycler, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt) const {
	// Allocate a function scope.
	const auto scope_with_args = std::make_shared<Scope>(Scope::purpose_function, nullptr);
	prepare_function_arguments(arguments_opt, m_parameters_opt);
	prepare_function_scope(scope_with_args, recycler, m_parameters_opt, std::move(this_opt), std::move(arguments_opt));
	// Execute the body.
	Xptr<Reference> returned_ref;
	const auto exec_result = execute_block_in_place(returned_ref, scope_with_args, recycler, m_bound_body_opt);
	switch(exec_result){
	case Block::execution_result_end_of_block:
		// If control flow reaches the end of the function, return `null`.
		return set_reference(result_out, nullptr);
	case Block::execution_result_break_unspecified:
	case Block::execution_result_break_switch:
	case Block::execution_result_break_while:
	case Block::execution_result_break_for:
	case Block::execution_result_break_foreach:
		ASTERIA_THROW_RUNTIME_ERROR("`break` statement encountered outside a `switch` or loop statement");
	case Block::execution_result_continue_unspecified:
	case Block::execution_result_continue_while:
	case Block::execution_result_continue_for:
	case Block::execution_result_continue_foreach:
		ASTERIA_THROW_RUNTIME_ERROR("`continue` statement encountered outside a loop statement");
	case Block::execution_result_return:
		// Forward the return value;
		return move_reference(result_out, std::move(returned_ref));
	default:
		ASTERIA_DEBUG_LOG("Unknown compound statement execution result enumeration `", exec_result, "`. This is probably a bug, please report.");
		std::terminate();
	}
}

}
