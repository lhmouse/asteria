// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_
#define ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_

#include "../fwd.hpp"
#include "../rcbase.hpp"
#include "abstract_context.hpp"

namespace Asteria {

class Global_Context final : public virtual Rcbase, public Abstract_Context
  {
  private:
    rcptr<Rcbase> m_prng;  // the global pseudo random number generator
    rcptr<Rcbase> m_stdv;  // the `std` variable
    rcptr<Rcbase> m_hooks_opt;  // the hook callback dispatcher

  public:
    explicit Global_Context(API_Version version = api_version_latest)
      {
        this->initialize(version);
      }
    ~Global_Context() override;

  protected:
    bool do_is_analytic() const noexcept override;
    const Abstract_Context* do_get_parent_opt() const noexcept override;
    Reference* do_lazy_lookup_opt(Reference_Dictionary& named_refs, const phsh_string& name) const override;

  public:
    bool is_analytic() const noexcept
      {
        return false;
      }
    const Abstract_Context* get_parent_opt() const noexcept
      {
        return nullptr;
      }

    // Get the maximum API version that is supported when this library is built.
    // N.B. This function must not be inlined for this reason.
    API_Version max_api_version() const noexcept;
    // Clear all references, perform a full garbage collection, then reload the standard library.
    void initialize(API_Version version = api_version_latest);

    // These are interfaces of the global garbage collector.
    Collector* get_collector_opt(GC_Generation gc_gen) const;
    rcptr<Variable> create_variable(const Source_Location& sloc, const phsh_string& name, GC_Generation gc_hint = gc_generation_newest) const;
    size_t collect_variables(GC_Generation gc_limit = gc_generation_oldest) const;

    // These are interfaces of the PRNG.
    uint32_t get_random_uint32() const noexcept;

    // These are interfaces of the standard library.
    const Value& get_std_member(const phsh_string& name) const;
    Value& open_std_member(const phsh_string& name);
    bool remove_std_member(const phsh_string& name);

    // These are interfaces of the hook dispatcher.
    rcptr<Abstract_Hooks> get_hooks_opt() const noexcept;
    rcptr<Abstract_Hooks> set_hooks(rcptr<Abstract_Hooks> hooks_opt) noexcept;
  };

}  // namespace Asteria

#endif
