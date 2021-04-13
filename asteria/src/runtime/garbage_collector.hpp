// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GARBAGE_COLLECTOR_HPP_
#define ASTERIA_RUNTIME_GARBAGE_COLLECTOR_HPP_

#include "../fwd.hpp"
#include "collector.hpp"
#include "../llds/variable_hashset.hpp"

namespace asteria {

class Garbage_Collector final
  : public Rcfwd<Garbage_Collector>
  {
  private:
    // Mind the order of construction and destruction.
    Variable_HashSet m_pool;
    Collector m_oldest;
    Collector m_middle;
    Collector m_newest;

  public:
    explicit
    Garbage_Collector() noexcept
      : m_oldest(&(this->m_pool), nullptr, 10),
        m_middle(&(this->m_pool), &(this->m_oldest), 60),
        m_newest(&(this->m_pool), &(this->m_middle), 800)
      { }

  private:
    Collector Garbage_Collector::*
    do_locate(size_t gc_gen) const;

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Garbage_Collector);

    size_t
    get_pool_size() const noexcept
      { return this->m_pool.size();  }

    Garbage_Collector&
    clear_pool() noexcept
      { return this->m_pool.clear(), *this;  }

    const Collector&
    get_collector(size_t gc_gen) const
      { return this->*(this->do_locate(gc_gen));  }

    Collector&
    open_collector(size_t gc_gen)
      { return this->*(this->do_locate(gc_gen));  }

    rcptr<Variable>
    create_variable(size_t gc_hint = 0);

    size_t
    collect_variables(size_t gc_limit = 2);

    Garbage_Collector&
    wipe_out_variables() noexcept;
  };

}  // namespace asteria

#endif
