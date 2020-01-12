// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "abstract_context.hpp"
#include "generational_collector.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Abstract_Context::Cleaner::operator()(Rcbase* base) noexcept
  try {
    auto coll = ::rocket::dynamic_pointer_cast<Generational_Collector>(rcptr<Rcbase>(base));
    ROCKET_ASSERT(coll);
    coll->collect_variables(gc_generation_oldest);
  }
  catch(exception& stdex) {
    // Ignore this exception, but notify the user about this error.
    ::fprintf(stderr, "-- WARNING: garbage collection failed: %s\n", stdex.what());
  }

Abstract_Context::~Abstract_Context()
  {
  }

rcptr<Generational_Collector> Abstract_Context::get_tied_collector_opt() const noexcept
  {
    return this->m_coll_opt ? this->m_coll_opt->share_this<Generational_Collector>() : nullptr;
  }

void Abstract_Context::tie_collector(const rcptr<Generational_Collector>& coll_opt) noexcept
  {
    this->m_coll_opt.reset(::rocket::static_pointer_cast<Rcbase>(coll_opt).release());
  }

}  // namespace Asteria
