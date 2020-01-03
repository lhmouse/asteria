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

tinyfmt& Simple_Binding_Wrapper::describe(tinyfmt& fmt) const
  {
    return fmt << this->m_desc;
  }

Reference& Simple_Binding_Wrapper::invoke(Reference& self, Global& global, cow_vector<Reference>&& args) const
  {
    return self = (*(this->m_proc))(::rocket::move(args), ::rocket::move(self), global, this->m_pval);
  }

Variable_Callback& Simple_Binding_Wrapper::enumerate_variables(Variable_Callback& callback) const
  {
    return this->m_pval.enumerate_variables(callback);
  }

}  // namespace Asteria
