// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "exception.hpp"

namespace Asteria {

Exception::~Exception()
  { }

static_assert(std::is_nothrow_copy_constructible<Exception>::value, "Copy constructors of exceptions are not allow to throw exceptions.");
static_assert(std::is_nothrow_move_constructible<Exception>::value, "Move constructors of exceptions are not allow to throw exceptions.");
static_assert(std::is_nothrow_copy_assignable<Exception>::value, "Copy assignment operators of exceptions are not allow to throw exceptions.");
static_assert(std::is_nothrow_move_assignable<Exception>::value, "Move assignment operators of exceptions are not allow to throw exceptions.");

}  // namespace Asteria
