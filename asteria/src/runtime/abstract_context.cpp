// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "abstract_context.hpp"

namespace Asteria {

Abstract_context::~Abstract_context()
  {
  }

void Abstract_context::dispose_named_references(Global_context &global) noexcept
  {
    this->m_dict.for_each(
      [&](const rocket::prehashed_string /*name*/, const Reference &ref)
        {
          ref.dispose_variable(global);
        }
      );
    this->m_dict.clear();
  }

}
