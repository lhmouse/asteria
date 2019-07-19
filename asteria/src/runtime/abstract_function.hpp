// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_FUNCTION_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_FUNCTION_HPP_

#include "../fwd.hpp"
#include "rcbase.hpp"

namespace Asteria {

class Abstract_Function : public virtual Rcbase
  {
  protected:
    Abstract_Function() noexcept
      {
      }
    ~Abstract_Function() override;

  public:
    virtual std::ostream& describe(std::ostream& os) const = 0;
    virtual Reference& invoke(Reference& self, const Global_Context& global, Cow_Vector<Reference>&& args) const = 0;
    virtual void enumerate_variables(const Abstract_Variable_Callback& callback) const = 0;
  };

inline std::ostream& operator<<(std::ostream& os, const Abstract_Function& func)
  {
    return func.describe(os);
  }

}  // namespace Asteria

#endif
