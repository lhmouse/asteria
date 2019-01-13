// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_
#define ASTERIA_RUNTIME_EXECUTIVE_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "../rocket/cow_vector.hpp"
#include "../rocket/refcounted_object.hpp"

namespace Asteria {

class Executive_context : public Abstract_context
  {
  private:
    const Executive_context *const m_parent_opt;
    rocket::cow_vector<Reference_dictionary::Template> m_predef_refs;

  public:
    explicit Executive_context(const Executive_context *parent_opt = nullptr) noexcept
      : m_parent_opt(parent_opt)
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Executive_context);

  public:
    bool is_analytic() const noexcept override;
    const Executive_context * get_parent_opt() const noexcept override;

    void initialize_for_function(const rocket::refcounted_object<Variadic_arguer> &zvarg, const rocket::cow_vector<rocket::prehashed_string> &params, Reference &&self, rocket::cow_vector<Reference> &&args);
  };

}

#endif
