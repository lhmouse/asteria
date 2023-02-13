// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_MUTEX_
#define ROCKET_MUTEX_

#include "fwd.hpp"
#include "assert.hpp"
#include <mutex>
namespace rocket {

struct mutex : ::std::mutex
  {
    using unique_lock = ::std::unique_lock<mutex>;
    using lock_guard = ::std::lock_guard<mutex>;
  };

}  // namespace rocket

#endif
