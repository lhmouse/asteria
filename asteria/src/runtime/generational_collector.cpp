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

Collector Generational_Collector::* Generational_Collector::do_locate(GC_Generation gc_gen) const
  {
    switch(gc_gen) {
      {{
    case gc_generation_newest:
        return &Generational_Collector::m_newest;
      }{
    case gc_generation_middle:
        return &Generational_Collector::m_middle;
      }{
    case gc_generation_oldest:
        return &Generational_Collector::m_oldest;
      }}
    default:
      ASTERIA_THROW("invalid GC generation (gc_gen `$1`)", gc_gen);
    }
  }

rcptr<Variable> Generational_Collector::create_variable(GC_Generation gc_hint)
  {
    // Locate the collector, which will be responsible for tracking the new variable.
    auto& coll = this->*(this->do_locate(gc_hint));
    // Try allocating a variable from the pool.
    auto var = this->m_pool.erase_random_opt();
    if(ROCKET_UNEXPECT(!var)) {
      // Create a new one if the pool has been exhausted.
      var = ::rocket::make_refcnt<Variable>();
    }
    coll.track_variable(var);
    return var;
  }

size_t Generational_Collector::collect_variables(GC_Generation gc_limit)
  {
    // Collect variables from the newest generation to the oldest.
    for(auto p = ::std::make_pair(::std::addressof(this->m_newest), gc_limit + 1);
           p.first && p.second; p.first = p.first->get_tied_collector_opt(), p.second--)
      p.first->collect_single_opt();
    // Clear the variable pool.
    auto nvars = this->m_pool.size();
    this->m_pool.clear();
    return nvars;
  }

}  // namespace Asteria
