// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_
#define ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"

namespace Asteria {

class Global_Context : public Abstract_Context
  {
  private:
    rcptr<Rcbase> m_placeholder;
    rcptr<Rcbase> m_prng;
    rcptr<Rcbase> m_stdv;

  public:
    explicit Global_Context(API_Version version = api_version_latest)
      {
        this->initialize(version);
      }
    ~Global_Context() override;

  public:
    bool is_analytic() const noexcept override
      {
        return false;
      }
    const Abstract_Context* get_parent_opt() const noexcept override
      {
        return nullptr;
      }

    // Clear all references, perform a full garbage collection, then reload the standard library.
    void initialize(API_Version version = api_version_latest);

    // These are interfaces of the global garbage collector.
    Collector* get_collector_opt(size_t gen) const;
    rcptr<Variable> create_variable(size_t gen_hint = 0) const;
    size_t collect_variables(size_t gen_limit = 9) const;

    // These are interfaces of the placeholder.
    rcobj<Placeholder> placeholder() const noexcept;
    rcobj<Abstract_Opaque> placeholder_opaque() const noexcept;
    rcobj<Abstract_Function> placeholder_function() const noexcept;

    // These are interfaces of the PRNG.
    uint32_t get_random_uint32() const noexcept;

    // These are interfaces of the standard library.
    const Value& get_std_member(const phsh_string& name) const;
    Value& open_std_member(const phsh_string& name);
    bool remove_std_member(const phsh_string& name);
  };

}  // namespace Asteria

#endif
