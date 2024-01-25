// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIADIC_ARGUER_
#define ASTERIA_RUNTIME_VARIADIC_ARGUER_

#include "../fwd.hpp"
#include "reference.hpp"
#include "instantiated_function.hpp"
#include "../source_location.hpp"
namespace asteria {

class Variadic_Arguer
  :
    public rcfwd<Variadic_Arguer>,
    public Abstract_Function
  {
  private:
    Source_Location m_sloc;
    cow_string m_func;
    cow_vector<Reference> m_vargs;

  public:
    Variadic_Arguer(const Instantiated_Function& xfunc, const cow_vector<Reference>& xvargs)
      :
        m_sloc(xfunc.sloc()), m_func(xfunc.func()), m_vargs(xvargs)
      { }

  public:
    const Source_Location&
    sloc() const noexcept
      { return this->m_sloc;  }

    const cow_string&
    func() const noexcept
      { return this->m_func;  }

    bool
    empty() const noexcept
      { return this->m_vargs.empty();  }

    size_t
    size() const noexcept
      { return this->m_vargs.size();  }

    const Reference&
    arg(size_t index) const
      { return this->m_vargs.at(index);  }

    tinyfmt&
    describe(tinyfmt& fmt) const override;

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const override;

    Reference&
    invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const override;
  };

}  // namespace asteria
#endif
