// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ABSTRACT_OPAQUE_HPP_
#define ASTERIA_ABSTRACT_OPAQUE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Abstract_opaque : public Sbase<Abstract_opaque>
  {
  public:
    Abstract_opaque() noexcept
      {
      }
    virtual ~Abstract_opaque();

  public:
    virtual String describe() const = 0;
    virtual Value * open_member(const String &key, bool create_new) = 0;
    virtual Value unset_member(const String &key) = 0;
  };

}

#endif
