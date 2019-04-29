
// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "global_context.hpp"
#include "placeholder.hpp"
#include "random_number_generator.hpp"
#include "generational_collector.hpp"
#include "variable.hpp"
#include "../library/bindings_version.hpp"
#include "../library/bindings_gc.hpp"
#include "../library/bindings_debug.hpp"
#include "../library/bindings_chrono.hpp"
#include "../library/bindings_string.hpp"
#include "../library/bindings_array.hpp"
#include "../library/bindings_numeric.hpp"
#include "../library/bindings_filesystem.hpp"
#include "../utilities.hpp"

namespace Asteria {

Global_Context::~Global_Context()
  {
  }

void Global_Context::initialize(API_Version version)
  {
    // Purge the context.
    this->clear_named_references();
    ///////////////////////////////////////////////////////////////////////////
    // Initializer global objects.
    ///////////////////////////////////////////////////////////////////////////
    auto placeholder = rocket::make_refcnt<Placeholder>();
    this->m_placeholder = placeholder;
    // Use default seed.
    auto prng = rocket::make_refcnt<Random_Number_Generator>();
    this->m_prng = prng;
    // Tie the collector to `*this` so a full garbage collection is performed when this context is destroyed.
    auto gcoll = rocket::make_refcnt<Generational_Collector>();
    this->tie_collector(gcoll);
    this->m_gcoll = gcoll;
    ///////////////////////////////////////////////////////////////////////////
    // Initialize standard library modules.
    ///////////////////////////////////////////////////////////////////////////
    struct Module
      {
        API_Version version;
        const char* name;
        void (*init)(G_object&, API_Version);
      }
    static constexpr s_ymods[] =
      {
        { api_version_none,       "",            nullptr                     },
        { api_version_0001_0000,  "gc",          create_bindings_gc          },
        { api_version_0001_0000,  "debug",       create_bindings_debug       },
        { api_version_0001_0000,  "chrono",      create_bindings_chrono      },
        { api_version_0001_0000,  "string",      create_bindings_string      },
        { api_version_0001_0000,  "array",       create_bindings_array       },
        { api_version_0001_0000,  "numeric",     create_bindings_numeric     },
        { api_version_0001_0000,  "filesystem",  create_bindings_filesystem  },
      };
#ifdef ROCKET_DEBUG
    ROCKET_ASSERT(std::is_sorted(std::begin(s_ymods), std::end(s_ymods), [&](const Module& lhs, const Module& rhs) { return lhs.version < rhs.version;  }));
#endif
    G_object yobj;
    // Get the range of modules to initialize.
    // This also determines the maximum version number of the library, which will be referenced as `yend[-1].version`.
    auto yend = std::find_if(std::begin(s_ymods) + 1, std::end(s_ymods), [&](const Module& elem) { return elem.version > version;  });
    // Initialize library modules.
    for(auto cur = std::begin(s_ymods) + 1; cur != yend; ++cur) {
      // Create the subobject if it doesn't exist.
      auto pair = yobj.try_emplace(rocket::sref(cur->name));
      if(pair.second) {
        pair.first->second = G_object();
      }
      ASTERIA_DEBUG_LOG("Begin initialization of standard library module: name = ", cur->name);
      (*(cur->init))(pair.first->second.mut_object(), yend[-1].version);
      ASTERIA_DEBUG_LOG("Finished initialization of standard library module: name = ", cur->name);
    }
    // Set up version information.
    auto pair = yobj.insert_or_assign(rocket::sref("version"), G_object());
    create_bindings_version(pair.first->second.mut_object(), yend[-1].version);
    ///////////////////////////////////////////////////////////////////////////
    // Set the `std` variable.
    ///////////////////////////////////////////////////////////////////////////
    auto yvar = gcoll->create_variable(9);
    yvar->reset(Source_Location(rocket::sref("<built-in>"), 0), rocket::move(yobj), true);
    Reference_Root::S_variable xref = { yvar };
    this->open_named_reference(rocket::sref("std")) = rocket::move(xref);
    this->m_yvar = yvar;
  }

Rcobj<Placeholder> Global_Context::get_placeholder() const noexcept
  {
    auto placeholder = rocket::dynamic_pointer_cast<Placeholder>(this->m_placeholder);
    ROCKET_ASSERT(placeholder);
    return rocket::move(placeholder);
  }

Rcobj<Abstract_Opaque> Global_Context::get_placeholder_opaque() const noexcept
  {
    auto placeholder = rocket::dynamic_pointer_cast<Abstract_Opaque>(this->m_placeholder);
    ROCKET_ASSERT(placeholder);
    return rocket::move(placeholder);
  }

Rcobj<Abstract_Function> Global_Context::get_placeholder_function() const noexcept
  {
    auto placeholder = rocket::dynamic_pointer_cast<Abstract_Function>(this->m_placeholder);
    ROCKET_ASSERT(placeholder);
    return rocket::move(placeholder);
  }

std::uint32_t Global_Context::get_random_uint32() const noexcept
  {
    auto prng = rocket::dynamic_pointer_cast<Random_Number_Generator>(this->m_prng);
    ROCKET_ASSERT(prng);
    return prng->bump();
  }

Collector* Global_Context::get_collector_opt(std::size_t gindex) const
  {
    auto gcoll = rocket::dynamic_pointer_cast<Generational_Collector>(this->m_gcoll);
    ROCKET_ASSERT(gcoll);
    return (gindex < gcoll->collector_count()) ? std::addressof(gcoll->collector(gindex)) : nullptr;
  }

Rcptr<Variable> Global_Context::create_variable(std::size_t ghint) const
  {
    auto gcoll = rocket::dynamic_pointer_cast<Generational_Collector>(this->m_gcoll);
    ROCKET_ASSERT(gcoll);
    return gcoll->create_variable(ghint);
  }

std::size_t Global_Context::collect_variables(std::size_t glimit) const
  {
    auto gcoll = rocket::dynamic_pointer_cast<Generational_Collector>(this->m_gcoll);
    ROCKET_ASSERT(gcoll);
    return gcoll->collect_variables(glimit);
  }

const Value& Global_Context::get_std_member(const PreHashed_String& name) const
  {
    auto yvar = rocket::dynamic_pointer_cast<Variable>(this->m_yvar);
    ROCKET_ASSERT(yvar);
    return yvar->get_value().as_object().get_or(name, Value::get_null());
  }

Value& Global_Context::open_std_member(const PreHashed_String& name)
  {
    auto yvar = rocket::dynamic_pointer_cast<Variable>(this->m_yvar);
    ROCKET_ASSERT(yvar);
    return yvar->open_value().mut_object().try_emplace(name).first->second;
  }

bool Global_Context::remove_std_member(const PreHashed_String& name)
  {
    auto yvar = rocket::dynamic_pointer_cast<Variable>(this->m_yvar);
    ROCKET_ASSERT(yvar);
    return yvar->open_value().mut_object().erase(name);
  }

}  // namespace Asteria
