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
    return ASTERIA_FORMAT_STRING("function `", this->m_head, "` at \'", this->m_head.get_location(), "\'");
  }

void Instantiated_function::enumerate_variables(const Abstract_variable_callback &callback) const
  {
    this->m_body_bnd.enumerate_variables(callback);
  }

Reference Instantiated_function::invoke(Global_context &global, Reference self, Vector<Reference> args) const
  {
    return this->m_body_bnd.execute_as_function(global, this->m_head, &(this->m_zvarg), std::move(self), std::move(args));
  }

}
