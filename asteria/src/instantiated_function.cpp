// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "instantiated_function.hpp"
#include "reference.hpp"
#include "executive_context.hpp"
#include "statement.hpp"
#include "utilities.hpp"

namespace Asteria {

Instantiated_function::~Instantiated_function()
  {
  }

String Instantiated_function::describe() const
  {
    return ASTERIA_FORMAT_STRING("function defined at \'", this->m_file, ':', this->m_line, "\'");
  }

Reference Instantiated_function::invoke(Reference self, Vector<Reference> args) const
  {
    // Create an orphan context.
    Executive_context ctx_next;
    ctx_next.initialize_for_function(this->m_params, this->m_file, this->m_line, std::move(self), std::move(std::move(args)));
    // Execute the function body.
    Reference ref;
    const auto status = this->m_body.execute_in_place(ref, ctx_next);
    switch(status) {
    case Block::status_next:
      // Return `null` because the control flow reached the end of the function.
      return { };
    case Block::status_break_unspec:
    case Block::status_break_switch:
    case Block::status_break_while:
    case Block::status_break_for:
      ASTERIA_THROW_RUNTIME_ERROR("`break` statements are not allowed outside matching `switch` or loop statements.");
    case Block::status_continue_unspec:
    case Block::status_continue_while:
    case Block::status_continue_for:
      ASTERIA_THROW_RUNTIME_ERROR("`continue` statements are not allowed outside matching loop statements.");
    case Block::status_return:
      // Forward the result reference.
      return std::move(ref);
    default:
      ASTERIA_TERMINATE("An unknown execution result enumeration `", status, "` has been encountered.");
    }
  }

}
