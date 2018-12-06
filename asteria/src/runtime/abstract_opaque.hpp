// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_OPAQUE_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_OPAQUE_HPP_

#include "../fwd.hpp"
#include "../rocket/refcounted_ptr.hpp"

namespace Asteria {

class Abstract_opaque : public rocket::refcounted_base<Abstract_opaque>
  {
  public:
    Abstract_opaque() noexcept
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Abstract_opaque, virtual);

  public:
    virtual rocket::cow_string describe() const = 0;
    virtual Abstract_opaque * clone(rocket::refcounted_ptr<Abstract_opaque> &value_out) const = 0;
    virtual void enumerate_variables(const Abstract_variable_callback &callback) const = 0;
  };

}

#endif
