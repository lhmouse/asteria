// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_FUNCTION_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_FUNCTION_HPP_

#include "../fwd.hpp"
#include "refcnt_base.hpp"

namespace Asteria {

class Abstract_Function : public virtual RefCnt_Base
  {
  public:
    Abstract_Function() noexcept
      {
      }
    ~Abstract_Function() override;

  public:
    virtual void describe(std::ostream &os) const = 0;
    virtual void invoke(Reference &self_io, const Global_Context &global, CoW_Vector<Reference> &&args) const = 0;
    virtual void enumerate_variables(const Abstract_Variable_Callback &callback) const = 0;
  };

inline std::ostream & operator<<(std::ostream &os, const Abstract_Function &func)
  {
    func.describe(os);
    return os;
  }

}

#endif
