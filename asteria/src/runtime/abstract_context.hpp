// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_

#include "../fwd.hpp"
#include "reference_dictionary.hpp"

namespace Asteria {

class Abstract_context
  {
  private:
    Reference_dictionary m_dict;

  public:
    Abstract_context() noexcept
      : m_dict()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Abstract_context, virtual);

  public:
    const Reference * get_named_reference_opt(const rocket::prehashed_string &name) const
      {
        return this->m_dict.get_opt(name);
      }
    Reference & open_named_reference(const rocket::prehashed_string &name)
      {
        return this->m_dict.open(name);
      }
    void clear(Global_context &global) noexcept
      {
        this->m_dict.clear(global);
      }

    virtual bool is_analytic() const noexcept = 0;
    virtual const Abstract_context * get_parent_opt() const noexcept = 0;
  };

}

#endif
