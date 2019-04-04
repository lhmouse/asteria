// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "global_context.hpp"
#include "generational_collector.hpp"
#include "variable.hpp"
#include "../library/bindings_debug.hpp"
#include "../library/bindings_chrono.hpp"
#include "../library/bindings_string.hpp"
#include "../utilities.hpp"

namespace Asteria {

Global_Context::~Global_Context()
  {
  }

void Global_Context::initialize(API_Version version)
  {
    // Purge the context.
    this->clear_named_references();
    // Initialize the global garbage collector.
    auto collector = rocket::make_refcnt<Generational_Collector>();
    this->tie_collector(collector);
    this->m_collector = collector;
    // Define the list of standard library components.
    struct Component
      {
        API_Version version;
        const char* name;
        void (*init)(D_object&, API_Version);
      }
    static constexpr s_components[] =
      {
        { api_version_0001_0000,  "debug",   create_bindings_debug   },
        { api_version_0001_0000,  "chrono",  create_bindings_chrono  },
        { api_version_0001_0000,  "string",  create_bindings_string  },
      };
    // Create the `std` object.
    D_object std_obj;
    for(auto cur = std::begin(s_components); (cur != std::end(s_components)) && (cur->version <= version); ++cur) {
      // Create the subobject if it doesn't exist.
      auto pair = std_obj.try_emplace(rocket::sref(cur->name));
      if(pair.second) {
        pair.first->second = D_object();
      }
      ASTERIA_DEBUG_LOG("Begin initialization of standard library component: name = ", cur->name);
      (*(cur->init))(pair.first->second.check<D_object>(), version);
      ASTERIA_DEBUG_LOG("Finished initialization of standard library component: name = ", cur->name);
    }
    auto std_var = collector->create_variable();
    std_var->reset(Source_Location(rocket::sref("<builtin>"), 0), rocket::move(std_obj), true);
    Reference_Root::S_variable xref = { std_var };
    this->open_named_reference(rocket::sref("std")) = rocket::move(xref);
    this->m_std_var = std_var;
  }

Rcptr<Variable> Global_Context::create_variable() const
  {
    auto collector = rocket::dynamic_pointer_cast<Generational_Collector>(this->m_collector);
    ROCKET_ASSERT(collector);
    return collector->create_variable();
  }

bool Global_Context::collect_variables(unsigned gen_limit) const
  {
    auto collector = rocket::dynamic_pointer_cast<Generational_Collector>(this->m_collector);
    ROCKET_ASSERT(collector);
    return collector->collect_variables(gen_limit);
  }

const Value& Global_Context::get_std_member(const PreHashed_String& name) const
  {
    auto std_var = rocket::dynamic_pointer_cast<Variable>(this->m_std_var);
    ROCKET_ASSERT(std_var);
    return std_var->get_value().check<D_object>().get_or(name, Value::get_null());
  }

Value& Global_Context::open_std_member(const PreHashed_String& name)
  {
    auto std_var = rocket::dynamic_pointer_cast<Variable>(this->m_std_var);
    ROCKET_ASSERT(std_var);
    return std_var->open_value().check<D_object>()[name];
  }

bool Global_Context::remove_std_member(const PreHashed_String& name)
  {
    auto std_var = rocket::dynamic_pointer_cast<Variable>(this->m_std_var);
    ROCKET_ASSERT(std_var);
    return std_var->open_value().check<D_object>().erase(name);
  }

}  // namespace Asteria
