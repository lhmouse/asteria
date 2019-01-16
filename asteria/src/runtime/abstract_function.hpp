// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_FUNCTION_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_FUNCTION_HPP_

#include "../fwd.hpp"
#include "refcounted_base.hpp"
#include "../rocket/cow_string.hpp"
#include "../rocket/cow_vector.hpp"

namespace Asteria {

class Abstract_Function : public Refcounted_Base
  {
  public:
    Abstract_Function() noexcept
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Abstract_Function, virtual);

  public:
    virtual rocket::cow_string describe() const = 0;
    virtual void invoke(Reference &self_io, Global_Context &global, rocket::cow_vector<Reference> &&args) const = 0;
    virtual void enumerate_variables(const Abstract_Variable_Callback &callback) const = 0;
  };

}

#endif
