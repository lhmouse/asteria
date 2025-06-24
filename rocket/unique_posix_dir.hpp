// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_POSIX_DIR_
#define ROCKET_UNIQUE_POSIX_DIR_

#include "unique_handle.hpp"
#include <dirent.h>  // ::DIR, ::closedir()
namespace rocket {

struct posix_dir_closer
  {
    using handle_type = ::DIR*;

    constexpr
    bool
    is_null(handle_type dp) const noexcept
      { return dp == nullptr;  }

    constexpr
    handle_type
    null() const noexcept
      { return nullptr;  }

    void
    close(handle_type dp) const noexcept
      {
        if(dp)
          ::closedir(dp);
      }
  };

using unique_posix_dir  = unique_handle<::DIR*, posix_dir_closer>;

}  // namespace rocket
#endif
