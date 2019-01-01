// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "instantiated_function.hpp"
#include "reference.hpp"
#include "executive_context.hpp"
#include "../syntax/statement.hpp"
#include "../utilities.hpp"

namespace Asteria {

Instantiated_function::~Instantiated_function()
  {
  }

rocket::cow_string Instantiated_function::describe() const
  {
    Formatter fmt;
    ASTERIA_FORMAT(fmt, "function `", this->m_name, "()` defined at \'", this->m_loc, "\'");
    return fmt.extract_string();
  }

void Instantiated_function::invoke(Reference &self_io, Global_context &global, rocket::cow_vector<Reference> &&args) const
  {
    this->m_body_bnd.execute_as_function(self_io, global, this->m_loc, this->m_name, this->m_params, std::move(args));
  }

void Instantiated_function::enumerate_variables(const Abstract_variable_callback &callback) const
  {
    this->m_body_bnd.enumerate_variables(callback);
  }

}
