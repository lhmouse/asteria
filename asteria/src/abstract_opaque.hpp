// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ABSTRACT_OPAQUE_HPP_
#define ASTERIA_ABSTRACT_OPAQUE_HPP_

#include "fwd.hpp"
#include "rcbase.hpp"

namespace Asteria {

class Abstract_Opaque : public virtual Rcbase
  {
  public:
    Abstract_Opaque() noexcept
      { }
    ~Abstract_Opaque() override;

  public:
    virtual tinyfmt& describe(tinyfmt& fmt) const = 0;
    virtual Variable_Callback& enumerate_variables(Variable_Callback& callback) const = 0;
  };

inline tinyfmt& operator<<(tinyfmt& fmt, const Abstract_Opaque& opaque)
  {
    return opaque.describe(fmt);
  }

}  // namespace Asteria

#endif
