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
    Variable_HashSet* m_output_opt;
    Collector* m_tied_opt;
    std::size_t m_threshold;

    std::size_t m_counter;
    long m_recur;
    Variable_HashSet m_tracked;
    Variable_HashSet m_staging;

  public:
    Collector(Variable_HashSet* output_opt, Collector* tied_opt, std::size_t threshold) noexcept
      : m_output_opt(output_opt), m_tied_opt(tied_opt), m_threshold(threshold),
        m_counter(0), m_recur(0)
      {
      }

    Collector(const Collector&)
      = delete;
    Collector& operator=(const Collector&)
      = delete;

  public:
    Variable_HashSet* getput_pool_opt() const noexcept
      {
        return this->m_output_opt;
      }
    void setput_pool(Variable_HashSet* output_opt) noexcept
      {
        this->m_output_opt = output_opt;
      }

    Collector* get_tied_collector_opt() const noexcept
      {
        return this->m_tied_opt;
      }
    void tie_collector(Collector* tied_opt) noexcept
      {
        this->m_tied_opt = tied_opt;
      }

    std::size_t get_threshold() const noexcept
      {
        return this->m_threshold;
      }
    void set_threshold(std::size_t threshold) noexcept
      {
        this->m_threshold = threshold;
      }

    std::size_t get_tracked_variable_count() const noexcept
      {
        return this->m_tracked.size();
      }
    bool track_variable(const Rcptr<Variable>& var);
    bool untrack_variable(const Rcptr<Variable>& var) noexcept;
    Collector* collect_single_opt();
  };

}  // namespace Asteria

#endif
