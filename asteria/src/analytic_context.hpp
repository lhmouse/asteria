// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ANALYTIC_CONTEXT_HPP_
#define ASTERIA_ANALYTIC_CONTEXT_HPP_

#include "fwd.hpp"
#include "abstract_context.hpp"

namespace Asteria {

class Analytic_context : public Abstract_context
  {
  private:
    const Abstract_context *m_parent_opt;

  public:
    explicit Analytic_context(const Abstract_context *parent_opt)
      : m_parent_opt(parent_opt)
      {
      }
    ~Analytic_context();

  public:
    bool is_analytic() const noexcept override;
    const Abstract_context * get_parent_opt() const noexcept override;
  };

extern void initialize_analytic_function_context(Analytic_context &ctx_out, const Vector<String> &params);

}

#endif
