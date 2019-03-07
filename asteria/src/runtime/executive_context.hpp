// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_
#define ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"

namespace Asteria {

class Executive_Context : public Abstract_Context
  {
  private:
    const Executive_Context *const m_parent_opt;

  public:
    // An executive context can only be created on another executive context (not an analytic one).
    explicit Executive_Context(const Executive_Context *parent_opt) noexcept
      : m_parent_opt(parent_opt)
      {
      }
    ~Executive_Context() override;

  public:
    bool is_analytic() const noexcept override
      {
        return false;
      }
    const Executive_Context * get_parent_opt() const noexcept override
      {
        return this->m_parent_opt;
      }
  };

}  // namespace Asteria

#endif
