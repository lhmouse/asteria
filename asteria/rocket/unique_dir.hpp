// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_DIR_HPP_
#define ROCKET_UNIQUE_DIR_HPP_

#include "unique_handle.hpp"
#include <dirent.h>  // ::DIR, ::closedir()

namespace rocket {

class dir_closer
  {
  public:
    using handle_type  = ::DIR*;
    using closer_type  = decltype(::closedir)*;

  private:
    closer_type m_cl;

  public:
    constexpr dir_closer(closer_type cl) noexcept
      :
        m_cl(cl)
      {
      }

  public:
    constexpr operator closer_type () const noexcept
      {
        return this->m_cl;
      }
    int operator()(handle_type dp) const noexcept
      {
        return this->close(dp);
      }

    constexpr handle_type null() const noexcept
      {
        return nullptr;
      }
    constexpr bool is_null(handle_type dp) const noexcept
      {
        return dp == nullptr;
      }
    int close(handle_type dp) const noexcept
      {
        if(this->m_cl)
          return this->m_cl(dp);
        else
          return 0;
      }
  };

using unique_dir  = unique_handle<::DIR*, dir_closer>;

}  // namespace rocket

#endif
