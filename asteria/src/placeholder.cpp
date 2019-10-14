// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "placeholder.hpp"
#include "utilities.hpp"

namespace Asteria {

Placeholder::~Placeholder()
  { }

tinyfmt& Placeholder::describe(tinyfmt& fmt) const
  {
    return fmt << "<placeholder for uninitialized values>";
  }

Reference& Placeholder::invoke(Reference& /*self*/, const Global_Context& /*global*/, cow_vector<Reference>&& /*args*/) const
  {
    ASTERIA_THROW_RUNTIME_ERROR("An uninitialized function cannot be called.");
  }

Variable_Callback& Placeholder::enumerate_variables(Variable_Callback& callback) const
  {
    return callback;
  }

}  // namespace Asteria
