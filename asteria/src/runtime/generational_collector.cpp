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
    unsigned gen = 0;
    for(;;) {
      if(gen == gen_limit) {
        break;
      }
      // Go to the next generation.
      qcoll = qcoll->get_tied_collector_opt();
      if(!qcoll) {
        return nullptr;
      }
      ++gen;
    }
    return qcoll;
  }

RefCnt_Ptr<Variable> Generational_Collector::create_variable()
  {
    // Get one from the pool.
    auto var = this->m_pool.erase_random_opt();
    if(ROCKET_EXPECT(var)) {
      // Initialize it to `null`.
      var->reset(D_null(), true);
    } else {
      // Create a new one if the pool has been exhausted.
      var = rocket::make_refcnt<Variable>();
    }
    // The variable is alive now.
    this->m_gen_zero.track_variable(var);
    return var;
  }

bool Generational_Collector::collect_variables(unsigned gen_limit)
  {
    auto qcoll = &(this->m_gen_zero);
    // Force collection from the newest generation to the oldest.
    unsigned gen = 0;
    for(;;) {
      // Collect this generation.
      ASTERIA_DEBUG_LOG("Generation ", gen, " garbage collection begins.");
      qcoll->collect();
      ASTERIA_DEBUG_LOG("Generation ", gen, " garbage collection ends.");
      // Go to the next generation.
      if(gen == gen_limit) {
        break;
      }
      qcoll = qcoll->get_tied_collector_opt();
      if(!qcoll) {
        return false;
      }
      ++gen;
    }
    return true;
  }

}
