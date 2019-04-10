// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "placeholder.hpp"
#include "../utilities.hpp"

namespace Asteria {

Placeholder::~Placeholder()
  {
  }

void Placeholder::describe(std::ostream& os) const
  {
    os << "<placeholder for uninitialized values>";
  }

Reference& Placeholder::invoke(Reference& /*self*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& /*args*/) const
  {
    ASTERIA_THROW_RUNTIME_ERROR("An uninitialized function cannot be called.");
  }

void Placeholder::enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const
  {
    // There is nothing to do.
  }

}  // namespace Asteria
