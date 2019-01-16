// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "generational_collector.hpp"
#include "variable.hpp"
#include "reference.hpp"
#include "../utilities.hpp"

namespace Asteria {

Generational_Collector::~Generational_Collector()
  {
  }

Collector * Generational_Collector::get_collector_opt(unsigned gen_limit) noexcept
  {
    auto qcoll = &(this->m_gen_zero);
    // Find the collector with the given generation from the newest generation to the oldest.
    for(unsigned gen = 0; (gen < gen_limit) && qcoll; ++gen) {
      // Go to the next generation.
      qcoll = qcoll->get_tied_collector_opt();
    }
    return qcoll;
  }

rocket::refcounted_ptr<Variable> Generational_Collector::create_variable()
  {
    // Get one from the pool.
    auto var = this->m_pool.erase_random_opt();
    if(ROCKET_UNEXPECT(!var)) {
      // The pool has been exhausted. Create a new variable.
      var = rocket::make_refcounted<Variable>(D_null(), true);
    }
    // The variable is alive now.
    this->m_gen_zero.track_variable(var);
    return var;
  }

bool Generational_Collector::collect(unsigned gen_limit)
  {
    auto qcoll = &(this->m_gen_zero);
    // Force collection from the newest generation to the oldest.
    for(unsigned gen = 0; (gen < gen_limit) && qcoll; ++gen) {
      // Collect this generation.
      ASTERIA_DEBUG_LOG("Generation ", gen, " garbage collection begins.");
      qcoll->collect();
      ASTERIA_DEBUG_LOG("Generation ", gen, " garbage collection ends.");
      // Go to the next generation.
      qcoll = qcoll->get_tied_collector_opt();
    }
    return qcoll != nullptr;
  }

}
