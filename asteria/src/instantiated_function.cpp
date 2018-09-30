// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "instantiated_function.hpp"
#include "reference.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
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

Reference Instantiated_function::invoke(Global_context *global_opt, Reference self, Vector<Reference> args) const
  {
    Executive_context ctx_next;
    ctx_next.initialize_for_function(this->m_params, this->m_file, this->m_line, std::move(self), std::move(std::move(args)));
    return this->m_body.execute_as_function_in_place(ctx_next, global_opt);
  }

}
