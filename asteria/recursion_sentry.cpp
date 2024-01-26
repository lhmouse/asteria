// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "recursion_sentry.hpp"
#include "utils.hpp"
namespace asteria {

void
Recursion_Sentry::
do_throw_stack_overflow(ptrdiff_t usage) const
  {
     ::rocket::sprintf_and_throw<::std::invalid_argument>(
           "asteria::Recursion_Sentry: stack overflow averted (`%td` > `%ld`)",
           ::std::abs(usage), 1L << stack_mask_bits);
  }

}  // namespace asteria
