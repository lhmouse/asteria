// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "generational_collector.hpp"
#include "reference.hpp"
#include "../utilities.hpp"

namespace Asteria {

Generational_collector::~Generational_collector()
  {
  }

bool Generational_collector::track_variable(const rocket::refcounted_ptr<Variable> &var)
  {
    return this->m_gen_zero.track_variable(var);
  }

void Generational_collector::perform_garbage_collection(unsigned gen_limit)
  {
    unsigned gen_cur = 0;
    auto qcoll = &(this->m_gen_zero);
    do {
      ASTERIA_DEBUG_LOG("Generation ", gen_cur, " garbage collection begins.");
      qcoll->collect();
      ASTERIA_DEBUG_LOG("Generation ", gen_cur, " garbage collection ends.");
      ++gen_cur;
      if(gen_cur > gen_limit) {
        return;
      }
      qcoll = qcoll->get_tied_collector_opt();
      if(!qcoll) {
        return;
      }
    } while(true);
  }

}
