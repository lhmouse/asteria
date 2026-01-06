// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_POSIX_FILE_
#define ROCKET_UNIQUE_POSIX_FILE_

#include "unique_handle.hpp"
#include <stdio.h>  // ::FILE, ::fclose()
namespace rocket {

struct posix_file_closer
  {
    using handle_type = ::FILE*;

    constexpr
    bool
    is_null(handle_type fp)
      const noexcept
      { return fp == nullptr;  }

    constexpr
    handle_type
    null()
      const noexcept
      { return nullptr;  }

    void
    close(handle_type fp)
      const noexcept
      {
        if((fp != stdin) && (fp != stdout) && (fp != stderr))
          ::fclose(fp);
      }
  };

using unique_posix_file  = unique_handle<::FILE*, posix_file_closer>;

}  // namespace rocket
#endif
