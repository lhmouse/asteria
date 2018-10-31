// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "global_collector.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Global_collector::~Global_collector()
  {
  }

rocket::refcounted_ptr<Variable> Global_collector::create_tracked_variable()
  {
    auto var = rocket::make_refcounted<Variable>(D_null(), true);
    this->m_gen_zero.track_variable(var);
    return var;
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
        break;
      }
      qcoll = qcoll->get_tied_collector_opt();
      if(!qcoll) {
        break;
      }
    } while(true);
  }

}
