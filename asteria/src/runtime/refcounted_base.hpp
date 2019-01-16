// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFCOUNTED_BASE_HPP_
#define ASTERIA_RUNTIME_REFCOUNTED_BASE_HPP_

#include "../fwd.hpp"
#include "../rocket/refcounted_ptr.hpp"

namespace Asteria {

class Refcounted_Base : public rocket::refcounted_base<Refcounted_Base>
  {
  public:
    Refcounted_Base() noexcept
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Refcounted_Base, virtual);

  public:
    // No public member functions.
  };

}

#endif
