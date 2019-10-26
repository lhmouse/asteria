// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_SYSFILE_HPP_
#define ROCKET_UNIQUE_SYSFILE_HPP_

#include "unique_handle.hpp"
#include <unistd.h>  // ::close()

namespace rocket {

class sysfile_closer
  {
  public:
    using handle_type  = int;
    using closer_type  = decltype(::close)*;

  private:
    closer_type m_cl;

  public:
    constexpr sysfile_closer(closer_type cl) noexcept
      :
        m_cl(cl)
      {
      }

  public:
    constexpr operator closer_type () const noexcept
      {
        return this->m_cl;
      }
    int operator()(handle_type fd) const noexcept
      {
        return this->close(fd);
      }

    constexpr handle_type null() const noexcept
      {
        return -1;
      }
    constexpr bool is_null(handle_type fd) const noexcept
      {
        return fd < 0;
      }
    int close(handle_type fd) const noexcept
      {
        if(this->m_cl)
          return this->m_cl(fd);
        else
          return 0;
      }
  };

using unique_posix_fd  = unique_handle<int, sysfile_closer>;

}  // namespace rocket

#endif
