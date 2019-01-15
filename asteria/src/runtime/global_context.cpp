// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "global_context.hpp"
#include "generational_collector.hpp"
#include "variable.hpp"
#include "../utilities.hpp"

namespace Asteria {

Global_context::~Global_context()
  {
    // Perform the final garbage collection.
    const auto coll = static_cast<Generational_collector *>(this->m_coll.get());
    if(coll) {
      try {
        this->clear_named_references();
        coll->perform_garbage_collection(UINT_MAX);
      } catch(std::exception &e) {
        ASTERIA_DEBUG_LOG("An exception was thrown during the final garbage collection and some resources might have leaked: ", e.what());
      }
    }
  }

void Global_context::do_initialize()
  {
    // Initialize all components.
    const auto coll = rocket::make_refcounted<Generational_collector>();
    this->m_coll = coll;
    // Add standard library interfaces.
    D_object root;
    ASTERIA_DEBUG_LOG("TODO add std library");
    Reference_root::S_constant ref_c = { std::move(root) };
  }

rocket::refcounted_ptr<Variable> Global_context::create_variable()
  {
    const auto coll = static_cast<Generational_collector *>(this->m_coll.get());
    ROCKET_ASSERT(coll);
    return coll->create_variable();
  }

}
