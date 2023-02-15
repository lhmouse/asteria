// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_POSIX_FD_
#define ROCKET_UNIQUE_POSIX_FD_

#include "unique_handle.hpp"
#include <unistd.h>  // ::close()
namespace rocket {

class posix_fd_closer
  {
  public:
    using handle_type  = int;
    using closer_type  = int (*)(int);  // ::close()

  private:
    closer_type m_cl;

  public:
    constexpr
    posix_fd_closer() noexcept
      : m_cl(::close)  { }

    constexpr
    posix_fd_closer(closer_type cl) noexcept
      : m_cl(cl)  { }

  public:
    constexpr operator
    closer_type() const noexcept
      { return this->m_cl;  }

    int
    operator()(handle_type fd) const noexcept
      { return this->close(fd);  }

    constexpr
    handle_type
    null() const noexcept
      { return -1;  }

    constexpr
    bool
    is_null(handle_type fd) const noexcept
      { return fd == -1;  }

    int
    close(handle_type fd) const noexcept
      {
        if(!this->m_cl)
          return 0;  // no close
        else
          return this->m_cl(fd);
      }
  };

using unique_posix_fd  = unique_handle<int, posix_fd_closer>;

}  // namespace rocket
#endif
