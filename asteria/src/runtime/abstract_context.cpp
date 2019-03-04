// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "abstract_context.hpp"
#include "generational_collector.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Abstract_Context::Collection_Trigger::operator()(Rcbase *base_opt) noexcept
  try {
    // Take ownership of the argument.
    auto collector = rocket::dynamic_pointer_cast<Generational_Collector>(Rcptr<Rcbase>(base_opt));
    if(!collector) {
      return;
    }
    // Collect all generations.
    collector->collect_variables(0x7F);
    // Finish.
  } catch(std::exception &stdex) {
    ASTERIA_DEBUG_LOG("An exception was thrown during the final garbage collection; some resources might have leaked: ", stdex.what());
  }

Abstract_Context::~Abstract_Context()
  {
  }

void Abstract_Context::do_tie_collector(Rcptr<Generational_Collector> tied_collector_opt) noexcept
  {
    this->m_tied_collector_opt.reset(tied_collector_opt.release());
  }

void Abstract_Context::do_set_named_reference_templates(const std::pair<Cow_String, Reference> *tdata_opt, std::size_t tsize) noexcept
  {
    this->m_named_references.set_templates(tdata_opt, tsize);
  }

}
