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

void Simple_Binding_Wrapper::describe(std::ostream& os) const
  {
    os << this->m_desc;
  }

void Simple_Binding_Wrapper::invoke(Reference& self, const Global_Context& global, Cow_Vector<Reference>&& args) const
  {
    self = (*(this->m_fptr))(this->m_opaque, global, rocket::move(args));
  }

void Simple_Binding_Wrapper::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    rocket::for_each(this->m_opaque, [&](const Value& p) { p.enumerate_variables(callback);  });
  }

}  // namespace Asteria
