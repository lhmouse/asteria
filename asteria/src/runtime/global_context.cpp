
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
#include "../library/bindings_math.hpp"
#include "../library/bindings_filesystem.hpp"
#include "../library/bindings_checksum.hpp"
#include "../utilities.hpp"

namespace Asteria {

Global_Context::~Global_Context()
  {
  }

    namespace {

    // N.B. Please keep this list sorted by the `version` member.
    struct Module
      {
        API_Version version;
        const char* name;
        void (*init)(G_object&, API_Version);
      }
    constexpr s_modules[] =
      {
        { api_version_none,       "",            nullptr                     },
        { api_version_0001_0000,  "gc",          create_bindings_gc          },
        { api_version_0001_0000,  "debug",       create_bindings_debug       },
        { api_version_0001_0000,  "chrono",      create_bindings_chrono      },
        { api_version_0001_0000,  "string",      create_bindings_string      },
        { api_version_0001_0000,  "array",       create_bindings_array       },
        { api_version_0001_0000,  "numeric",     create_bindings_numeric     },
        { api_version_0001_0000,  "math",        create_bindings_math        },
        { api_version_0001_0000,  "filesystem",  create_bindings_filesystem  },
        { api_version_0001_0000,  "checksum",    create_bindings_checksum    },
      };

    struct Module_Comparator
      {
        bool operator()(const Module& lhs, const Module& rhs) const noexcept
          {
            return lhs.version < rhs.version;
          }
        bool operator()(API_Version lhs, const Module& rhs) const noexcept
          {
            return lhs < rhs.version;
          }
        bool operator()(const Module& lhs, API_Version rhs) const noexcept
          {
            return lhs.version < rhs;
          }
      };

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
#ifdef ROCKET_DEBUG
    ROCKET_ASSERT(std::is_sorted(std::begin(s_modules), std::end(s_modules), Module_Comparator()));
#endif
    // Get the range of modules to initialize.
    // This also determines the maximum version number of the library, which will be referenced as `yend[-1].version`.
    G_object ostd;
    auto emods = std::upper_bound(std::begin(s_modules) + 1, std::end(s_modules), version, Module_Comparator());
    // Initialize library modules.
    for(auto q = std::begin(s_modules) + 1; q != emods; ++q) {
      // Create the subobject if it doesn't exist.
      auto pair = ostd.try_emplace(rocket::sref(q->name));
      if(pair.second) {
        ROCKET_ASSERT(pair.first->second.is_null());
        pair.first->second = G_object();
      }
      ASTERIA_DEBUG_LOG("Begin initialization of standard library module: name = ", q->name);
      (*(q->init))(pair.first->second.mut_object(), emods[-1].version);
      ASTERIA_DEBUG_LOG("Finished initialization of standard library module: name = ", q->name);
    }
    // Set up version information.
    auto pair = ostd.try_emplace(rocket::sref("version"));
    if(pair.second) {
      ROCKET_ASSERT(pair.first->second.is_null());
      pair.first->second = G_object();
    }
    create_bindings_version(pair.first->second.mut_object(), emods[-1].version);
    ///////////////////////////////////////////////////////////////////////////
    // Set the `std` variable.
    ///////////////////////////////////////////////////////////////////////////
    auto vstd = gcoll->create_variable(9);
    vstd->reset(Source_Location(rocket::sref("<built-in>"), 0), rocket::move(ostd), true);
    Reference_Root::S_variable xref = { vstd };
    this->open_named_reference(rocket::sref("std")) = rocket::move(xref);
    this->m_vstd = vstd;
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
    auto vstd = rocket::dynamic_pointer_cast<Variable>(this->m_vstd);
    ROCKET_ASSERT(vstd);
    return vstd->get_value().as_object().get_or(name, Value::get_null());
  }

Value& Global_Context::open_std_member(const PreHashed_String& name)
  {
    auto vstd = rocket::dynamic_pointer_cast<Variable>(this->m_vstd);
    ROCKET_ASSERT(vstd);
    return vstd->open_value().mut_object().try_emplace(name).first->second;
  }

bool Global_Context::remove_std_member(const PreHashed_String& name)
  {
    auto vstd = rocket::dynamic_pointer_cast<Variable>(this->m_vstd);
    ROCKET_ASSERT(vstd);
    return vstd->open_value().mut_object().erase(name);
  }

}  // namespace Asteria
