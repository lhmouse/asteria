// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "abstract_context.hpp"
#include "../utils.hpp"

namespace asteria {

Abstract_Context::
~Abstract_Context()
  {
  }

const Reference*
Abstract_Context::
get_named_reference_opt(const phsh_string& name)
  const
  {
    // Search `m_named_refs` for the name.
    auto qref = this->m_named_refs.find_opt(name);
    if(ROCKET_EXPECT(qref))
      return qref;

    // Create lazy references for builtin names.
    qref = this->do_lazy_lookup_opt(name);
    if(ROCKET_UNEXPECT(qref))
      return qref;

    // If no such name exists, fail.
    return nullptr;
  }

Reference&
Abstract_Context::
open_named_reference(const phsh_string& name)
  {
    // Search `m_named_refs` for the name.
    auto qref = this->m_named_refs.mut_find_opt(name);
    if(ROCKET_EXPECT(qref))
      return *qref;

    // Create lazy references for builtin names.
    qref = this->do_lazy_lookup_opt(name);
    if(ROCKET_UNEXPECT(qref))
      return *qref;

    // If no such name exists, create a new reference.
    return this->m_named_refs.open(name);
  }

}  // namespace asteria
