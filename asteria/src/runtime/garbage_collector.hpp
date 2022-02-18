// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GARBAGE_COLLECTOR_HPP_
#define ASTERIA_RUNTIME_GARBAGE_COLLECTOR_HPP_

#include "../fwd.hpp"
#include "../llds/variable_hashmap.hpp"

namespace asteria {

class Garbage_Collector final
  : public Rcfwd<Garbage_Collector>
  {
  private:
    long m_recur = 0;
    Variable_HashMap m_pool;  // key is a pointer to the `Variable` itself

    static constexpr size_t gMax = gc_generation_oldest;
    array<size_t, gMax+1> m_counts = { };
    array<size_t, gMax+1> m_thres = { 10, 70, 500 };
    array<Variable_HashMap, gMax+1> m_tracked;

    Variable_HashMap m_staged;  // key is address of the owner of a `Variable`
    Variable_HashMap m_temp_1;  // key is address to a `Variable`
    Variable_HashMap m_temp_2;
    Variable_HashMap m_unreach;

  public:
    explicit
    Garbage_Collector() noexcept
      { }

  private:
    inline size_t
    do_collect_generation(size_t gen);

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Garbage_Collector);

    // Properties
    size_t
    get_threshold(GC_Generation gen) const
      { return this->m_thres.at(gMax-gen);  }

    Garbage_Collector&
    set_threshold(GC_Generation gen, size_t thres)
      { this->m_thres.mut(gMax-gen) = thres;  return *this;  }

    size_t
    count_tracked_variables(GC_Generation gen) const
      { return this->m_tracked.at(gMax-gen).size();  }

    size_t
    count_pooled_variables() const noexcept
      { return this->m_pool.size();  }

    Garbage_Collector&
    clear_pooled_variables() noexcept
      { this->m_pool.clear();  return *this;  }

    // Allocation and collection
    rcptr<Variable>
    create_variable(GC_Generation gen_hint = gc_generation_newest);

    size_t
    collect_variables(GC_Generation gen_limit = gc_generation_oldest);

    size_t
    finalize() noexcept;
  };

}  // namespace asteria

#endif
