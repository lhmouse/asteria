// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFCNT_BASE_HPP_
#define ASTERIA_RUNTIME_REFCNT_BASE_HPP_

#include "../fwd.hpp"
#include "../rocket/refcnt_ptr.hpp"

namespace Asteria {

class RefCnt_Base : public rocket::refcnt_base<RefCnt_Base>
  {
  public:
    RefCnt_Base() noexcept
      {
      }
    RefCnt_Base(const RefCnt_Base &) noexcept
      : RefCnt_Base()
      {
      }
    RefCnt_Base & operator=(const RefCnt_Base &) noexcept
      {
        return *this;
      }
    virtual ~RefCnt_Base();

  public:
    bool unique() const noexcept
      {
        return rocket::refcnt_base<RefCnt_Base>::unique();
      }
    long use_count() const noexcept
      {
        return rocket::refcnt_base<RefCnt_Base>::use_count();
      }
  };

}

#endif
