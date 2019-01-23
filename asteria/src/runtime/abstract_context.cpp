// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "abstract_context.hpp"
#include "generational_collector.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Abstract_Context::Collection_Trigger::operator()(RefCnt_Base *base_opt) noexcept
  try {
    const auto collector = rocket::dynamic_pointer_cast<Generational_Collector>(RefCnt_Ptr<RefCnt_Base>(base_opt));
    if(!collector) {
      return;
    }
    collector->collect(UINT_MAX);
  } catch(std::exception &e) {
    ASTERIA_DEBUG_LOG("An exception was thrown during the final garbage collection and some resources might have leaked: ", e.what());
  }

Abstract_Context::~Abstract_Context()
  {
  }

void Abstract_Context::do_tie_collector(RefCnt_Ptr<Generational_Collector> tied_collector_opt) noexcept
  {
    this->m_tied_collector_opt.reset(tied_collector_opt.release());
  }

void Abstract_Context::do_set_named_reference_templates(const Reference_Dictionary::Template *tdata_opt, std::size_t tsize) noexcept
  {
    this->m_named_references.set_templates(tdata_opt, tsize);
  }

}
