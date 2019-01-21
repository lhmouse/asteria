// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GENERATIONAL_COLLECTOR_HPP_
#define ASTERIA_RUNTIME_GENERATIONAL_COLLECTOR_HPP_

#include "../fwd.hpp"
#include "refcnt_base.hpp"
#include "variable_hashset.hpp"
#include "collector.hpp"

namespace Asteria {

class Generational_Collector : public virtual RefCnt_Base
  {
  private:
    Variable_Hashset m_pool;
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
    ROCKET_NONCOPYABLE_DESTRUCTOR(Generational_Collector);

  public:
    Collector * get_collector_opt(unsigned gen_limit) noexcept;
    RefCnt_Ptr<Variable> create_variable();
    bool collect(unsigned gen_limit);
  };

}

#endif
