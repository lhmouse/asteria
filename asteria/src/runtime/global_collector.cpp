// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "global_collector.hpp"
#include "reference.hpp"
#include "../utilities.hpp"

namespace Asteria {

Global_collector::~Global_collector()
  {
  }

void Global_collector::track_variable(const rocket::refcounted_ptr<Variable> &var)
  {
    ROCKET_ASSERT(var);
    if(!this->m_gen_zero.track_variable(var)) {
      ASTERIA_THROW_RUNTIME_ERROR("A variable can only be added at most once.");
    }
  }

bool Global_collector::untrack_variable(const rocket::refcounted_ptr<Variable> &var) noexcept
  {
    auto qcoll = &(this->m_gen_zero);
    do {
      if(qcoll->untrack_variable(var)) {
        return true;
      }
      qcoll = qcoll->get_tied_collector_opt();
      if(!qcoll) {
        return false;
      }
    } while(true);
  }

void Global_collector::perform_garbage_collection(unsigned gen_limit)
  {
    auto qcoll = &(this->m_gen_zero);
    unsigned gen_cur = 0;
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
