// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASCII_CASE_
#  error Please include <rocket/ascii_case.hpp> instead.
#endif
namespace details_ascii_case {

constexpr uint8_t case_table[128] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0,
  };

constexpr
uint32_t
cmask_of(char c) noexcept
  {
    return (uint8_t(c) < 128) ? case_table[uint8_t(c)] : uint32_t(0);
  }

constexpr
uint32_t
to_upper(char c) noexcept
  {
    return uint8_t(c) & ~((cmask_of(c) & uint32_t(2)) << 4);
  }

constexpr
uint32_t
to_lower(char c) noexcept
  {
    return uint8_t(c) |  ((cmask_of(c) & uint32_t(1)) << 5);
  }

}  // namespace details_mutex
