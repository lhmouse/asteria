// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ABSTRACT_CONTEXT_HPP_
#define ASTERIA_ABSTRACT_CONTEXT_HPP_

#include "fwd.hpp"

namespace Asteria {

class Abstract_context
  {
  public:
    Abstract_context() noexcept
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Abstract_context, virtual);

  public:
    virtual bool is_analytic() const noexcept = 0;
    virtual const Abstract_context * get_parent_opt() const noexcept = 0;

    virtual const Reference * get_named_reference_opt(const rocket::prehashed_string &name) const = 0;
    virtual Reference & mutate_named_reference(const rocket::prehashed_string &name) = 0;
  };

}

#endif
