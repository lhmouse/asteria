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
    Static_Vector<Reference_Dictionary::Template, 7> m_predef_refs;

  public:
    explicit Function_Executive_Context(std::nullptr_t) noexcept  // The argument is reserved.
      : Executive_Context(nullptr),
        m_predef_refs()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Function_Executive_Context);

  public:
    void initialize(const RefCnt_Object<Variadic_Arguer> &zvarg, const Cow_Vector<PreHashed_String> &params, Reference &&self, Cow_Vector<Reference> &&args);
  };

}

#endif
