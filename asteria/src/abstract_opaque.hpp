// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ABSTRACT_OPAQUE_HPP_
#define ASTERIA_ABSTRACT_OPAQUE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Abstract_Opaque : public Rcfwd<Abstract_Opaque>
  {
  public:
    Abstract_Opaque() noexcept
      {
      }
    ~Abstract_Opaque() override;

  public:
    virtual tinyfmt& describe(tinyfmt& fmt) const = 0;
    virtual Variable_Callback& enumerate_variables(Variable_Callback& callback) const = 0;

    // This function is called when a mutable reference is requested and the current instance
    // is shared. If this function returns a null pointer, the shared instance is used. If this
    // function returns a non-null pointer, it replaces the current value. Derived classes that
    // are not copyable should throw an exception in this function.
    virtual Abstract_Opaque* clone_opt(rcptr<Abstract_Opaque>& output) const = 0;
  };

inline tinyfmt& operator<<(tinyfmt& fmt, const Abstract_Opaque& opaq)
  {
    return opaq.describe(fmt);
  }

}  // namespace Asteria

#endif
