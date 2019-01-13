// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_
#define ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"

namespace Asteria {

class Executive_context : public Abstract_context
  {
  private:
    const Executive_context *const m_parent_opt;

  public:
    explicit Executive_context(const Executive_context *parent_opt) noexcept
      : Abstract_context(),
        m_parent_opt(parent_opt)
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Executive_context);

  public:
    bool is_analytic() const noexcept override
      {
        return false;
      }
    const Executive_context * get_parent_opt() const noexcept override
      {
        return this->m_parent_opt;
      }
  };

}

#endif
