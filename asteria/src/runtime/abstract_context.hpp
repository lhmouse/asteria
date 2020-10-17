// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_

#include "../fwd.hpp"
#include "../llds/reference_dictionary.hpp"

namespace asteria {

class Abstract_Context
  {
  public:
    struct M_plain     { };
    struct M_defer     { };
    struct M_function  { };

  private:
    // This stores all named references (variables, parameters, etc.) of
    // this context.
    Reference_Dictionary m_named_refs;

  public:
    Abstract_Context()
      noexcept
      { }

    virtual
    ~Abstract_Context();

  protected:
    virtual
    bool
    do_is_analytic()
      const noexcept
      = 0;

    virtual
    Abstract_Context*
    do_get_parent_opt()
      const noexcept
      = 0;

    virtual
    Reference*
    do_lazy_lookup_opt(const phsh_string& name)
      = 0;

  public:
    bool
    is_analytic()
      const noexcept
      { return this->do_is_analytic();  }

    Abstract_Context*
    get_parent_opt()
      const noexcept
      { return this->do_get_parent_opt();  }

    const Reference*
    get_named_reference_opt(const phsh_string& name)
      {
        auto qref = this->m_named_refs.find_opt(name);
        if(ROCKET_UNEXPECT(!qref))
          qref = this->do_lazy_lookup_opt(name);
        return qref;
      }

    Reference&
    open_named_reference(const phsh_string& name)
      { return this->m_named_refs.open(name);  }

    Abstract_Context&
    clear_named_references()
      noexcept
      { return this->m_named_refs.clear(), *this;  }
  };

}  // namespace asteria

#endif
