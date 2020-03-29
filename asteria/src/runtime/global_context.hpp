// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_
#define ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "abstract_hooks.hpp"
#include "../recursion_sentry.hpp"

namespace Asteria {

class Global_Context : public Abstract_Context
  {
  private:
    Recursion_Sentry m_sentry;
    rcptr<Abstract_Hooks> m_qhooks;

    rcfwdp<Generational_Collector> m_gcoll;
    rcfwdp<Random_Number_Generator> m_prng;
    rcfwdp<Variable> m_vstd;

  public:
    explicit Global_Context(API_Version version = api_version_latest)
      {
        this->initialize(version);
      }
    ~Global_Context() override;

  protected:
    bool do_is_analytic() const noexcept final;
    const Abstract_Context* do_get_parent_opt() const noexcept final;
    Reference* do_lazy_lookup_opt(const phsh_string& name) override;

  public:
    bool is_analytic() const noexcept
      {
        return false;
      }
    const Abstract_Context* get_parent_opt() const noexcept
      {
        return nullptr;
      }

    // This provides stack overflow protection.
    Recursion_Sentry copy_recursion_sentry() const
      {
        return this->m_sentry;
      }
    const void* get_recursion_base() const noexcept
      {
        return this->m_sentry.get_base();
      }
    Global_Context& set_recursion_base(const void* base) noexcept
      {
        return this->m_sentry.set_base(base), *this;
      }

    // This helps debugging and profiling.
    rcptr<Abstract_Hooks> get_hooks_opt() const noexcept
      {
        return this->m_qhooks;
      }
    Global_Context& set_hooks(rcptr<Abstract_Hooks> hooks_opt) noexcept
      {
        return this->m_qhooks = ::std::move(hooks_opt), *this;
      }

    // These are interfaces for individual global components.
    ASTERIA_INCOMPLET(Generational_Collector)
        rcptr<Generational_Collector> generational_collector() const noexcept
      {
        return unerase_cast<Generational_Collector>(this->m_gcoll);
      }
    ASTERIA_INCOMPLET(Random_Number_Generator)
        rcptr<Random_Number_Generator> random_number_generator() const noexcept
      {
        return unerase_cast<Random_Number_Generator>(this->m_prng);
      }
    ASTERIA_INCOMPLET(Variable)
        rcptr<Variable> std_variable() const noexcept
      {
        return unerase_cast<Variable>(this->m_vstd);
      }

    // Get the maximum API version that is supported when this library is built.
    // N.B. This function must not be inlined for this reason.
    API_Version max_api_version() const noexcept;
    // Clear all references, perform a full garbage collection, then reload the standard library.
    void initialize(API_Version version = api_version_latest);
  };

}  // namespace Asteria

#endif
