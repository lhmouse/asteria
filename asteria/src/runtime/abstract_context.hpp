// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_

#include "../fwd.hpp"
#include "reference_dictionary.hpp"

namespace Asteria {

class Abstract_Context
  {
  private:
    Reference_Dictionary m_dict;

  public:
    Abstract_Context() noexcept
      : m_dict()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Abstract_Context, virtual);

  protected:
    void do_set_named_reference_templates(const Reference_Dictionary::Template *tdata_opt, std::size_t tsize) noexcept
      {
        this->m_dict.set_templates(tdata_opt, tsize);
      }

  public:
    const Reference * get_named_reference_opt(const rocket::prehashed_string &name) const
      {
        return this->m_dict.get_opt(name);
      }
    Reference & open_named_reference(const rocket::prehashed_string &name)
      {
        return this->m_dict.open(name);
      }
    void clear_named_references() noexcept
      {
        this->m_dict.clear();
      }

    virtual bool is_analytic() const noexcept = 0;
    virtual const Abstract_Context * get_parent_opt() const noexcept = 0;
  };

}

#endif
