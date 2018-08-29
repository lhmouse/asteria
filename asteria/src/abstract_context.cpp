// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "abstract_context.hpp"

namespace Asteria {

Abstract_context::~Abstract_context()
  {
  }

bool is_name_reserved(const String &name)
  {
    return name.empty() || name.starts_with("__");
  }

}
