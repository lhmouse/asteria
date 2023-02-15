// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ANALYTIC_CONTEXT_
#define ASTERIA_RUNTIME_ANALYTIC_CONTEXT_

#include "../fwd.hpp"
#include "abstract_context.hpp"
namespace asteria {

class Analytic_Context
  : public Abstract_Context
  {
  private:
    Abstract_Context* m_parent_opt;

  public:
    // A plain context must have a parent context.
    // Its parent context shall outlast itself.
    explicit
    Analytic_Context(M_plain, Abstract_Context& parent)
      : m_parent_opt(&parent)  { }

    // A function context may have a parent.
    // Names found in ancestor contexts will be bound into the
    // instantiated function object.
    explicit
    Analytic_Context(M_function, Abstract_Context* parent_opt,
                     const cow_vector<phsh_string>& params);

  protected:
    bool
    do_is_analytic() const noexcept final
      { return this->is_analytic();  }

    Abstract_Context*
    do_get_parent_opt() const noexcept override
      { return this->get_parent_opt();  }

    Reference*
    do_create_lazy_reference_opt(Reference*, phsh_stringR) const override
      { return nullptr;  }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Analytic_Context);

    bool
    is_analytic() const noexcept
      { return true;  }

    Abstract_Context*
    get_parent_opt() const noexcept
      { return this->m_parent_opt;  }
  };

}  // namespace asteria
#endif
