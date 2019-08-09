// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GENERATIONAL_COLLECTOR_HPP_
#define ASTERIA_RUNTIME_GENERATIONAL_COLLECTOR_HPP_

#include "../fwd.hpp"
#include "rcbase.hpp"
#include "collector.hpp"
#include "../llds/variable_hashset.hpp"

namespace Asteria {

class Generational_Collector final : public virtual Rcbase
  {
  private:
    Variable_HashSet m_pool;
    sso_vector<Collector, 3> m_colls;

  public:
    Generational_Collector() noexcept
      {
        this->m_colls.emplace_back(&(this->m_pool), nullptr,                      10);
        this->m_colls.emplace_back(&(this->m_pool), &(this->m_colls.mut_back()),  50);
        this->m_colls.emplace_back(&(this->m_pool), &(this->m_colls.mut_back()), 500);
      }
    ~Generational_Collector() override;

    Generational_Collector(const Generational_Collector&)
      = delete;
    Generational_Collector& operator=(const Generational_Collector&)
      = delete;

  public:
    size_t get_pool_size() const noexcept
      {
        return this->m_pool.size();
      }
    size_t count_collectors() const noexcept
      {
        return this->m_colls.size();
      }
    const Collector& get_collector(size_t gen) const
      {
        return this->m_colls.at(gen);
      }
    Collector& mut_collector(size_t gen)
      {
        return this->m_colls.mut(gen);
      }

    rcptr<Variable> create_variable(size_t gen_limit);
    size_t collect_variables(size_t gen_limit);
  };

}  // namespace Asteria

#endif
