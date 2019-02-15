// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_COLLECTOR_HPP_
#define ASTERIA_RUNTIME_COLLECTOR_HPP_

#include "../fwd.hpp"
#include "variable_hashset.hpp"

namespace Asteria {

class Collector
  {
  private:
    Variable_HashSet *m_output_opt;
    Collector *m_tied_opt;
    unsigned m_threshold;

    unsigned m_counter;
    long m_recur;
    Variable_HashSet m_tracked;
    Variable_HashSet m_staging;

  public:
    Collector(Variable_HashSet *output_opt, Collector *tied_opt, unsigned threshold) noexcept
      : m_output_opt(output_opt), m_tied_opt(tied_opt), m_threshold(threshold),
        m_counter(0), m_recur(0)
      {
      }

    Collector(const Collector &)
      = delete;
    Collector & operator=(const Collector &)
      = delete;

  private:
    Collector * do_collect_once();

  public:
    Variable_HashSet * get_output_pool_opt() const noexcept
      {
        return this->m_output_opt;
      }
    void set_output_pool(Variable_HashSet *output_opt) noexcept
      {
        this->m_output_opt = output_opt;
      }

    Collector * get_tied_collector_opt() const noexcept
      {
        return this->m_tied_opt;
      }
    void tie_collector(Collector *tied_opt) noexcept
      {
        this->m_tied_opt = tied_opt;
      }

    unsigned get_threshold() const noexcept
      {
        return this->m_threshold;
      }
    void set_threshold(unsigned threshold) noexcept
      {
        this->m_threshold = threshold;
      }

    std::size_t get_tracked_variable_count() const noexcept
      {
        return this->m_tracked.size();
      }
    bool track_variable(const RefCnt_Ptr<Variable> &var);
    bool untrack_variable(const RefCnt_Ptr<Variable> &var) noexcept;
    void collect();
  };

}

#endif
