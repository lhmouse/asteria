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
    // variables recursively. It has no effect on children that are
    // not variables, which are always enumerated. Because values are
    // copy-on-write, it is possible for the same (shared) value to
    // be encountered multiple times during garbave collection. So we
    // require the `id_ptr` argument to tell whether a pointer to
    // variable itself has been marked or not.
    virtual bool
    do_process_one(const void* id_ptr, const rcptr<Variable>& var)
      = 0;

  public:
    virtual
    ~Variable_Callback();

    Variable_Callback&
    process(const void* id_ptr, const rcptr<Variable>& var_opt)
      {
        return var_opt && this->do_process_one(id_ptr, var_opt)
                  ? var_opt->get_value().enumerate_variables(*this)
                  : *this;
      }

    template<typename ContainerT>
    Variable_Callback&
    operator()(ContainerT& cont)
      { return cont.enumerate_variables(*this);  }
  };

}  // namespace asteria

#endif
