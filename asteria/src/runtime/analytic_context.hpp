// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ANALYTIC_CONTEXT_HPP_
#define ASTERIA_RUNTIME_ANALYTIC_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "../rocket/cow_vector.hpp"

namespace Asteria {

class Analytic_context : public Abstract_context
  {
  private:
    const Abstract_context *const m_parent_opt;

  public:
    explicit Analytic_context(const Abstract_context *parent_opt) noexcept
      : Abstract_context(),
        m_parent_opt(parent_opt)
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Analytic_context);

  public:
    bool is_analytic() const noexcept override
      {
        return true;
      }
    const Abstract_context * get_parent_opt() const noexcept override
      {
        return this->m_parent_opt;
      }
  };

}

#endif
