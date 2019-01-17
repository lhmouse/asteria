// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_OPAQUE_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_OPAQUE_HPP_

#include "../fwd.hpp"
#include "refcnt_base.hpp"

namespace Asteria {

class Abstract_Opaque : public RefCnt_Base
  {
  public:
    Abstract_Opaque() noexcept
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Abstract_Opaque, virtual);

  public:
    virtual Cow_String describe() const = 0;
    virtual Abstract_Opaque * clone(RefCnt_Ptr<Abstract_Opaque> &value_out) const = 0;
    virtual void enumerate_variables(const Abstract_Variable_Callback &callback) const = 0;
  };

}

#endif
