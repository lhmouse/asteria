// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "simple_binding_wrapper.hpp"
#include "../runtime/reference.hpp"
#include "../utilities.hpp"

namespace Asteria {

Simple_Binding_Wrapper::~Simple_Binding_Wrapper()
  {
  }

void Simple_Binding_Wrapper::describe(std::ostream &os) const
  {
    os << this->m_desc;
  }

void Simple_Binding_Wrapper::invoke(Reference &self_io, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) const
  {
    self_io = (*(this->m_sfunc))(this->m_iparam, this->m_pparam, std::move(args));
  }

void Simple_Binding_Wrapper::enumerate_variables(const Abstract_Variable_Callback & /*callback*/) const
  {
    // There is no variable to enumerate.
  }

}
