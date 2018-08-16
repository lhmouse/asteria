// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_OPAQUE_BASE_HPP_
#define ASTERIA_OPAQUE_BASE_HPP_

#include "fwd.hpp"

namespace Asteria
{

class Opaque_base
  {
  public:
    Opaque_base() noexcept
      {
      }
    virtual ~Opaque_base();

    Opaque_base(const Opaque_base &) = delete;
    Opaque_base & operator=(const Opaque_base &) = delete;

  public:
    virtual String describe() const = 0;
  };

}

#endif
