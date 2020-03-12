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
    rcfwdp<Generational_Collector> m_gcoll;  // the global garbage collector
    rcfwdp<Abstract_Hooks> m_hooks_opt;
    Recursion_Sentry m_sentry;

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
    const Collector& get_collector(GC_Generation gc_gen) const;
    Collector& open_collector(GC_Generation gc_gen);
    rcptr<Variable> create_variable(GC_Generation gc_hint = gc_generation_newest);
    size_t collect_variables(GC_Generation gc_limit = gc_generation_oldest);

    // These are interfaces of the PRNG.
    uint32_t get_random_uint32() noexcept;

    // These are interfaces of the standard library.
    const Value& get_std_member(const phsh_string& name) const;
    Value& open_std_member(const phsh_string& name);
    bool remove_std_member(const phsh_string& name);

    // These are interfaces of the hook dispatcher.
    rcptr<Abstract_Hooks> get_hooks_opt() const noexcept;
    Global_Context& set_hooks(rcptr<Abstract_Hooks> hooks_opt) noexcept;

    // Get the maximum API version that is supported when this library is built.
    // N.B. This function must not be inlined for this reason.
    API_Version max_api_version() const noexcept;
    // Clear all references, perform a full garbage collection, then reload the standard library.
    void initialize(API_Version version = api_version_latest);
  };

}  // namespace Asteria

#endif
