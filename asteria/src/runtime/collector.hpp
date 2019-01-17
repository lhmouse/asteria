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
    Variable_Hashset *m_output_opt;
    Collector *m_tied_opt;
    unsigned m_threshold;

    unsigned m_counter;
    long m_recur;
    Variable_Hashset m_tracked;
    Variable_Hashset m_staging;

  public:
    Collector(Variable_Hashset *output_opt, Collector *tied_opt, unsigned threshold) noexcept
      : m_output_opt(output_opt), m_tied_opt(tied_opt), m_threshold(threshold),
        m_counter(0), m_recur(0)
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Collector);

  public:
    Variable_Hashset * get_output_pool_opt() const noexcept
      {
        return this->m_output_opt;
      }
    void set_output_pool(Variable_Hashset *output_opt) noexcept
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

    bool track_variable(const RefCnt_Ptr<Variable> &var);
    bool untrack_variable(const RefCnt_Ptr<Variable> &var) noexcept;
    void collect();
  };

}

#endif
