// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "global_context.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Global_context::~Global_context()
  {
  }

bool Global_context::is_analytic() const noexcept
  {
    return false;
  }
const Abstract_context * Global_context::get_parent_opt() const noexcept
  {
    return nullptr;
  }

rocket::refcounted_ptr<Variable> Global_context::create_tracked_variable()
  {
    return rocket::make_refcounted<Variable>();
  }

}
