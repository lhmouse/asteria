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

rocket::unique_ptr<Executive_context> Global_context::allocate_executive_context()
  {
    rocket::unique_ptr<Executive_context> ctx;
    if(ROCKET_UNEXPECT(this->m_ectx_pool.empty())) {
      ctx = rocket::make_unique<Executive_context>(nullptr);
    } else {
      ctx = std::move(this->m_ectx_pool.mut_back());
      this->m_ectx_pool.pop_back();
    }
    return ctx;
  }

bool Global_context::return_executive_context(rocket::unique_ptr<Executive_context> &&ctx) noexcept
  try {
    ROCKET_ASSERT(ctx);
    ctx->clear_named_references(this->m_gen_coll.get());
    this->m_ectx_pool.emplace_back(std::move(ctx));
    return true;
  } catch(std::exception &e) {
    ASTERIA_DEBUG_LOG("Failed to return context: ", e.what());
    return false;
  }

rocket::unique_ptr<Reference_stack> Global_context::allocate_reference_stack()
  {
    rocket::unique_ptr<Reference_stack> stack;
    if(ROCKET_UNEXPECT(this->m_stack_pool.empty())) {
      stack = rocket::make_unique<Reference_stack>();
    } else {
      stack = std::move(this->m_stack_pool.mut_back());
      this->m_stack_pool.pop_back();
    }
    return stack;
  }

bool Global_context::return_reference_stack(rocket::unique_ptr<Reference_stack> &&stack) noexcept
  try {
    ROCKET_ASSERT(stack);
    stack->clear(this->m_gen_coll.get());
    this->m_stack_pool.emplace_back(std::move(stack));
    return true;
  } catch(std::exception &e) {
    ASTERIA_DEBUG_LOG("Failed to return stack: ", e.what());
    return false;
  }

}
