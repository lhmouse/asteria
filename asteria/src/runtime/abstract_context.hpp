// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_

#include "../fwd.hpp"
#include "../llds/reference_dictionary.hpp"

namespace Asteria {

class Abstract_Context
  {
  private:
    // This pointer owns a reference count of the collector if it isn't null.
    // It triggers a full garbage collection when destroyed, which occurs after destruction
    // of all local references, so it shall precede everything else.
    struct Cleaner { void operator()(Rcbase* base) noexcept;  };
    uptr<Rcbase, Cleaner> m_coll_opt;
    // This stores all named references (variables, parameters, etc.) of this context.
    mutable Reference_Dictionary m_named_refs;

  public:
    Abstract_Context() noexcept
      {
      }
    virtual ~Abstract_Context();

    Abstract_Context(const Abstract_Context&)
      = delete;
    Abstract_Context& operator=(const Abstract_Context&)
      = delete;

  protected:
    virtual bool do_is_analytic() const noexcept = 0;
    virtual const Abstract_Context* do_get_parent_opt() const noexcept = 0;
    virtual Reference* do_allocate_reference_lazy_opt(Reference_Dictionary& named_refs, const phsh_string& name) const = 0;

  public:
    bool is_analytic() const noexcept
      {
        return this->do_is_analytic();
      }
    const Abstract_Context* get_parent_opt() const noexcept
      {
        return this->do_get_parent_opt();
      }

    const Reference* get_named_reference_opt(const phsh_string& name) const
      {
        auto qref = this->m_named_refs.get_opt(name);
        if(ROCKET_UNEXPECT(!qref)) {
          // Initialize builtins only when needed.
          qref = this->do_allocate_reference_lazy_opt(this->m_named_refs, name);
        }
        return qref;
      }
    Reference& open_named_reference(const phsh_string& name)
      {
        return this->m_named_refs.open(name);
      }
    Abstract_Context& clear_named_references() noexcept
      {
        return this->m_named_refs.clear(), *this;
      }

    Generational_Collector* get_tied_collector_opt() const noexcept;
    void tie_collector(const rcptr<Generational_Collector>& coll_opt) noexcept;
  };

}  // namespace Asteria

#endif
