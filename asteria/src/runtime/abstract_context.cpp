// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "abstract_context.hpp"
#include "generational_collector.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Abstract_Context::Cleaner::operator()(Rcbase* base) noexcept
  try {
    auto coll = rocket::dynamic_pointer_cast<Generational_Collector>(rcptr<Rcbase>(base));
    ROCKET_ASSERT(coll);
    coll->collect_variables(gc_generation_oldest);
  }
  catch(const std::exception& stdex) {
    ASTERIA_DEBUG_LOG("An exception was thrown during garbage collection and some resources might have leaked: ", stdex.what());
  }

Abstract_Context::~Abstract_Context()
  {
  }


Generational_Collector* Abstract_Context::get_tied_collector_opt() const noexcept
  {
    return dynamic_cast<Generational_Collector*>(this->m_coll_opt.get());
  }

void Abstract_Context::tie_collector(const rcptr<Generational_Collector>& coll_opt) noexcept
  {
    this->m_coll_opt.reset(rocket::static_pointer_cast<Rcbase>(coll_opt).release());
  }

}  // namespace Asteria
