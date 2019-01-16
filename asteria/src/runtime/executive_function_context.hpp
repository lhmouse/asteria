// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EXECUTIVE_FUNCTION_CONTEXT_HPP_
#define ASTERIA_RUNTIME_EXECUTIVE_FUNCTION_CONTEXT_HPP_

#include "../fwd.hpp"
#include "executive_context.hpp"
#include "../rocket/static_vector.hpp"
#include "../rocket/cow_vector.hpp"
#include "../rocket/refcounted_object.hpp"

namespace Asteria {

class Executive_Function_Context : public Executive_Context
  {
  private:
    // N.B. If you have ever changed the capacity, remember to update 'analytic_function_context.hpp' as well.
    rocket::static_vector<Reference_Dictionary::Template, 7> m_predef_refs;

  public:
    explicit Executive_Function_Context(std::nullptr_t) noexcept  // The argument is reserved.
      : Executive_Context(nullptr),
        m_predef_refs()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Executive_Function_Context);

  public:
    void initialize(const rocket::refcounted_object<Variadic_Arguer> &zvarg, const rocket::cow_vector<rocket::prehashed_string> &params, Reference &&self, rocket::cow_vector<Reference> &&args);
  };

}

#endif
