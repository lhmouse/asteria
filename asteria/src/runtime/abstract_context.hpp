// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_

#include "../fwd.hpp"
#include "../llds/reference_dictionary.hpp"

namespace asteria {

class Abstract_Context
  : public Rcfwd<Abstract_Context>
  {
  public:
    struct M_plain     { };
    struct M_defer     { };
    struct M_function  { };

  private:
    // This stores all named references (variables, parameters, etc.) of
    // this context.
    mutable Reference_Dictionary m_named_refs;

  protected:
    explicit
    Abstract_Context()
      noexcept
      = default;

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
      const
      = 0;

    template<typename XRefT>
    Reference*
    do_set_named_reference(const phsh_string& name, XRefT&& xref)
      const
      {
        auto& ref = this->m_named_refs.open(name);
        ref = ::std::forward<XRefT>(xref);
        return &ref;
      }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Abstract_Context);

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
      const;

    Reference&
    open_named_reference(const phsh_string& name);
  };

}  // namespace asteria

#endif
