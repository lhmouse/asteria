// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "recursion_sentry.hpp"
#include "utils.hpp"
namespace asteria {

void
Recursion_Sentry::
do_throw_overflow(ptrdiff_t usage, int limit)
  const
  {
     ::rocket::sprintf_and_throw<::std::invalid_argument>(
           "do_throw_overflow: stack overflow averted (`%td` > `%d`)",
           ::std::abs(usage), limit);
  }

}  // namespace asteria
