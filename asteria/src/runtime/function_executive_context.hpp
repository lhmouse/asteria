// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_FUNCTION_EXECUTIVE_CONTEXT_HPP_
#define ASTERIA_RUNTIME_FUNCTION_EXECUTIVE_CONTEXT_HPP_

#include "../fwd.hpp"
#include "executive_context.hpp"

namespace Asteria {

class Function_Executive_Context : public Executive_Context
  {
  private:
    // N.B. If you have ever changed the capacity, remember to update 'function_analytic_context.hpp' as well.
    Static_Vector<std::pair<Cow_String, Reference>, 5> m_predef_refs;

  public:
    // A function executive context does not have a parent context. All undefined names in the to-be-executed function must have already been resolved.
    Function_Executive_Context(const Rcobj<Variadic_Arguer> &zvarg, const Cow_Vector<PreHashed_String> &params, Reference &&self, Cow_Vector<Reference> &&args)
      : Executive_Context(nullptr)
      {
        this->do_set_arguments(zvarg, params, rocket::move(self), rocket::move(args));
      }
    ~Function_Executive_Context() override;

  private:
    void do_set_arguments(const Rcobj<Variadic_Arguer> &zvarg, const Cow_Vector<PreHashed_String> &params, Reference &&self, Cow_Vector<Reference> &&args);
  };

}  // namespace Asteria

#endif
