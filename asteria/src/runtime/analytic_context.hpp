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
    const Abstract_Context* m_parent_opt;

  public:
    Analytic_Context(int, const Abstract_Context* parent_opt, const cow_vector<phsh_string>& params)  // for functions
      : m_parent_opt(parent_opt)
      {
        this->do_prepare_function(params);
      }
    Analytic_Context(int, const Abstract_Context& parent)  // for non-functions
      : m_parent_opt(std::addressof(parent))
      {
      }
    ~Analytic_Context() override;

  private:
    void do_prepare_function(const cow_vector<phsh_string>& params);

  public:
    bool is_analytic() const noexcept override
      {
        return true;
      }
    const Abstract_Context* get_parent_opt() const noexcept override
      {
        return this->m_parent_opt;
      }
  };

}  // namespace Asteria

#endif
