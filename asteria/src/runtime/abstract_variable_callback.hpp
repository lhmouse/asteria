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
    ROCKET_COPYABLE_DESTRUCTOR(Abstract_Variable_Callback, virtual);

  public:
    virtual bool operator()(const RefCnt_Ptr<Variable> &var) const = 0;
  };

}

#endif
