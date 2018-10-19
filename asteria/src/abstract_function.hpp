// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ABSTRACT_FUNCTION_HPP_
#define ASTERIA_ABSTRACT_FUNCTION_HPP_

#include "fwd.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

class Abstract_function : public rocket::refcounted_base<Abstract_function>
  {
  public:
    Abstract_function() noexcept
      {
      }
    virtual ~Abstract_function();

  public:
    virtual String describe() const = 0;
    virtual void enumerate_variables(const Abstract_variable_callback &callback) const = 0;

    virtual Reference invoke(Global_context &global, Reference self, Vector<Reference> args) const = 0;
  };

}

#endif
