// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ABSTRACT_VARIABLE_CALLBACK_HPP_
#define ASTERIA_ABSTRACT_VARIABLE_CALLBACK_HPP_

#include "fwd.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

class Abstract_variable_callback
  {
  public:
    Abstract_variable_callback() noexcept
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Abstract_variable_callback, virtual);

  public:
    virtual bool accept(const rocket::refcounted_ptr<Variable> &var) const = 0;
  };

}

#endif
