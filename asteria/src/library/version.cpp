// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "version.hpp"
#include "../value.hpp"
#include "../util.hpp"

namespace asteria {

void
create_bindings_version(V_object& result, API_Version version)
  {
    result.insert_or_assign(::rocket::sref("major"),
      V_integer(
        version / 0x10000
      ));

    result.insert_or_assign(::rocket::sref("minor"),
      V_integer(
        version % 0x10000
      ));
  }

}  // namespace asteria
