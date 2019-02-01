// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "instantiated_function.hpp"
#include "reference.hpp"
#include "executive_context.hpp"
#include "../syntax/statement.hpp"
#include "../utilities.hpp"

namespace Asteria {

Instantiated_Function::~Instantiated_Function()
  {
  }

void Instantiated_Function::describe(std::ostream &os) const
  {
    os << this->m_zvarg->get_function_signature() << " @ " << this->m_zvarg->get_source_location();
  }

void Instantiated_Function::invoke(Reference &self_io, const Global_Context &global, CoW_Vector<Reference> &&args) const
  {
    this->m_body_bnd.execute_as_function(self_io, this->m_zvarg, this->m_params, global, std::move(args));
  }

void Instantiated_Function::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    this->m_body_bnd.enumerate_variables(callback);
  }

}
