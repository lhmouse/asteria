// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "global_context.hpp"
#include "random_number_generator.hpp"
#include "generational_collector.hpp"
#include "abstract_hooks.hpp"
#include "variable.hpp"
#include "../placeholder.hpp"
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
#include "../utilities.hpp"

namespace Asteria {

Global_Context::~Global_Context()
  {
  }

bool Global_Context::do_is_analytic() const noexcept
  {
    return this->is_analytic();
  }

const Abstract_Context* Global_Context::do_get_parent_opt() const noexcept
  {
    return this->get_parent_opt();
  }

Reference* Global_Context::do_lazy_lookup_opt(Reference_Dictionary& /*named_refs*/, const phsh_string& name) const
  {
    ASTERIA_DEBUG_LOG("An undeclared identifier `", name, "` has been encountered.");
    return nullptr;
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
        { api_version_0001_0000,  "json",        create_bindings_json        },
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

API_Version Global_Context::max_api_version() const noexcept
  {
    return static_cast<API_Version>(api_version_sentinel - 1);
  }

void Global_Context::initialize(API_Version version)
  {
    // Initialize global objects.
    this->clear_named_references();
    // Initialize the global garbage collector.
    auto gcoll = rocket::make_refcnt<Generational_Collector>();
    this->tie_collector(gcoll);
    // Allocate a new placeholder.
    auto xph = rocket::make_refcnt<Placeholder>();
    this->m_xph = xph;
    // Use default seed.
    auto prng = rocket::make_refcnt<Random_Number_Generator>();
    this->m_prng = prng;
    // Initialize standard library modules.
#ifdef ROCKET_DEBUG
    ROCKET_ASSERT(std::is_sorted(begin(s_modules), end(s_modules), Module_Comparator()));
#endif
    // Get the range of modules to initialize.
    // This also determines the maximum version number of the library, which will be referenced as `yend[-1].version`.
    G_object stdo;
    auto bmods = begin(s_modules) + 1;
    auto emods = std::upper_bound(bmods, end(s_modules), version, Module_Comparator());
    // Initialize library modules.
    for(auto q = bmods; q != emods; ++q) {
      // Create the subobject if it doesn't exist.
      auto pair = stdo.try_emplace(rocket::sref(q->name));
      if(pair.second) {
        ROCKET_ASSERT(pair.first->second.is_null());
        pair.first->second = G_object();
      }
      ASTERIA_DEBUG_LOG("Begin initialization of standard library module: name = ", q->name);
      (*(q->init))(pair.first->second.open_object(), emods[-1].version);
      ASTERIA_DEBUG_LOG("Finished initialization of standard library module: name = ", q->name);
    }
    // Set up version information.
    auto pair = stdo.try_emplace(rocket::sref("version"));
    if(pair.second) {
      ROCKET_ASSERT(pair.first->second.is_null());
      pair.first->second = G_object();
    }
    create_bindings_version(pair.first->second.open_object(), emods[-1].version);
    // Set the `std` variable now.
    auto stdv = gcoll->create_variable(gc_generation_oldest);
    stdv->reset(rocket::move(stdo), true);
    Reference_Root::S_variable xref = { stdv };
    this->open_named_reference(rocket::sref("std")) = rocket::move(xref);
    this->m_stdv = stdv;
  }

Collector* Global_Context::get_collector_opt(GC_Generation gc_gen) const
  {
    Collector* coll = nullptr;
    // Get the collector for generation `gc_gen`.
    auto gcoll = this->get_tied_collector_opt();
    if(ROCKET_EXPECT(gcoll)) {
      coll = std::addressof(gcoll->open_collector(gc_gen));
    }
    return coll;
  }

rcptr<Variable> Global_Context::create_variable(const Source_Location& sloc, const phsh_string& name, GC_Generation gc_hint) const
  {
    rcptr<Variable> var;
    // Allocate a variable from the collector.
    auto gcoll = this->get_tied_collector_opt();
    if(ROCKET_EXPECT(gcoll)) {
      var = gcoll->create_variable(gc_hint);
    }
    if(ROCKET_UNEXPECT(!var)) {
      var = rocket::make_refcnt<Variable>();
    }
    // Call the hook function if any.
    auto qh = this->get_hooks_opt();
    if(qh) {
      qh->on_variable_declare(sloc, name);
    }
    return var;
  }

size_t Global_Context::collect_variables(GC_Generation gc_limit) const
  {
    size_t nfreed = 0;
    // Perform garbage collection up to `gc_limit`.
    auto gcoll = this->get_tied_collector_opt();
    if(ROCKET_EXPECT(gcoll)) {
      nfreed = gcoll->collect_variables(gc_limit);
    }
    return nfreed;
  }

rcobj<Placeholder> Global_Context::placeholder() const noexcept
  {
    auto xph = rocket::dynamic_pointer_cast<Placeholder>(this->m_xph);
    ROCKET_ASSERT(xph);
    return rocket::move(xph);
  }

rcobj<Abstract_Opaque> Global_Context::placeholder_opaque() const noexcept
  {
    auto xph = rocket::dynamic_pointer_cast<Abstract_Opaque>(this->m_xph);
    ROCKET_ASSERT(xph);
    return rocket::move(xph);
  }

rcobj<Abstract_Function> Global_Context::placeholder_function() const noexcept
  {
    auto xph = rocket::dynamic_pointer_cast<Abstract_Function>(this->m_xph);
    ROCKET_ASSERT(xph);
    return rocket::move(xph);
  }

uint32_t Global_Context::get_random_uint32() const noexcept
  {
    auto prng = rocket::dynamic_pointer_cast<Random_Number_Generator>(this->m_prng);
    ROCKET_ASSERT(prng);
    return prng->bump();
  }

const Value& Global_Context::get_std_member(const phsh_string& name) const
  {
    auto stdv = rocket::dynamic_pointer_cast<Variable>(this->m_stdv);
    ROCKET_ASSERT(stdv);
    return stdv->get_value().as_object().get_or(name, null_value);
  }

Value& Global_Context::open_std_member(const phsh_string& name)
  {
    auto stdv = rocket::dynamic_pointer_cast<Variable>(this->m_stdv);
    ROCKET_ASSERT(stdv);
    return stdv->open_value().open_object().try_emplace(name).first->second;
  }

bool Global_Context::remove_std_member(const phsh_string& name)
  {
    auto stdv = rocket::dynamic_pointer_cast<Variable>(this->m_stdv);
    ROCKET_ASSERT(stdv);
    return stdv->open_value().open_object().erase(name);
  }

rcptr<Abstract_Hooks> Global_Context::get_hooks_opt() const noexcept
  {
    return rocket::dynamic_pointer_cast<Abstract_Hooks>(this->m_hooks_opt);
  }

rcptr<Abstract_Hooks> Global_Context::set_hooks(rcptr<Abstract_Hooks> hooks_opt) noexcept
  {
    return rocket::dynamic_pointer_cast<Abstract_Hooks>(std::exchange(this->m_hooks_opt, rocket::move(hooks_opt)));
  }

}  // namespace Asteria
