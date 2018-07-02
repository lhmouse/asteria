// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "instantiated_function.hpp"
#include "stored_reference.hpp"
#include "block.hpp"
#include "parameter.hpp"
#include "scope.hpp"
#include "utilities.hpp"

namespace Asteria {

Instantiated_function::~Instantiated_function() = default;

D_string Instantiated_function::describe() const {
	return ASTERIA_FORMAT_STRING(m_category, " @ '", m_source, "'");
}
void Instantiated_function::invoke(Vp<Reference> &result_out, Sp_ref<Recycler> recycler_out, Vp<Reference> &&this_opt, Vector<Vp<Reference>> &&args) const {
	// Allocate a function scope.
	const auto scope_with_args = std::make_shared<Scope>(Scope::purpose_function, nullptr);
	prepare_function_arguments(args, m_params);
	prepare_function_scope(scope_with_args, recycler_out, m_source, m_params, std::move(this_opt), std::move(args));
	// Execute the body.
	const auto exec_result = execute_block_in_place(result_out, scope_with_args, recycler_out, m_bound_body_opt);
	switch(exec_result){
	case Statement::execution_result_next:
		// If control flow reaches the end of the function, return `null`.
		return set_reference(result_out, nullptr);
	case Statement::execution_result_break_unspecified:
	case Statement::execution_result_break_switch:
	case Statement::execution_result_break_while:
	case Statement::execution_result_break_for:
		ASTERIA_THROW_RUNTIME_ERROR("`break` statements are not allowed outside matching `switch` or loop statements.");
	case Statement::execution_result_continue_unspecified:
	case Statement::execution_result_continue_while:
	case Statement::execution_result_continue_for:
		ASTERIA_THROW_RUNTIME_ERROR("`continue` statements are not allowed outside matching loop statements.");
	case Statement::execution_result_return:
		// Forward the result reference;
		return;
	default:
		ASTERIA_DEBUG_LOG("An unknown statement execution result enumeration `", exec_result, "` is encountered. This is probably a bug. Please report.");
		std::terminate();
	}
}

}
