// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "genius_collector.hpp"
#include "variable.hpp"
#include "reference.hpp"
#include "../utils.hpp"

namespace asteria {

Genius_Collector::
~Genius_Collector()
  {
  }

Collector Genius_Collector::*
Genius_Collector::
do_locate(GC_Generation gc_gen)
  const
  {
    switch(gc_gen) {
      case gc_generation_newest:
        return &Genius_Collector::m_newest;

      case gc_generation_middle:
        return &Genius_Collector::m_middle;

      case gc_generation_oldest:
        return &Genius_Collector::m_oldest;

      default:
        ASTERIA_THROW("Invalid GC generation (gc_gen `$1`)", gc_gen);
    }
  }

rcptr<Variable>
Genius_Collector::
create_variable(GC_Generation gc_hint)
  {
    // Locate the collector, which will be responsible for tracking the new variable.
    auto& coll = this->*(this->do_locate(gc_hint));

    // Try allocating a variable from the pool.
    auto var = this->m_pool.erase_random_opt();
    if(ROCKET_UNEXPECT(!var))
      var = ::rocket::make_refcnt<Variable>();
    coll.track_variable(var);

    // Mark it uninitialized.
    var->uninitialize();
    return var;
  }

size_t
Genius_Collector::
collect_variables(GC_Generation gc_limit)
  {
    // Collect variables from the newest generation to the oldest.
    for(auto p = ::std::make_pair(&(this->m_newest), gc_limit + 1);
          p.first && p.second;  p.first = p.first->get_tied_collector_opt(), p.second--)
      p.first->collect_single_opt();

    // Clear the variable pool.
    auto nvars = this->m_pool.size();
    this->m_pool.clear();
    return nvars;
  }

Genius_Collector&
Genius_Collector::
wipe_out_variables()
  noexcept
  {
    // Uninitialize all variables recursively.
    this->m_newest.wipe_out_variables();
    this->m_middle.wipe_out_variables();
    this->m_oldest.wipe_out_variables();
    return *this;
  }

}  // namespace asteria
