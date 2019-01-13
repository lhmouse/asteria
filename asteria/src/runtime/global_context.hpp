// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_
#define ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "generational_collector.hpp"

namespace Asteria {

class Global_context : public Abstract_context
  {
  private:
    Generational_collector m_gen_coll;

  public:
    Global_context()
      : Abstract_context()
      {
        this->do_add_std_bindings();
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Global_context);

  private:
    void do_add_std_bindings();

  public:
    bool is_analytic() const noexcept override
      {
        return false;
      }
    const Abstract_context * get_parent_opt() const noexcept override
      {
        return nullptr;
      }

    Generational_collector & get_collector() noexcept
      {
        return this->m_gen_coll;
      }
  };

}

#endif
