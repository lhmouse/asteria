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
    Reference_Dictionary m_named_refs;

  public:
    Abstract_Context() noexcept
      {
      }
    virtual ~Abstract_Context();

    Abstract_Context(const Abstract_Context&)
      = delete;
    Abstract_Context& operator=(const Abstract_Context&)
      = delete;

  public:
    const Reference* get_named_reference_opt(const phsh_string& name) const
      {
        return this->m_named_refs.get_opt(name);
      }
    Reference& open_named_reference(const phsh_string& name)
      {
        return this->m_named_refs.open(name);
      }
    void clear_named_references() noexcept
      {
        this->m_named_refs.clear();
      }

    Generational_Collector* tied_collector_opt() const noexcept;
    void tie_collector(const rcptr<Generational_Collector>& coll_opt) noexcept;

    virtual bool is_analytic() const noexcept = 0;
    virtual const Abstract_Context* get_parent_opt() const noexcept = 0;
  };

}  // namespace Asteria

#endif
