// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_RECURSIVE_MUTEX_
#define ROCKET_RECURSIVE_MUTEX_

#include "fwd.hpp"
#include "assert.hpp"
#include <mutex>
namespace rocket {

struct recursive_mutex : ::std::recursive_mutex
  {
    using unique_lock = ::std::unique_lock<recursive_mutex>;
    using lock_guard = ::std::lock_guard<recursive_mutex>;
  };

}  // namespace rocket

#endif
