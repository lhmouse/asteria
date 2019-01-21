// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "abstract_context.hpp"
#include "generational_collector.hpp"
#include "../utilities.hpp"

namespace Asteria {

Abstract_Context::~Abstract_Context()
  {
    const auto collector = rocket::dynamic_pointer_cast<Generational_Collector>(std::move(this->m_tied_collector_opt));
    if(collector) {
      // Perform the final garbage collection.
      this->m_named_references.clear();
      try {
        collector->collect(UINT_MAX);
      } catch(std::exception &e) {
        ASTERIA_DEBUG_LOG("An exception was thrown during the final garbage collection and some resources might have leaked: ", e.what());
      }
    }
  }

void Abstract_Context::do_tie_collector(const RefCnt_Ptr<Generational_Collector> &tied_collector_opt) noexcept
  {
    this->m_tied_collector_opt = tied_collector_opt;
  }

void Abstract_Context::do_set_named_reference_templates(const Reference_Dictionary::Template *tdata_opt, std::size_t tsize) noexcept
  {
    this->m_named_references.set_templates(tdata_opt, tsize);
  }

}
