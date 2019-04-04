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

Collector* Generational_Collector::get_collector_opt(unsigned generation) noexcept
  {
    auto coll = &(this->m_coll_newest);
    auto gcnt = generation;
    for(;;) {
      // Found?
      if(gcnt == 0) {
        break;
      }
      --gcnt;
      // Get the next generation.
      coll = coll->get_tied_collector_opt();
      if(!coll) {
        break;
      }
    }
    return coll;
  }

Rcptr<Variable> Generational_Collector::create_variable()
  {
    auto qvar = this->m_pool.erase_random_opt();
    if(ROCKET_UNEXPECT(!qvar)) {
      // Create a new one if the pool has been exhausted.
      qvar = rocket::make_refcnt<Variable>(Source_Location(rocket::sref("<fresh>"), 0));
    }
    // Track it so it can be collected when out of use.
    this->m_coll_newest.track_variable(qvar);
    return qvar;
  }

std::size_t Generational_Collector::collect_variables(unsigned generation_limit)
  {
    // Collect each generation.
    auto coll = &(this->m_coll_newest);
    auto gcnt = generation_limit;
    for(;;) {
      // Collect it.
      coll->collect_single_opt();
      // Found?
      if(gcnt == 0) {
        break;
      }
      --gcnt;
      // Get the next generation.
      coll = coll->get_tied_collector_opt();
      if(!coll) {
        break;
      }
    }
    // Clear the variable pool.
    auto nvars = this->m_pool.size();
    this->m_pool.clear();
    return nvars;
  }

}  // namespace Asteria
