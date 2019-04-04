// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GENERATIONAL_COLLECTOR_HPP_
#define ASTERIA_RUNTIME_GENERATIONAL_COLLECTOR_HPP_

#include "../fwd.hpp"
#include "rcbase.hpp"
#include "variable_hashset.hpp"
#include "collector.hpp"

namespace Asteria {

class Generational_Collector : public virtual Rcbase
  {
  private:
    Variable_HashSet m_pool;
    Collector m_coll_oldest;
    Collector m_coll_middle;
    Collector m_coll_newest;

  public:
    Generational_Collector()
      : m_pool(),
        m_coll_oldest(&(this->m_pool), nullptr, 10),
        m_coll_middle(&(this->m_pool), &(this->m_coll_oldest), 50),
        m_coll_newest(&(this->m_pool), &(this->m_coll_middle), 500)
      {
      }
    ~Generational_Collector() override;

    Generational_Collector(const Generational_Collector&)
      = delete;
    Generational_Collector& operator=(const Generational_Collector&)
      = delete;

  public:
    Collector* get_collector_opt(unsigned generation) noexcept;
    Rcptr<Variable> create_variable();
    std::size_t collect_variables(unsigned generation_limit);
  };

}  // namespace Asteria

#endif
