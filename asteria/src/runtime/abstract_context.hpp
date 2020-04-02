// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_

#include "../fwd.hpp"
#include "../llds/reference_dictionary.hpp"

namespace Asteria {

class Abstract_Context
  {
  private:
    // This stores all named references (variables, parameters, etc.) of this context.
    mutable Reference_Dictionary m_named_refs;

  public:
    constexpr Abstract_Context() noexcept
      = default;

    virtual ~Abstract_Context();

    Abstract_Context(const Abstract_Context&)
      = delete;

    Abstract_Context& operator=(const Abstract_Context&)
      = delete;

  protected:
    virtual bool do_is_analytic() const noexcept = 0;
    virtual const Abstract_Context* do_get_parent_opt() const noexcept = 0;
    virtual Reference* do_lazy_lookup_opt(const phsh_string& name) = 0;

  public:
    bool is_analytic() const noexcept
      { return this->do_is_analytic();  }

    const Abstract_Context* get_parent_opt() const noexcept
      { return this->do_get_parent_opt();  }

    const Reference* get_named_reference_opt(const phsh_string& name) const
      {
        auto qref = this->m_named_refs.get_opt(name);
        // Initialize builtins only when needed.
        if(ROCKET_UNEXPECT(!qref))
          qref = const_cast<Abstract_Context*>(this)->do_lazy_lookup_opt(name);
        return qref;
      }

    Reference& open_named_reference(const phsh_string& name)
      { return this->m_named_refs.open(name);  }

    Abstract_Context& clear_named_references() noexcept
      { return this->m_named_refs.clear(), *this;  }
  };

}  // namespace Asteria

#endif
