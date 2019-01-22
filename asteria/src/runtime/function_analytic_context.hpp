// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_FUNCTION_ANALYTIC_CONTEXT_HPP_
#define ASTERIA_RUNTIME_FUNCTION_ANALYTIC_CONTEXT_HPP_

#include "../fwd.hpp"
#include "analytic_context.hpp"

namespace Asteria {

class Function_Analytic_Context : public Analytic_Context
  {
  private:
    // N.B. If you have ever changed the capacity, remember to update 'function_executive_context.hpp' as well.
    Static_Vector<Reference_Dictionary::Template, 7> m_predef_refs;

  public:
    explicit Function_Analytic_Context(const Abstract_Context *parent_opt) noexcept
      : Analytic_Context(parent_opt),
        m_predef_refs()
      {
      }
    ~Function_Analytic_Context() override;

  public:
    void initialize(const Cow_Vector<PreHashed_String> &params);
  };

}

#endif
