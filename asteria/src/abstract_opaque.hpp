// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ABSTRACT_OPAQUE_HPP_
#define ASTERIA_ABSTRACT_OPAQUE_HPP_

#include "fwd.hpp"

namespace Asteria
{

class Abstract_opaque
  {
  public:
    Abstract_opaque() noexcept
      {
      }
    virtual ~Abstract_opaque();

    Abstract_opaque(const Abstract_opaque &) = delete;
    Abstract_opaque & operator=(const Abstract_opaque &) = delete;

  public:
    virtual String describe() const = 0;
  };

}

#endif
