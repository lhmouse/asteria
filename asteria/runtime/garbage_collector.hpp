// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GARBAGE_COLLECTOR_
#define ASTERIA_RUNTIME_GARBAGE_COLLECTOR_

#include "../fwd.hpp"
#include "../llds/variable_hashmap.hpp"
namespace asteria {

class Garbage_Collector
  :
    public rcfwd<Garbage_Collector>
  {
  private:
    int m_recur = 0;
    Variable_HashMap m_pool;  // key is a pointer to the `Variable` itself

    static constexpr uint32_t gMax = gc_generation_oldest;
    array<size_t, gMax+1> m_thres;
    array<size_t, gMax+1> m_counts = { };
    array<Variable_HashMap, gMax+1> m_tracked;

    Variable_HashMap m_staged;  // key is address of the owner of a `Variable`
    Variable_HashMap m_temp_1;  // key is address to a `Variable`
    Variable_HashMap m_temp_2;
    Variable_HashMap m_unreach;

  public:
    // Creates an empty garbage collector.
    Garbage_Collector() noexcept;

  private:
    inline
    size_t
    do_collect_generation(uint32_t gen);

  public:
    Garbage_Collector(const Garbage_Collector&) = delete;
    Garbage_Collector& operator=(const Garbage_Collector&) & = delete;
    ~Garbage_Collector();

    // accessors
    size_t
    get_threshold(GC_Generation gen) const
      { return this->m_thres.at(gMax-gen);  }

    void
    set_threshold(GC_Generation gen, size_t thres)
      { this->m_thres.mut(gMax-gen) = thres;  }

    size_t
    count_tracked_variables(GC_Generation gen) const
      { return this->m_tracked.at(gMax-gen).size();  }

    size_t
    count_pooled_variables() const noexcept
      { return this->m_pool.size();  }

    void
    clear_pooled_variables() noexcept
      { this->m_pool.clear();  }

    // These functions manage dynamic memory by managing variables. Variables
    // that are not created with `create_variable()` are 'foreign' and will never
    // be collected.
    refcnt_ptr<Variable>
    create_variable(GC_Generation gen_hint = gc_generation_newest);

    size_t
    collect_variables(GC_Generation gen_limit = gc_generation_oldest);

    size_t
    finalize() noexcept;
  };

}  // namespace asteria
#endif
