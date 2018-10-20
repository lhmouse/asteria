// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COLLECTOR_HPP_
#define ASTERIA_COLLECTOR_HPP_

#include "fwd.hpp"
#include "variable_hashset.hpp"

namespace Asteria {

class Collector
  {
  private:
    Collector *m_tied_opt;
    Size m_threshold;
    Size m_counter;
    long m_recur;
    Variable_hashset m_tracked;
    Variable_hashset m_staging;

  public:
    Collector(Collector *tied_opt, Size threshold) noexcept
      : m_tied_opt(tied_opt), m_threshold(threshold), m_counter(0), m_recur(0)
      {
      }
    ~Collector();

    Collector(const Collector &)
      = delete;
    Collector & operator=(const Collector &)
      = delete;

  public:
    Collector * get_tied_collector_opt() const noexcept
      {
        return this->m_tied_opt;
      }
    void tie_collector(Collector *tied_opt) noexcept
      {
        this->m_tied_opt = tied_opt;
      }

    Size get_threshold() const noexcept
      {
        return this->m_threshold;
      }
    void set_threshold(Size threshold) noexcept
      {
        this->m_threshold = threshold;
      }

    bool track_variable(const rocket::refcounted_ptr<Variable> &var);
    bool untrack_variable(const rocket::refcounted_ptr<Variable> &var) noexcept;

    bool auto_collect();
    void collect();
  };

}

#endif
