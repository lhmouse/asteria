// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "global_context.hpp"
#include "generational_collector.hpp"
#include "random_number_generator.hpp"
#include "variable.hpp"
#include "abstract_hooks.hpp"
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
#include "../library/bindings_json.hpp"
#include "../library/bindings_process.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

using Module_Initializer = void (V_object& result, API_Version version);

// N.B. Please keep this list sorted by the `version` member.
struct Module
  {
    API_Version version;
    const char* name;
    Module_Initializer& init;
  }
constexpr s_modules[] =
  {
    { api_version_none,       "version",     create_bindings_version     },
    { api_version_0001_0000,  "gc",          create_bindings_gc          },
    { api_version_0001_0000,  "debug",       create_bindings_debug       },
    { api_version_0001_0000,  "chrono",      create_bindings_chrono      },
    { api_version_0001_0000,  "string",      create_bindings_string      },
    { api_version_0001_0000,  "array",       create_bindings_array       },
    { api_version_0001_0000,  "numeric",     create_bindings_numeric     },
    { api_version_0001_0000,  "math",        create_bindings_math        },
    { api_version_0001_0000,  "filesystem",  create_bindings_filesystem  },
    { api_version_0001_0000,  "checksum",    create_bindings_checksum    },
    { api_version_0001_0000,  "json",        create_bindings_json        },
    { api_version_0001_0000,  "process",     create_bindings_process     },
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

}  // namespace

Global_Context::~Global_Context()
  {
    auto gcoll = unerase_cast(this->m_gcoll);
    ROCKET_ASSERT(gcoll);
    gcoll->wipe_out_variables();
  }

bool Global_Context::do_is_analytic() const noexcept
  {
    return this->is_analytic();
  }

const Abstract_Context* Global_Context::do_get_parent_opt() const noexcept
  {
    return this->get_parent_opt();
  }

Reference* Global_Context::do_lazy_lookup_opt(const phsh_string& /*name*/)
  {
    return nullptr;
  }

API_Version Global_Context::max_api_version() const noexcept
  {
    return static_cast<API_Version>(api_version_sentinel - 1);
  }

void Global_Context::initialize(API_Version version)
  {
    // Tidy old contents.
    this->clear_named_references();
    this->m_vstd.reset();
    // Perform a level-2 garbage collection.
    auto gcoll = unerase_cast(this->m_gcoll);
    try {
      if(gcoll)
        gcoll->collect_variables(gc_generation_oldest);
    }
    catch(exception& stdex) {
      // Ignore this exception, but notify the user about this error.
      ::fprintf(stderr, "-- WARNING: garbage collection failed: %s\n", stdex.what());
    }
    if(!gcoll)
      gcoll = ::rocket::make_refcnt<Generational_Collector>();
    this->m_gcoll = gcoll;

    // Initialize the global random numbger generator.
    auto prng = unerase_cast(this->m_prng);
    if(!prng)
      prng = ::rocket::make_refcnt<Random_Number_Generator>();
    this->m_prng = prng;

    // Initialize standard library modules.
#ifdef ROCKET_DEBUG
    ROCKET_ASSERT(::std::is_sorted(begin(s_modules), end(s_modules), Module_Comparator()));
#endif
    // Get the range of modules to initialize.
    // This also determines the maximum version number of the library, which will be referenced
    // as `yend[-1].version`.
    V_object ostd;
    auto bptr = begin(s_modules);
    auto eptr = ::std::upper_bound(bptr, end(s_modules), version, Module_Comparator());
    // Initialize library modules.
    for(auto q = bptr;  q != eptr;  ++q) {
      // Create the subobject if it doesn't exist.
      auto pair = ostd.try_emplace(::rocket::sref(q->name));
      if(pair.second) {
        ROCKET_ASSERT(pair.first->second.is_null());
        pair.first->second = V_object();
      }
      q->init(pair.first->second.open_object(), eptr[-1].version);
    }
    auto vstd = gcoll->create_variable(gc_generation_oldest);
    vstd->initialize(::rocket::move(ostd), true);
    // Set the `std` reference now.
    Reference_root::S_variable xref = { vstd };
    this->open_named_reference(::rocket::sref("std")) = ::rocket::move(xref);
    this->m_vstd = vstd;
  }

}  // namespace Asteria
