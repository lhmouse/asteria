// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "recursion_sentry.hpp"
#include "utils.hpp"
namespace asteria {

void
Recursion_Sentry::
do_throw_stack_overflow(ptrdiff_t usage, int limit) const
  {
     ::rocket::sprintf_and_throw<::std::invalid_argument>(
           "do_throw_stack_overflow: stack overflow averted (`%td` > `%d`)",
           usage, limit);
  }

}  // namespace asteria
