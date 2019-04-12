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
    // This is used to initialize an object of type `G_opaque` or `G_function` which does not have a default constructor.
    Rcptr<Rcbase> m_placeholder;
    // This is the global high-quality Pseudo Random Number Generator (PRNG).
    Rcptr<Rcbase> m_prng;
    // This is the global garbage collector.
    Rcptr<Rcbase> m_gcoll;
    // This is the variable holding an object referenced as `std` in this context.
    Rcptr<Rcbase> m_std_var;

  public:
    // A global context does not have a parent context.
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

    // These are interfaces of the placeholder.
    Rcobj<Placeholder> get_placeholder() const noexcept;
    Rcobj<Abstract_Opaque> get_placeholder_opaque() const noexcept;
    Rcobj<Abstract_Function> get_placeholder_function() const noexcept;

    // These are interfaces of the PRNG.
    std::uint32_t get_random_uint32() const noexcept;

    // These are interfaces of the global garbage collector.
    Collector* get_collector_opt(unsigned generation) const;
    Rcptr<Variable> create_variable() const;
    std::size_t collect_variables(unsigned generation_limit = INT_MAX) const;

    // These are interfaces of the standard library.
    const Value& get_std_member(const PreHashed_String& name) const;
    Value& open_std_member(const PreHashed_String& name);
    bool remove_std_member(const PreHashed_String& name);
  };

}  // namespace Asteria

#endif
