// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_PTC_ARGUMENTS_
#define ASTERIA_RUNTIME_PTC_ARGUMENTS_

#include "../fwd.hpp"
#include "instantiated_function.hpp"
#include "../llds/reference_stack.hpp"
#include "../source_location.hpp"
namespace asteria {

class PTC_Arguments
  :
    public rcfwd<PTC_Arguments>
  {
  private:
    // These describe characteristics of the function call.
    Source_Location m_sloc;
    PTC_Aware m_ptc;

    // These are the target function and its arguments.
    cow_function m_target;
    Reference m_self;
    Reference_Stack m_stack;

    // These are captured data.
    refcnt_ptr<const Instantiated_Function> m_caller_opt;
    cow_bivector<Source_Location, AVM_Rod> m_defer;

  public:
    PTC_Arguments(const Source_Location& sloc, PTC_Aware ptc, const cow_function& target,
                  Reference&& self, Reference_Stack&& stack)
      :
        m_sloc(sloc), m_ptc(ptc), m_target(target),
        m_self(move(self)), m_stack(move(stack))
      { }

  public:
    PTC_Arguments(const PTC_Arguments&) = delete;
    PTC_Arguments& operator=(const PTC_Arguments&) & = delete;
    virtual ~PTC_Arguments();

    // accessors
    const Source_Location&
    sloc()
      const noexcept
      { return this->m_sloc;  }

    PTC_Aware
    ptc_aware()
      const noexcept
      { return this->m_ptc;  }

    const cow_function&
    target()
      const noexcept
      { return this->m_target;  }

    const Reference_Stack&
    stack()
      const noexcept
      { return this->m_stack;  }

    Reference_Stack&
    mut_stack()
      noexcept
      { return this->m_stack;  }

    const Reference&
    self()
      const noexcept
      { return this->m_self;  }

    Reference&
    mut_self()
      noexcept
      { return this->m_self;  }

    refcnt_ptr<const Instantiated_Function>
    caller_opt()
      const noexcept
      { return this->m_caller_opt;  }

    void
    set_caller(const refcnt_ptr<const Instantiated_Function>& caller)
      noexcept
      { this->m_caller_opt = caller;  }

    const cow_bivector<Source_Location, AVM_Rod>&
    defer()
      const noexcept
      { return this->m_defer;  }

    cow_bivector<Source_Location, AVM_Rod>&
    mut_defer()
      noexcept
      { return this->m_defer;  }
  };

}  // namespace asteria
#endif
