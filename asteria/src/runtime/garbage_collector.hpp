// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GARBAGE_COLLECTOR_HPP_
#define ASTERIA_RUNTIME_GARBAGE_COLLECTOR_HPP_

#include "../fwd.hpp"
#include "../llds/variable_hashset.hpp"
#include "../llds/pointer_hashset.hpp"

namespace asteria {

class Garbage_Collector final
  : public Rcfwd<Garbage_Collector>
  {
  public:
    enum : uint8_t {
      generation_youngest  = 0,
      generation_oldest    = 2,
    };

  private:
    static constexpr size_t gMax = generation_oldest;
    array<size_t, gMax+1> m_thres = { 10, 70, 500 };
    array<Variable_HashSet, gMax+1> m_tracked;

    long m_recur = 0;
    Variable_HashSet m_pool;
    Variable_HashSet m_staging;
    Pointer_HashSet m_id_ptrs;

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
    get_threshold(uint8_t gen) const
      { return this->m_thres.at(gMax-gen);  }

    Garbage_Collector&
    set_threshold(uint8_t gen, size_t thres)
      { return this->m_thres.mut(gMax-gen) = thres, *this;  }

    size_t
    count_tracked_variables(uint8_t gen) const
      { return this->m_tracked.at(gMax-gen).size();  }

    size_t
    count_pooled_variables() const noexcept
      { return this->m_pool.size();  }

    Garbage_Collector&
    clear_pooled_variables() noexcept
      { return this->m_pool.clear(), *this;  }

    // Allocation and collection
    rcptr<Variable>
    create_variable(uint8_t gen_hint = generation_youngest);

    size_t
    collect_variables(uint8_t gen_limit = generation_oldest);

    size_t
    finalize() noexcept;
  };

}  // namespace asteria

#endif
