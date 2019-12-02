// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "runtime_error.hpp"

namespace Asteria {

Runtime_Error::~Runtime_Error()
  {
  }

static_assert(rocket::conjunction<std::is_nothrow_copy_constructible<Runtime_Error>,
                                  std::is_nothrow_move_constructible<Runtime_Error>,
                                  std::is_nothrow_copy_assignable<Runtime_Error>,
                                  std::is_nothrow_move_assignable<Runtime_Error>>::value, "");

}  // namespace Asteria
