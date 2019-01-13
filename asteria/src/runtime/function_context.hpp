// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_FUNCTION_CONTEXT_HPP_
#define ASTERIA_RUNTIME_FUNCTION_CONTEXT_HPP_

#include "../fwd.hpp"
#include "executive_context.hpp"
#include "../rocket/static_vector.hpp"
#include "../rocket/cow_vector.hpp"
#include "../rocket/refcounted_object.hpp"

namespace Asteria {

class Function_context : public Executive_context
  {
  private:
    rocket::static_vector<Reference_dictionary::Template, 7> m_predef_refs;

  public:
    Function_context() noexcept
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Function_context);

  public:
    void initialize_for_function(const rocket::refcounted_object<Variadic_arguer> &zvarg, const rocket::cow_vector<rocket::prehashed_string> &params, Reference &&self, rocket::cow_vector<Reference> &&args);
  };

}

#endif
