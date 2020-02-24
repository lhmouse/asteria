// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_COLLECTOR_HPP_
#define ASTERIA_RUNTIME_COLLECTOR_HPP_

#include "../fwd.hpp"
#include "../llds/variable_hashset.hpp"

namespace Asteria {

class Collector
  {
  private:
    Variable_HashSet* m_output_opt;
    Collector* m_tied_opt;
    uint32_t m_threshold;

    uint32_t m_counter = 0;
    long m_recur = 0;
    Variable_HashSet m_tracked;
    Variable_HashSet m_staging;

  public:
    Collector(Variable_HashSet* output_opt, Collector* tied_opt, uint32_t threshold) noexcept
      :
        m_output_opt(output_opt), m_tied_opt(tied_opt), m_threshold(threshold)
      {
      }

    Collector(const Collector&)
      = delete;
    Collector& operator=(const Collector&)
      = delete;

  public:
    Variable_HashSet* get_output_pool_opt() const noexcept
      {
        return this->m_output_opt;
      }
    Collector& set_output_pool(Variable_HashSet* output_opt) noexcept
      {
        return this->m_output_opt = output_opt, *this;
      }

    Collector* get_tied_collector_opt() const noexcept
      {
        return this->m_tied_opt;
      }
    Collector& tie_collector(Collector* tied_opt) noexcept
      {
        return this->m_tied_opt = tied_opt, *this;
      }

    uint32_t get_threshold() const noexcept
      {
        return this->m_threshold;
      }
    Collector& set_threshold(uint32_t threshold) noexcept
      {
        return this->m_threshold = threshold, *this;
      }

    size_t count_tracked_variables() const noexcept
      {
        return this->m_tracked.size();
      }
    bool track_variable(const rcptr<Variable>& var);
    bool untrack_variable(const rcptr<Variable>& var) noexcept;

    Collector* collect_single_opt();
    Collector& wipe_out_variables() noexcept;
  };

}  // namespace Asteria

#endif
