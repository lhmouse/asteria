// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_POSIX_FILE_
#define ROCKET_UNIQUE_POSIX_FILE_

#include "unique_handle.hpp"
#include <stdio.h>  // ::FILE, ::fclose()
namespace rocket {

class posix_file_closer
  {
  public:
    using handle_type  = ::FILE*;
    using closer_type  = int (*)(::FILE*);  // ::fclose()

  private:
    closer_type m_cl;

  public:
    constexpr posix_file_closer() noexcept
      :
        m_cl(::fclose)
      { }

    constexpr posix_file_closer(closer_type cl) noexcept
      :
        m_cl(cl)
      { }

  public:
    constexpr
    bool
    is_null(handle_type fp) const noexcept
      { return fp == nullptr;  }

    constexpr
    handle_type
    null() const noexcept
      { return nullptr;  }

    int
    close(handle_type fp) const noexcept
      { return this->m_cl ? this->m_cl(fp) : 0;  }
  };

using unique_posix_file  = unique_handle<::FILE*, posix_file_closer>;

}  // namespace rocket
#endif
