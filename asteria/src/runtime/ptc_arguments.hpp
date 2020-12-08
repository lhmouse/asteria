// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_PTC_ARGUMENTS_HPP_
#define ASTERIA_RUNTIME_PTC_ARGUMENTS_HPP_

#include "../fwd.hpp"
#include "../llds/reference_stack.hpp"
#include "../source_location.hpp"

namespace asteria {

class PTC_Arguments
  final
  : public Rcfwd<PTC_Arguments>
  {
  public:
    struct Caller
      {
        Source_Location sloc;
        cow_string func;
      };

  private:
    // These describe characteristics of the function call.
    Source_Location m_sloc;
    PTC_Aware m_ptc;

    // These are the target function and its arguments.
    cow_function m_target;
    Reference_Stack m_stack;

    // These are captured data.
    opt<Caller> m_caller;
    cow_bivector<Source_Location, AVMC_Queue> m_defer;

  public:
    PTC_Arguments(const Source_Location& sloc, PTC_Aware ptc,
                  const cow_function& target, Reference_Stack&& stack)
      : m_sloc(sloc), m_ptc(ptc),
        m_target(target), m_stack(::std::move(stack))
      { }

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(PTC_Arguments);

    const Source_Location&
    sloc()
      const noexcept
      { return this->m_sloc;  }

    PTC_Aware
    ptc_aware()
      const noexcept
      { return this->m_ptc;  }

    const cow_function&
    get_target()
      const noexcept
      { return this->m_target;  }

    const Reference_Stack&
    get_stack()
      const noexcept
      { return this->m_stack;  }

    Reference_Stack&
    open_stack()
      noexcept
      { return this->m_stack;  }

    const Caller*
    caller_opt()
      const noexcept
      { return this->m_caller.value_ptr();  }

    PTC_Arguments&
    set_caller(Caller&& caller)
      noexcept
      { return this->m_caller = ::std::move(caller), *this;  }

    PTC_Arguments&
    clear_caller()
      noexcept
      { return this->m_caller.reset(), *this;  }

    const cow_bivector<Source_Location, AVMC_Queue>&
    get_defer()
      const noexcept
      { return this->m_defer;  }

    cow_bivector<Source_Location, AVMC_Queue>&
    open_defer()
      noexcept
      { return this->m_defer;  }
  };

}  // namespace asteria

#endif
