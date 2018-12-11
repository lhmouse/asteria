// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "global_context.hpp"
#include "generational_collector.hpp"
#include "variable.hpp"
#include "executive_context.hpp"
#include "reference_stack.hpp"
#include "../utilities.hpp"

namespace Asteria {

Global_context::Global_context()
  {
    ASTERIA_DEBUG_LOG("`Global_context` constructor: ", static_cast<void *>(this));
    // Create the global garbage collector.
    this->m_gen_coll = rocket::make_refcounted<Generational_collector>();
  }

Global_context::~Global_context()
  {
    ASTERIA_DEBUG_LOG("`Global_context` destructor: ", static_cast<void *>(this));
    // Perform the final garbage collection.
    try {
      this->clear_named_references(this->m_gen_coll.get());
      this->m_gen_coll->perform_garbage_collection(100);
    } catch(std::exception &e) {
      ASTERIA_DEBUG_LOG("An exception was thrown during final garbage collection and some resources might have leaked: ", e.what());
    }
  }

bool Global_context::is_analytic() const noexcept
  {
    return false;
  }

const Abstract_context * Global_context::get_parent_opt() const noexcept
  {
    return nullptr;
  }

}
