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
    Collector m_gen_two;
    Collector m_gen_one;
    Collector m_gen_zero;

  public:
    Generational_Collector() noexcept
      : m_pool(),
        m_gen_two(&(this->m_pool), nullptr, 10),
        m_gen_one(&(this->m_pool), &(this->m_gen_two), 50),
        m_gen_zero(&(this->m_pool), &(this->m_gen_one), 500)
      {
      }
    ~Generational_Collector() override;

    Generational_Collector(const Generational_Collector &)
      = delete;
    Generational_Collector & operator=(const Generational_Collector &)
      = delete;

  public:
    Collector * get_collector_opt(unsigned gen_limit) noexcept;

    Rcptr<Variable> create_variable();
    bool collect_variables(unsigned gen_limit);
  };

}  // namespace Asteria

#endif
