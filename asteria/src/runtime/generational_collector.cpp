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

bool Generational_Collector::perform_garbage_collection(unsigned gen_limit)
  {
    // Force collection from the newest generation to the oldest.
    auto gen_cur = unsigned(0);
    auto qcoll = &(this->m_gen_zero);
    do {
      ASTERIA_DEBUG_LOG("Generation ", gen_cur, " garbage collection begins.");
      qcoll->collect();
      ASTERIA_DEBUG_LOG("Generation ", gen_cur, " garbage collection ends.");
      // Stop at `gen_limit + 1`.
      gen_cur++;
      if(gen_cur > gen_limit) {
        return true;
      }
      // Collect the next generation.
      qcoll = qcoll->get_tied_collector_opt();
      if(!qcoll) {
        return false;
      }
    } while(true);
  }

}
