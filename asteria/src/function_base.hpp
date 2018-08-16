// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FUNCTION_BASE_HPP_
#define ASTERIA_FUNCTION_BASE_HPP_

#include "fwd.hpp"

namespace Asteria
{

class Function_base
  {
  public:
    Function_base() noexcept
      {
      }
    virtual ~Function_base();

    Function_base(const Function_base &) = delete;
    Function_base & operator=(const Function_base &) = delete;

  public:
    virtual Cow_string describe() const = 0;
    virtual Reference invoke(Reference &&self, Vector<Reference> &&args) const = 0;
  };

}

#endif
