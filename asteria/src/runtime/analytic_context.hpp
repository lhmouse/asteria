// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ANALYTIC_CONTEXT_HPP_
#define ASTERIA_RUNTIME_ANALYTIC_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"

namespace Asteria {

class Analytic_Context : public Abstract_Context
  {
  private:
    const Abstract_Context *const m_parent_opt;

  public:
    explicit Analytic_Context(const Abstract_Context *parent_opt) noexcept
      : Abstract_Context(),
        m_parent_opt(parent_opt)
      {
      }
    ~Analytic_Context() override;

  public:
    bool is_analytic() const noexcept override
      {
        return true;
      }
    const Abstract_Context * get_parent_opt() const noexcept override
      {
        return this->m_parent_opt;
      }
  };

}

#endif
