// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "version.hpp"
#include "../value.hpp"
#include "../utils.hpp"
namespace asteria {

void
create_bindings_version(V_object& result, API_Version version)
  {
    result.insert_or_assign(sref("major"),
      V_integer(
        version / 0x10000
      ));

    result.insert_or_assign(sref("minor"),
      V_integer(
        version % 0x10000
      ));
  }

}  // namespace asteria
