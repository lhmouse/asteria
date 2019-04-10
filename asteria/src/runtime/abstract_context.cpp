// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "abstract_context.hpp"
#include "generational_collector.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Abstract_Context::Collection_Trigger::operator()(Rcbase* base_opt) noexcept
  try {
    // Take ownership of the argument.
    auto qcoll = rocket::dynamic_pointer_cast<Generational_Collector>(Rcptr<Rcbase>(base_opt));
    if(qcoll) {
      qcoll->collect_variables(UINT_MAX);
    }
  } catch(std::exception& stdex) {
    ASTERIA_DEBUG_LOG("An exception was thrown during the final garbage collection; some resources might have leaked: ", stdex.what());
  }

Abstract_Context::~Abstract_Context()
  {
  }

void Abstract_Context::tie_collector(Rcptr<Generational_Collector> coll_opt) noexcept
  {
    this->m_tied_coll_opt.reset(coll_opt.release());
  }

}  // namespace Asteria
