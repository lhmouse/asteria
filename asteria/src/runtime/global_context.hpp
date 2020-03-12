// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_
#define ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "../recursion_sentry.hpp"

namespace Asteria {

class Global_Context : public Abstract_Context
  {
  private:
    Recursion_Sentry m_sentry;
    rcfwdp<Generational_Collector> m_gcoll;  // the global garbage collector
    rcfwdp<Abstract_Hooks> m_hooks;

    rcfwdp<Random_Number_Generator> m_prng;  // the global pseudo random number generator
    rcfwdp<Variable> m_vstd;  // the `std` variable

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

    // These are interfaces of the global garbage collector.
    ASTERIA_INCOMPLET(Generational_Collector) const Collector& get_collector(GC_Generation gc_gen) const
      {
        auto gcoll = unerase_cast<Generational_Collector>(this->m_gcoll);
        ROCKET_ASSERT(gcoll);
        return gcoll->get_collector(gc_gen);
      }
    ASTERIA_INCOMPLET(Generational_Collector) Collector& open_collector(GC_Generation gc_gen)
      {
        auto gcoll = unerase_cast<Generational_Collector>(this->m_gcoll);
        ROCKET_ASSERT(gcoll);
        return gcoll->open_collector(gc_gen);
      }
    ASTERIA_INCOMPLET(Generational_Collector) rcptr<Variable> create_variable(GC_Generation gc_hint = gc_generation_newest)
      {
        auto gcoll = unerase_cast<Generational_Collector>(this->m_gcoll);
        ROCKET_ASSERT(gcoll);
        return gcoll->create_variable(gc_hint);
      }
    ASTERIA_INCOMPLET(Generational_Collector) size_t collect_variables(GC_Generation gc_limit = gc_generation_oldest)
      {
        auto gcoll = unerase_cast<Generational_Collector>(this->m_gcoll);
        ROCKET_ASSERT(gcoll);
        return gcoll->collect_variables(gc_limit);
      }

    // These are interfaces of the PRNG.
    ASTERIA_INCOMPLET(Random_Number_Generator) uint32_t get_random_uint32() noexcept
      {
        auto prng = unerase_cast<Random_Number_Generator>(this->m_prng);
        ROCKET_ASSERT(prng);
        return prng->bump();
      }

    // These are interfaces of the standard library.
    ASTERIA_INCOMPLET(Variable) const Value& get_std_member(const phsh_string& name) const
      {
        auto vstd = unerase_cast<Variable>(this->m_vstd);
        ROCKET_ASSERT(vstd);
        return vstd->get_value().as_object().get_or(name, null_value);
      }
    ASTERIA_INCOMPLET(Variable) Value& open_std_member(const phsh_string& name)
      {
        auto vstd = unerase_cast<Variable>(this->m_vstd);
        ROCKET_ASSERT(vstd);
        return vstd->open_value().open_object().try_emplace(name).first->second;
      }
    ASTERIA_INCOMPLET(Variable) bool remove_std_member(const phsh_string& name)
      {
        auto vstd = unerase_cast<Variable>(this->m_vstd);
        ROCKET_ASSERT(vstd);
        return vstd->open_value().open_object().erase(name);
      }

    // These are interfaces of the hook dispatcher.
    ASTERIA_INCOMPLET(Abstract_Hooks) rcptr<Abstract_Hooks> get_hooks_opt() const noexcept
      {
        return unerase_cast(this->m_hooks);
      }
    ASTERIA_INCOMPLET(Abstract_Hooks) Global_Context& set_hooks(rcptr<Abstract_Hooks> hooks_opt) noexcept
      {
        return this->m_hooks = ::rocket::move(hooks_opt), *this;
      }

    // Get the maximum API version that is supported when this library is built.
    // N.B. This function must not be inlined for this reason.
    API_Version max_api_version() const noexcept;
    // Clear all references, perform a full garbage collection, then reload the standard library.
    void initialize(API_Version version = api_version_latest);
  };

}  // namespace Asteria

#endif
