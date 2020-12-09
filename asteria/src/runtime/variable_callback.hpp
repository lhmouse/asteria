// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_CALLBACK_HPP_
#define ASTERIA_RUNTIME_VARIABLE_CALLBACK_HPP_

#include "../fwd.hpp"
#include "variable.hpp"

namespace asteria {

class Variable_Callback
  {
  protected:
    // The return value indicates whether to invoke `*this` on child
    // variables recursively. It has no effect on children that are not
    // variables, which are always enumerated.
    virtual
    bool
    do_process_one(const rcptr<Variable>& var)
      = 0;

  public:
    virtual
    ~Variable_Callback();

    Variable_Callback&
    process(const rcptr<Variable>& var)
      {
        if(this->do_process_one(var))
          var->enumerate_variables_descent(*this);
        return *this;
      }

    template<typename ContainerT>
    Variable_Callback&
    operator()(ContainerT& cont)
      { return cont.enumerate_variables(*this);  }
  };

}  // namespace asteria

#endif
