// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ONCE_FLAG_
#  error Please include <rocket/once_flag.hpp> instead.
#endif

namespace details_once_flag {

// Note these conforms to Itanium ABI.
// See <https://itanium-cxx-abi.github.io/cxx-abi/abi.html> for details.
// On ARM EABI only 32 bits are required. We reserve 64 bits always.

union guard
  {
    uint8_t bytes[8];
    uint64_t word;
  };

extern "C"
int
__cxa_guard_acquire(guard* g) noexcept;

extern "C"
void
__cxa_guard_abort(guard* g) noexcept;

extern "C"
void
__cxa_guard_release(guard* g) noexcept;

}  // namespace details_once_flag
