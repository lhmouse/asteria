// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_POSIX_FD_
#define ROCKET_UNIQUE_POSIX_FD_

#include "unique_handle.hpp"
#include <unistd.h>  // ::close()
namespace rocket {

struct posix_fd_closer
  {
    using handle_type = int;

    constexpr
    bool
    is_null(handle_type fd)
      const noexcept
      { return fd == -1;  }

    constexpr
    handle_type
    null()
      const noexcept
      { return -1;  }

    void
    close(handle_type fd)
      const noexcept
      {
        if(fd > STDERR_FILENO)
          ::close(fd);
      }
  };

using unique_posix_fd  = unique_handle<int, posix_fd_closer>;

}  // namespace rocket
#endif
