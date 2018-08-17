// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "context.hpp"
#include "reference.hpp"

namespace Asteria
{

Context::~Context() = default;

Sptr<Variable> Context::get_variable_opt(const String &name)
  {
    const auto ref = m_named_refs.get(name);
    if(!ref) {
      return nullptr;
    }
    return materialize_reference(*ref);
  }

}
