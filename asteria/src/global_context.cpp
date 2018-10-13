// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "global_context.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Global_context::~Global_context()
  {
  }

bool Global_context::is_analytic() const noexcept
  {
    return false;
  }
const Abstract_context * Global_context::get_parent_opt() const noexcept
  {
    return nullptr;
  }

rocket::refcounted_ptr<Variable> Global_context::create_tracked_variable()
  {
    auto coll = this->m_collector_opt;
    if(!coll) {
      coll = rocket::make_refcounted<Global_collector>();
      this->m_collector_opt = coll;
    }
    return coll->create_tracked_variable();
  }

void Global_context::perform_garbage_collection(unsigned gen_max, bool unreserve)
  {
    auto coll = this->m_collector_opt;
    if(!coll) {
      return;
    }
    return coll->perform_garbage_collection(gen_max, unreserve);
  }

}
