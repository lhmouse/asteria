// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_CALLBACK_HPP_
#define ASTERIA_RUNTIME_VARIABLE_CALLBACK_HPP_

#include "../fwd.hpp"
#include "variable.hpp"

namespace asteria {

class Variable_Callback
  {
  protected:
    // The return value indicates whether to invoke `*this` on child
    // variables/ recursively. It has no effect on children that are
    // not variables, which are always enumerated.
    virtual bool
    do_process_one(const rcptr<Variable>& var)
      = 0;

  public:
    virtual
    ~Variable_Callback();

    Variable_Callback&
    process(const rcptr<Variable>& var)
      {
        const auto& value = var->get_value();
        if(this->do_process_one(var) && !value.is_scalar())
          value.enumerate_variables(*this);
        return *this;
      }

    template<typename ContainerT>
    Variable_Callback&
    operator()(ContainerT& cont)
      { return cont.enumerate_variables(*this);  }
  };

}  // namespace asteria

#endif
