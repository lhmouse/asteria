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
    Static_Vector<std::pair<CoW_String, Reference>, 7> m_predef_refs;

  public:
    // A function analytic context can be created on another analytic or executive context.
    Function_Analytic_Context(const Abstract_Context *parent_opt,
                              const CoW_Vector<PreHashed_String> &params)
      : Analytic_Context(parent_opt)
      {
        this->do_set_parameters(params);
      }
    ~Function_Analytic_Context() override;

  private:
    void do_set_parameters(const CoW_Vector<PreHashed_String> &params);
  };

}

#endif
