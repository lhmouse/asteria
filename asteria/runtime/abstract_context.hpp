// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_CONTEXT_
#define ASTERIA_RUNTIME_ABSTRACT_CONTEXT_

#include "../fwd.hpp"
#include "../llds/reference_dictionary.hpp"
namespace asteria {

class Abstract_Context
  : public rcfwd<Abstract_Context>
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
    // Builtin references such as `__func` are only created when they are
    // mentioned.
    virtual
    Reference*
    do_create_lazy_reference_opt(Reference* hint_opt, phsh_stringR name) const = 0;

    // This function is called by `do_create_lazy_reference_opt()` to avoid
    // possibility of infinite recursion, which would otherwise be caused
    // if `mut_named_reference()` was called instead.
    Reference&
    do_open_named_reference(Reference* hint_opt, phsh_stringR name) const
      {
        return hint_opt ? *hint_opt  // hint valid
            : *(this->m_named_refs.insert(name).first);  // insert a new one
      }

    void
    do_clear_named_references() noexcept
      { this->m_named_refs.clear();  }

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
        if(qref)
          return qref;

        // If the name is not reserved, fail.
        if(!name.rdstr().starts_with(sref("__")))
          return nullptr;

        qref = this->do_create_lazy_reference_opt(nullptr, name);
        return qref;
      }

    const Reference*
    get_named_reference_with_hint_opt(size_t& hint, phsh_stringR name) const
      {
        auto qref = this->m_named_refs.find_with_hint_opt(hint, name);
        if(qref)
          return qref;

        // If the name is not reserved, fail.
        if(!name.rdstr().starts_with(sref("__")))
          return nullptr;

        qref = this->do_create_lazy_reference_opt(nullptr, name);
        return qref;
      }

    pair<Reference*, bool>
    insert_named_reference(phsh_stringR name)
      {
        auto pair = this->m_named_refs.insert(name);
        if(!pair.second)
          return pair;

        // If the name is not reserved, don't initialize it.
        if(!name.rdstr().starts_with(sref("__")))
          return pair;

        this->do_create_lazy_reference_opt(pair.first, name);
        return pair;
      }

    Reference&
    mut_named_reference(phsh_stringR name)
      {
        auto pair = this->insert_named_reference(name);
        return *(pair.first);
      }

    bool
    erase_named_reference(phsh_stringR name) noexcept
      {
        return this->m_named_refs.erase(name);
      }
  };

}  // namespace asteria
#endif
