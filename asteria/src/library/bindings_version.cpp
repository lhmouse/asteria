// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_version.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"

namespace Asteria {

void create_bindings_version(D_object& result, API_Version version)
  {
    //===================================================================
    // `std.version.major`
    //===================================================================
    result.insert_or_assign(rocket::sref("major"),
      D_integer(
        version / 0x10000
      ));
    //===================================================================
    // `std.version.minor`
    //===================================================================
    result.insert_or_assign(rocket::sref("minor"),
      D_integer(
        version % 0x10000
      ));
    //===================================================================
    // End of `std.version`
    //===================================================================
  }

}  // namespace Asteria
