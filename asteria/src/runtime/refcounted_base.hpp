// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFCOUNTED_BASE_HPP_
#define ASTERIA_RUNTIME_REFCOUNTED_BASE_HPP_

#include "../fwd.hpp"
#include "../rocket/refcounted_ptr.hpp"

namespace Asteria {

class Refcounted_base : public rocket::refcounted_base<Refcounted_base>
  {
  public:
    Refcounted_base() noexcept
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Refcounted_base, virtual);

  public:
    // No public member functions.
  };

}

#endif
