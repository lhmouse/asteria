// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_
#define ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "refcnt_base.hpp"

namespace Asteria {

class Global_Context : public Abstract_Context
  {
  private:
    RefCnt_Ptr<RefCnt_Base> m_collector;

  public:
    Global_Context()
      : Abstract_Context()
      {
        this->do_initialize();
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Global_Context);

  private:
    void do_initialize();

  public:
    bool is_analytic() const noexcept override
      {
        return false;
      }
    const Abstract_Context * get_parent_opt() const noexcept override
      {
        return nullptr;
      }

    RefCnt_Ptr<Variable> create_variable();
  };

}

#endif
