// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "recursion_sentry.hpp"
#include "util.hpp"

namespace asteria {

void
Recursion_Sentry::
do_throw_stack_overflow(size_t usage, size_t limit)
  const
  {
    ASTERIA_THROW("Stack overflow averted (stack usage `$1` exceeded `$2`)", usage, limit);
  }

}  // namespace asteria
