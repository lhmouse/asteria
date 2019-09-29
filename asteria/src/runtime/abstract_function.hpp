// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_FUNCTION_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_FUNCTION_HPP_

#include "../fwd.hpp"
#include "rcbase.hpp"

namespace Asteria {

class Abstract_Function : public virtual Rcbase
  {
  public:
    Abstract_Function() noexcept
      {
      }
    ~Abstract_Function() override;

  public:
    virtual tinyfmt& describe(tinyfmt& fmt) const = 0;
    virtual Reference& invoke(Reference& self, const Global_Context& global, cow_vector<Reference>&& args) const = 0;
    virtual Variable_Callback& enumerate_variables(Variable_Callback& callback) const = 0;
  };

inline tinyfmt& operator<<(tinyfmt& fmt, const Abstract_Function& func)
  {
    return func.describe(fmt);
  }

}  // namespace Asteria

#endif
