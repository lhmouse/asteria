// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_CALLBACK_HPP_
#define ASTERIA_RUNTIME_VARIABLE_CALLBACK_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Variable_Callback
  {
  public:
    Variable_Callback() noexcept
      {
      }
    virtual ~Variable_Callback();

  public:
    // This is a helper function for `std` algorithms.
    template<typename ContainerT> Variable_Callback& operator()(ContainerT& cont)
      {
        return cont.enumerate_variables(*this);
      }

    // The return value indicates whether to invoke `*this` on child VARIABLES recursively.
    // It has no effect on children that are not variables, which are always enumerated.
    virtual bool process(const rcptr<Variable>& var) const = 0;
  };

}  // namespace Asteria

#endif
