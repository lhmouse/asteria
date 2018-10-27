// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "global_context.hpp"
#include "global_collector.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Global_context::Global_context()
  : m_gcoll(rocket::make_refcounted<Global_collector>())
  {
    ASTERIA_DEBUG_LOG("`Global_context` constructor: ", static_cast<void *>(this));
  }

Global_context::~Global_context()
  {
    ASTERIA_DEBUG_LOG("`Global_context` destructor: ", static_cast<void *>(this));
    // Perform the final garbage collection.
    try {
      this->m_dict.clear();
      this->m_gcoll->perform_garbage_collection(100);
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

const Reference * Global_context::get_named_reference_opt(const String &name) const
  {
    // Check for overriden references.
    const auto qit = this->m_dict.find(name);
    if(qit != this->m_dict.end()) {
      return &(qit->second);
    }
    // Check for references from the standard library.
    // TODO std
    return nullptr;
  }

void Global_context::set_named_reference(const String &name, Reference ref)
  {
    this->m_dict.insert_or_assign(name, std::move(ref));
  }

rocket::refcounted_ptr<Variable> Global_context::create_tracked_variable()
  {
    return this->m_gcoll->create_tracked_variable();
  }

void Global_context::perform_garbage_collection(unsigned gen_limit)
  {
    return this->m_gcoll->perform_garbage_collection(gen_limit);
  }

}
