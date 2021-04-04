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

    // This function is called when a name is not found in `m_named_refs`.
    // Builtin references such as `__func` are only created when they are
    // mentioned.
    virtual
    Reference*
    do_create_lazy_reference(Reference* hint_opt, const phsh_string& name)
      const
      = 0;

    // This function is called by `do_create_lazy_reference()` to avoid
    // possibility of infinite recursion, which would otherwise be caused
    // if `open_named_reference()` was called instead.
    Reference&
    do_open_named_reference(Reference* hint_opt, const phsh_string& name)
      const
      {
        auto qref = hint_opt;
        if(ROCKET_UNEXPECT(!qref))
          qref = this->m_named_refs.insert(name).first;
        return *qref;
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
      const
      {
        auto qref = this->m_named_refs.find_opt(name);
        if(ROCKET_UNEXPECT(!qref))
          qref = this->do_create_lazy_reference(nullptr, name);
        return qref;
      }

    Reference&
    open_named_reference(const phsh_string& name)
      {
        auto pair = this->m_named_refs.insert(name);
        if(ROCKET_UNEXPECT(pair.second))
          this->do_create_lazy_reference(pair.first, name);
        return *(pair.first);
      }
  };

}  // namespace asteria

#endif
