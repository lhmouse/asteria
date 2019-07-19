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
    Analytic_Context(int, const Cow_Vector<PreHashed_String>& params)  // for top-level functions
      : m_parent_opt(nullptr)
      {
        this->do_prepare_function(params);
      }
    Analytic_Context(int, const Abstract_Context& parent, const Cow_Vector<PreHashed_String>& params)  // for closure functions
      : m_parent_opt(&parent)
      {
        this->do_prepare_function(params);
      }
    Analytic_Context(int, const Abstract_Context& parent)  // for non-functions
      : m_parent_opt(std::addressof(parent))
      {
      }
    ~Analytic_Context() override;

  private:
    void do_prepare_function(const Cow_Vector<PreHashed_String>& params);

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
