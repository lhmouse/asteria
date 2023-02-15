// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIADIC_ARGUER_
#define ASTERIA_RUNTIME_VARIADIC_ARGUER_

#include "../fwd.hpp"
#include "reference.hpp"
#include "../source_location.hpp"
namespace asteria {

class Variadic_Arguer final
  : public rcfwd<Variadic_Arguer>,
    public Abstract_Function
  {
  private:
    Source_Location m_sloc;
    cow_string m_func;
    cow_vector<Reference> m_vargs;

  public:
    explicit
    Variadic_Arguer(const Source_Location& xsloc, stringR xfunc)
      : m_sloc(xsloc), m_func(xfunc)  { }

    explicit
    Variadic_Arguer(const Variadic_Arguer& other, const cow_vector<Reference>& xvargs)
      : m_sloc(other.m_sloc), m_func(other.m_func), m_vargs(xvargs)  { }

  public:
    const Source_Location&
    sloc() const noexcept
      { return this->m_sloc;  }

    const cow_string&
    file() const noexcept
      { return this->m_sloc.file();  }

    int
    line() const noexcept
      { return this->m_sloc.line();  }

    int
    column() const noexcept
      { return this->m_sloc.column();  }

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
    get_variables(Variable_HashMap& staged, Variable_HashMap& temp) const override;

    Reference&
    invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const override;
  };

}  // namespace asteria
#endif
