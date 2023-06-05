// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYBUF_BASE_
#define ROCKET_TINYBUF_BASE_

#include "fwd.hpp"
namespace rocket {

struct tinybuf_base
  {
    enum seek_dir : uint8_t
      {
        seek_set  = SEEK_SET,
        seek_cur  = SEEK_CUR,
        seek_end  = SEEK_END,
      };

    enum open_mode : uint32_t
      {
        open_no_access   = 0b000000000000,  // 0
        open_read        = 0b000000000001,  // O_RDONLY
        open_write       = 0b000000000010,  // O_WRONLY
        open_read_write  = 0b000000000011,  // O_RDWR
        open_append      = 0b000000000100,  // O_APPEND
        open_binary      = 0b000000001000,  // _O_BINARY (Windows only)
        open_create      = 0b000000010000,  // O_CREAT
        open_truncate    = 0b000000100000,  // O_TRUNC
        open_exclusive   = 0b000001000000,  // O_EXCL
      };
  };

constexpr
tinybuf_base::open_mode
operator~(tinybuf_base::open_mode rhs) noexcept
  {
    return static_cast<tinybuf_base::open_mode>(
               ~ static_cast<uint32_t>(rhs));
  }

constexpr
tinybuf_base::open_mode
operator&(tinybuf_base::open_mode lhs, tinybuf_base::open_mode rhs) noexcept
  {
    return static_cast<tinybuf_base::open_mode>(
               static_cast<uint32_t>(lhs)
               & static_cast<uint32_t>(rhs));
  }

constexpr
tinybuf_base::open_mode
operator|(tinybuf_base::open_mode lhs, tinybuf_base::open_mode rhs) noexcept
  {
    return static_cast<tinybuf_base::open_mode>(
               static_cast<uint32_t>(lhs)
               | static_cast<uint32_t>(rhs));
  }

constexpr
tinybuf_base::open_mode
operator^(tinybuf_base::open_mode lhs, tinybuf_base::open_mode rhs) noexcept
  {
    return static_cast<tinybuf_base::open_mode>(
               static_cast<uint32_t>(lhs)
               ^ static_cast<uint32_t>(rhs));
  }

}  // namespace rocket
#endif
