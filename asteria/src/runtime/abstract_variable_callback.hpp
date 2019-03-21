// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_VARIABLE_CALLBACK_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_VARIABLE_CALLBACK_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Abstract_Variable_Callback
  {
  public:
    Abstract_Variable_Callback() noexcept
      {
      }
    virtual ~Abstract_Variable_Callback();

  public:
    virtual bool operator()(const Rcptr<Variable>& var) const = 0;
  };

}  // namespace Asteria

#endif
