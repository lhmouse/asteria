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
    ROCKET_COPYABLE_DESTRUCTOR(RefCnt_Base, virtual);

  public:
    // No public member functions.
  };

}

#endif
