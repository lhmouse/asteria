// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "version.hpp"
#include "../value.hpp"
#include "../utils.hpp"
namespace asteria {

void
create_bindings_version(V_object& result, API_Version version)
  {
    result.insert_or_assign(&"major",
      V_integer(
        version / 0x10000
      ));

    result.insert_or_assign(&"minor",
      V_integer(
        version % 0x10000
      ));
  }

}  // namespace asteria
