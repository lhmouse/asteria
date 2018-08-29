// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "instantiated_function.hpp"
#include "executive_context.hpp"
#include "utilities.hpp"

namespace Asteria {

Instantiated_function::~Instantiated_function()
  {
  }

String Instantiated_function::describe() const
  {
    return ASTERIA_FORMAT_STRING("function defined at \'", m_file, ':', m_line, "\'");
  }

Reference Instantiated_function::invoke(Reference self, Vector<Reference> args) const
  {
    // Create an orphan context.
    Executive_context ctx_next;
    initialize_executive_function_context(ctx_next, m_params, m_file, m_line, std::move(self), std::move(std::move(args)));
    // Execute the function body.
    Reference ref;
    const auto status = execute_block_in_place(ref, ctx_next, m_body);
    switch(status) {
    case Statement::status_next:
      // Return `null` because the control flow reached the end of the function.
      return Reference();
    case Statement::status_break_unspec:
    case Statement::status_break_switch:
    case Statement::status_break_while:
    case Statement::status_break_for:
      ASTERIA_THROW_RUNTIME_ERROR("`break` statements are not allowed outside matching `switch` or loop statements.");
    case Statement::status_continue_unspec:
    case Statement::status_continue_while:
    case Statement::status_continue_for:
      ASTERIA_THROW_RUNTIME_ERROR("`continue` statements are not allowed outside matching loop statements.");
    case Statement::status_return:
      // Forward the result reference.
      return std::move(ref);
    default:
      ASTERIA_TERMINATE("An unknown execution result enumeration `", status, "` has been encountered.");
    }
  }

}
