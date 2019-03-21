// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_
#define ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "rcbase.hpp"

namespace Asteria {

class Global_Context : public Abstract_Context
  {
  public:
    enum API_Version : std::uint32_t
      {
        api_none    = 0x00000000,  // no standard library
        api_1_0     = 0x00010000,  // `debug`, `chrono`, `string`
        api_newest  = 0xFFFFFFFF,  // everything
      };

  private:
    // This is the global garbage collector.
    Rcptr<Rcbase> m_collector;
    // This is the variable holding an object referenced as `std` in this context.
    Rcptr<Rcbase> m_std_var;

  public:
    // A global context does not have a parent context.
    explicit Global_Context(API_Version api_ver = api_newest)
      {
        this->initialize(api_ver);
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
    void initialize(API_Version api_ver = api_newest);

    // These are interfaces of the global garbage collector/
    Rcptr<Variable> create_variable() const;
    bool collect_variables(unsigned gen_limit = 0x7F) const;

    // These are interfaces of the standard library.
    const Value& get_std_member(const PreHashed_String& name) const;
    Value& open_std_member(const PreHashed_String& name);
    bool remove_std_member(const PreHashed_String& name);
  };

}  // namespace Asteria

#endif
