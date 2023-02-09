// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "global_context.hpp"
#include "garbage_collector.hpp"
#include "random_engine.hpp"
#include "module_loader.hpp"
#include "variable.hpp"
#include "abstract_hooks.hpp"
#include "../library/version.hpp"
#include "../library/system.hpp"
#include "../library/debug.hpp"
#include "../library/chrono.hpp"
#include "../library/string.hpp"
#include "../library/array.hpp"
#include "../library/numeric.hpp"
#include "../library/math.hpp"
#include "../library/filesystem.hpp"
#include "../library/checksum.hpp"
#include "../library/json.hpp"
#include "../library/io.hpp"
#include "../library/zlib.hpp"
#include "../library/ini.hpp"
#include "../library/csv.hpp"
#include "../utils.hpp"
namespace asteria {
namespace {

// N.B. Please keep this list sorted by the `version` member.
struct Module
  {
    API_Version version;
    const char* name;
    decltype(create_bindings_version)& init;
  }
constexpr s_modules[] =
  {
    { api_version_none,       "version",     create_bindings_version     },
    { api_version_0001_0000,  "system",      create_bindings_system      },
    { api_version_0001_0000,  "debug",       create_bindings_debug       },
    { api_version_0001_0000,  "chrono",      create_bindings_chrono      },
    { api_version_0001_0000,  "string",      create_bindings_string      },
    { api_version_0001_0000,  "array",       create_bindings_array       },
    { api_version_0001_0000,  "numeric",     create_bindings_numeric     },
    { api_version_0001_0000,  "math",        create_bindings_math        },
    { api_version_0001_0000,  "filesystem",  create_bindings_filesystem  },
    { api_version_0001_0000,  "checksum",    create_bindings_checksum    },
    { api_version_0001_0000,  "json",        create_bindings_json        },
    { api_version_0001_0000,  "io",          create_bindings_io          },
    { api_version_0001_0000,  "zlib",        create_bindings_zlib        },
    { api_version_0001_0000,  "ini",         create_bindings_ini         },
    { api_version_0001_0000,  "csv",         create_bindings_csv         },
  };

struct Module_Comparator
  {
    constexpr
    bool
    operator()(const Module& lhs, const Module& rhs) const noexcept
      { return lhs.version < rhs.version;  }

    constexpr
    bool
    operator()(API_Version lhs, const Module& rhs) const noexcept
      { return lhs < rhs.version;  }

    constexpr
    bool
    operator()(const Module& lhs, API_Version rhs) const noexcept
      { return lhs.version < rhs;  }
  };

}  // namespace

Global_Context::
Global_Context(API_Version version)
  : m_gcoll(::rocket::make_refcnt<Garbage_Collector>()),
    m_prng(::rocket::make_refcnt<Random_Engine>()),
    m_ldrlk(::rocket::make_refcnt<Module_Loader>())
  {
    // Get the range of modules to initialize.
    // This also determines the maximum version number of the library, which
    // will be referenced as `yend[-1].version`.
    static constexpr Module_Comparator comp;
#ifdef ROCKET_DEBUG
    ROCKET_ASSERT(::std::is_sorted(begin(s_modules), end(s_modules), comp));
#endif
    auto bptr = begin(s_modules);
    auto eptr = ::std::upper_bound(bptr, end(s_modules), version, comp);

    V_object ostd;
    ::std::for_each(bptr, eptr,
      [&](const Module& mod) {
        auto r = ostd.try_emplace(sref(mod.name));
        if(r.second)
          r.first->second = V_object();

        mod.init(r.first->second.mut_object(), eptr[-1].version);
      });

    // Allocate the global variable `std`.
    const auto gcoll = unerase_cast<Garbage_Collector*>(this->m_gcoll);
    ROCKET_ASSERT(gcoll);
    auto vstd = gcoll->create_variable(gc_generation_oldest);
    vstd->initialize(::std::move(ostd), Variable::state_immutable);
    this->do_open_named_reference(nullptr, sref("std")).set_variable(vstd);
    this->m_vstd = ::std::move(vstd);
  }

Global_Context::
~Global_Context()
  {
    // Perform the final garbage collection. Note if there are still cyclic
    // references afterwards, they are left uncollected!
    this->do_clear_named_references();
    const auto gcoll = unerase_cast<Garbage_Collector*>(this->m_gcoll);
    ROCKET_ASSERT(gcoll);
    gcoll->finalize();
  }

API_Version
Global_Context::
max_api_version() const noexcept
  {
    return static_cast<API_Version>(api_version_sentinel - 1);
  }

}  // namespace asteria
