// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_FUNCTION_ANALYTIC_CONTEXT_HPP_
#define ASTERIA_RUNTIME_FUNCTION_ANALYTIC_CONTEXT_HPP_

#include "../fwd.hpp"
#include "analytic_context.hpp"
#include "../rocket/static_vector.hpp"
#include "../rocket/cow_vector.hpp"
#include "../rocket/refcounted_object.hpp"

namespace Asteria {

class Function_analytic_context : public Analytic_context
  {
  private:
    // N.B. If you have ever changed the capacity, remember to update 'function_executive_context.hpp' as well.
    rocket::static_vector<Reference_dictionary::Template, 7> m_predef_refs;

  public:
    explicit Function_analytic_context(const Abstract_context *parent_opt) noexcept
      : Analytic_context(parent_opt),
        m_predef_refs()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Function_analytic_context);

  public:
    void initialize(const rocket::cow_vector<rocket::prehashed_string> &params);
  };

}

#endif
