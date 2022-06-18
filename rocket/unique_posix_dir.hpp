// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_POSIX_DIR_
#define ROCKET_UNIQUE_POSIX_DIR_

#include "unique_handle.hpp"
#include <dirent.h>  // ::DIR, ::closedir()

namespace rocket {

class posix_dir_closer
  {
  public:
    using handle_type  = ::DIR*;
    using closer_type  = int (*)(::DIR*);  // ::closedir()

  private:
    closer_type m_cl;

  public:
    constexpr
    posix_dir_closer() noexcept
      : m_cl(::closedir)
      { }

    constexpr
    posix_dir_closer(closer_type cl) noexcept
      : m_cl(cl)
      { }

  public:
    constexpr operator
    closer_type() const noexcept
      { return this->m_cl;  }

    int
    operator()(handle_type dp) const noexcept
      { return this->close(dp);  }

    constexpr
    handle_type
    null() const noexcept
      { return nullptr;  }

    constexpr
    bool
    is_null(handle_type dp) const noexcept
      { return dp == nullptr;  }

    int
    close(handle_type dp) const noexcept
      {
        if(!this->m_cl)
          return 0;  // no close
        else
          return this->m_cl(dp);
      }
  };

using unique_posix_dir  = unique_handle<::DIR*, posix_dir_closer>;

}  // namespace rocket

#endif
