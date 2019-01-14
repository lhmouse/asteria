// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_
#define ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "abstract_opaque.hpp"

namespace Asteria {

class Global_context : public Abstract_context
  {
  private:
    rocket::refcounted_ptr<Abstract_opaque> m_impl;

  public:
    Global_context()
      : Abstract_context(),
        m_impl()
      {
        this->do_initialize();
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Global_context);

  private:
    void do_initialize();

  public:
    bool is_analytic() const noexcept override
      {
        return false;
      }
    const Abstract_context * get_parent_opt() const noexcept override
      {
        return nullptr;
      }

    rocket::refcounted_ptr<Variable> create_variable();
  };

}

#endif
