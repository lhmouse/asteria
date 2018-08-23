// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ABSTRACT_OPAQUE_HPP_
#define ASTERIA_ABSTRACT_OPAQUE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Abstract_opaque
  {
  public:
    Abstract_opaque() noexcept
      {
      }
    virtual ~Abstract_opaque();

  public:
    virtual String describe() const = 0;
  };

}

#endif
