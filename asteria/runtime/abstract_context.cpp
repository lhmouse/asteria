// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "abstract_context.hpp"
#include "../utils.hpp"
namespace asteria {

Abstract_Context::
~Abstract_Context()
  {
  }

Reference&
Abstract_Context::
do_mut_named_reference(Reference* hint_opt, phsh_stringR name) const
  {
    return hint_opt ? *hint_opt : this->m_named_refs.insert(name);
  }

void
Abstract_Context::
do_clear_named_references() noexcept
  {
    this->m_named_refs.clear();
  }

const Reference*
Abstract_Context::
get_named_reference_opt(phsh_stringR name) const
  {
    auto qref = this->m_named_refs.find_opt(name);
    if(ROCKET_UNEXPECT(!qref) && (name[0] == '_') && (name[1] == '_')) {
      // Create a lazy reference. Built-in references such as `__func`
      // are only created when they are mentioned.
      qref = this->do_create_lazy_reference_opt(nullptr, name);
    }
    return qref;
  }

Reference&
Abstract_Context::
insert_named_reference(phsh_stringR name)
  {
    bool newly = false;
    auto& ref = this->m_named_refs.insert(name, &newly);
    if(ROCKET_UNEXPECT(newly) && (name[0] == '_') && (name[1] == '_')) {
      // If a built-in reference has been inserted, it may have a default
      // value, so initialize it. DO NOT CALL THIS FUNCTION INSIDE
      // `do_create_lazy_reference_opt()`, as it will result in infinite
      // recursion; call `do_mut_named_reference()` instead.
      this->do_create_lazy_reference_opt(&ref, name);
    }
    return ref;
  }

bool
Abstract_Context::
erase_named_reference(phsh_stringR name, Reference* refp_opt) noexcept
  {
    return this->m_named_refs.erase(name, refp_opt);
  }

}  // namespace asteria
