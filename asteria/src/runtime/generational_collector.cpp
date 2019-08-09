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

rcptr<Variable> Generational_Collector::create_variable(size_t gen_limit)
  {
    auto rlimit = rocket::min(gen_limit, this->m_colls.size() - 1);
    // Get a variable from the pool.
    auto qvar = this->m_pool.erase_random_opt();
    if(ROCKET_UNEXPECT(!qvar)) {
      // Create a new one if the pool has been exhausted.
      qvar = rocket::make_refcnt<Variable>();
    }
    // Track this variable.
    this->m_colls.mut(rlimit).track_variable(qvar);
    // Return it.
    return qvar;
  }

size_t Generational_Collector::collect_variables(size_t gen_limit)
  {
    auto rlimit = rocket::min(gen_limit, this->m_colls.size() - 1);
    // Collect variables from the newest generation to the oldest generation.
    for(size_t gen = 0; gen <= rlimit; ++gen) {
      // Collect it.
      this->m_colls.mut(gen).collect_single_opt();
    }
    // Clear the variable pool.
    auto nvars = this->m_pool.size();
    this->m_pool.clear();
    return nvars;
  }

}  // namespace Asteria
