// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_CONTEXT_
#define ASTERIA_RUNTIME_ABSTRACT_CONTEXT_

#include "../fwd.hpp"
#include "../llds/reference_dictionary.hpp"
namespace asteria {

class Abstract_Context
  :
    public rcfwd<Abstract_Context>
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
    Abstract_Context() noexcept = default;

  protected:
    virtual
    bool
    do_is_analytic() const noexcept = 0;

    virtual
    Abstract_Context*
    do_get_parent_opt() const noexcept = 0;

    // This function is called when a name is not found in `m_named_refs`.
    // Built-in references such as `__func` are only created when they are
    // mentioned.
    virtual
    Reference*
    do_create_lazy_reference_opt(Reference* hint_opt, phsh_stringR name) const = 0;

    // This function is called by `do_create_lazy_reference_opt()` to avoid
    // infinite recursion.
    Reference&
    do_mut_named_reference(Reference* hint_opt, phsh_stringR name) const
      {
        return hint_opt ? *hint_opt : this->m_named_refs.insert(name);
      }

    void
    do_clear_named_references() noexcept
      {
        this->m_named_refs.clear();
      }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Abstract_Context);

    bool
    is_analytic() const noexcept
      { return this->do_is_analytic();  }

    Abstract_Context*
    get_parent_opt() const noexcept
      { return this->do_get_parent_opt();  }

    const Reference*
    get_named_reference_opt(phsh_stringR name) const
      {
        auto qref = this->m_named_refs.find_opt(name);
        if(ROCKET_UNEXPECT(!qref) && (name[0] == '_') && (name[1] == '_')) {
          // Create a lazy reference. Built-in references such as `__func`
          // are only created when they are mentioned.
          qref = this->do_create_lazy_reference_opt(nullptr, name);
        }
        return qref;
      }

    Reference&
    insert_named_reference(phsh_stringR name)
      {
        bool newly;
        auto& ref = this->m_named_refs.insert(name, &newly);
        if(ROCKET_UNEXPECT(newly) && (name[0] == '_') && (name[1] == '_')) {
          // If a built-in reference has been inserted, it may have a default
          // value, so initialize it. DO NOT CALL THIS FUNCTION INSIDE
          // `do_create_lazy_reference_opt()`, as it will result in infinite
          // recursion; call `do_mut_named_reference()` instead.
          this->do_create_lazy_reference_opt(&ref, name);
        }
        return ref;
      }

    bool
    erase_named_reference(phsh_stringR name, Reference* refp_opt = nullptr) noexcept
      {
        return this->m_named_refs.erase(name, refp_opt);
      }
  };

}  // namespace asteria
#endif
