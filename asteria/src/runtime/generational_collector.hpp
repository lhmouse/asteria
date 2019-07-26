// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GENERATIONAL_COLLECTOR_HPP_
#define ASTERIA_RUNTIME_GENERATIONAL_COLLECTOR_HPP_

#include "../fwd.hpp"
#include "rcbase.hpp"
#include "collector.hpp"
#include "../llds/variable_hashset.hpp"

namespace Asteria {

class Generational_Collector : public virtual Rcbase
  {
  private:
    Variable_HashSet m_pool;
    Static_Vector<Collector, 3> m_colls;

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
    std::size_t pool_size() const noexcept
      {
        return this->m_pool.size();
      }
    std::size_t collector_count() const noexcept
      {
        return this->m_colls.size();
      }
    const Collector& collector(std::size_t gindex) const
      {
        return this->m_colls.at(gindex);
      }
    Collector& collector(std::size_t gindex)
      {
        return this->m_colls.mut(gindex);
      }

    Rcptr<Variable> create_variable(std::size_t glimit);
    std::size_t collect_variables(std::size_t glimit);
  };

}  // namespace Asteria

#endif
