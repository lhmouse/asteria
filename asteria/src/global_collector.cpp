// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "global_collector.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Global_collector::~Global_collector()
  try {
    this->perform_garbage_collection(100, true);
  } catch(std::exception &e) {
    ASTERIA_DEBUG_LOG("An exception was thrown while performing the final GC and some variables might have leaked: ", e.what());
    return;
  }

rocket::refcounted_ptr<Variable> Global_collector::create_tracked_variable()
  {
    auto var = rocket::make_refcounted<Variable>();
    this->m_gen_zero.track_variable(var);
    return var;
  }

void Global_collector::perform_garbage_collection(unsigned gen_max, bool unreserve)
  {
    auto qcoll = &(this->m_gen_zero);
    auto gen_cur = gen_max;
    do {
      ASTERIA_DEBUG_LOG("Performing garbage collection: generation = ", gen_cur, ", unreserve = ", unreserve);
      qcoll->collect(unreserve);
      if(gen_cur == 0) {
        break;
      }
      --gen_cur;
      qcoll = qcoll->get_tied_collector_opt();
      if(!qcoll) {
        break;
      }
    } while(true);
  }

}
