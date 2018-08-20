// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ABSTRACT_FUNCTION_HPP_
#define ASTERIA_ABSTRACT_FUNCTION_HPP_

#include "fwd.hpp"
#include "abstract_opaque.hpp"

namespace Asteria
{

class Abstract_function
  : public Abstract_opaque
  {
  public:
    Abstract_function() noexcept
      {
      }
    ~Abstract_function();

  public:
    virtual String describe() const = 0;
    virtual Reference invoke(Reference self, Vector<Reference> args) const = 0;
  };

}

#endif
