// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GENERATIONAL_COLLECTOR_HPP_
#define ASTERIA_RUNTIME_GENERATIONAL_COLLECTOR_HPP_

#include "../fwd.hpp"
#include "variable_hashset.hpp"
#include "collector.hpp"
#include "../rocket/refcounted_ptr.hpp"

namespace Asteria {

class Generational_collector : public rocket::refcounted_base<Generational_collector>
  {
  private:
    Variable_hashset m_pool;
    Collector m_gen_two;
    Collector m_gen_one;
    Collector m_gen_zero;

  public:
    Generational_collector() noexcept
      : m_pool(),
        m_gen_two(&(this->m_pool), nullptr, 20),
        m_gen_one(&(this->m_pool), &(this->m_gen_two), 100),
        m_gen_zero(&(this->m_pool), &(this->m_gen_one), 500)
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Generational_collector);

  public:
    rocket::refcounted_ptr<Variable> create_variable();
    void perform_garbage_collection(unsigned gen_limit);
  };

}

#endif
