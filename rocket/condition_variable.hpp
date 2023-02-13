// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_CONDITION_VARIABLE_
#define ROCKET_CONDITION_VARIABLE_

#include "fwd.hpp"
#include "assert.hpp"
#include <condition_variable>
namespace rocket {

struct condition_variable : ::std::condition_variable
  {
    using unique_lock = ::std::unique_lock<condition_variable>;
    using lock_guard = ::std::lock_guard<condition_variable>;
  };

}  // namespace rocket
#endif
