// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "version.hpp"
#include "../value.hpp"
#include "../utilities.hpp"

namespace Asteria {

void create_bindings_version(Oval& result, API_Version version)
  {
    //===================================================================
    // `std.version.major`
    //===================================================================
    result.insert_or_assign(::rocket::sref("major"),
      Ival(
        // The major version number of the standard library that has been enabled.
        version / 0x10000
      ));
    //===================================================================
    // `std.version.minor`
    //===================================================================
    result.insert_or_assign(::rocket::sref("minor"),
      Ival(
        // The minor version number of the standard library that has been enabled.
        version % 0x10000
      ));
    //===================================================================
    // End of `std.version`
    //===================================================================
  }

}  // namespace Asteria
