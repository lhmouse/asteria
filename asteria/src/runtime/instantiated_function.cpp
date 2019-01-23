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
    os << this->m_zvarg.get().get_name() << "("
       << rocket::ostream_implode(this->m_params.begin(), this->m_params.size(), ", ")
       << ") @ " << this->m_zvarg.get().get_location();
  }

void Instantiated_Function::invoke(Reference &self_io, Global_Context &global, CoW_Vector<Reference> &&args) const
  {
    this->m_body_bnd.execute_as_function(self_io, global, this->m_zvarg, this->m_params, std::move(args));
  }

void Instantiated_Function::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    this->m_body_bnd.enumerate_variables(callback);
  }

}
