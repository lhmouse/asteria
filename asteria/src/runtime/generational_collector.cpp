// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "generational_collector.hpp"
#include "variable.hpp"
#include "reference.hpp"
#include "../utilities.hpp"

namespace Asteria {

Generational_collector::~Generational_collector()
  {
  }

rocket::refcounted_ptr<Variable> Generational_collector::create_variable()
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

void Generational_collector::perform_garbage_collection(unsigned gen_limit)
  {
    // Force collection from the newest generation to the oldest.
    unsigned gen_cur = 0;
    auto qcoll = &(this->m_gen_zero);
    do {
      ASTERIA_DEBUG_LOG("Generation ", gen_cur, " garbage collection begins.");
      qcoll->collect();
      ASTERIA_DEBUG_LOG("Generation ", gen_cur, " garbage collection ends.");
    } while((gen_cur++ < gen_limit) && (qcoll = qcoll->get_tied_collector_opt()));
  }

}
