// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "uninitialized_placeholder.hpp"
#include "../utilities.hpp"

namespace Asteria {

Uninitialized_Placeholder::~Uninitialized_Placeholder()
  {
  }

void Uninitialized_Placeholder::describe(std::ostream& os) const
  {
    os << "<uninitialized placeholder>";
  }

Reference& Uninitialized_Placeholder::invoke(Reference& /*self*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& /*args*/) const
  {
    ASTERIA_THROW_RUNTIME_ERROR("An uninitialized function cannot be called.");
  }

void Uninitialized_Placeholder::enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const
  {
    // There is nothing to do.
  }

}  // namespace Asteria
