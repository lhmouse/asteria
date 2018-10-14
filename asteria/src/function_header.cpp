// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "function_header.hpp"

namespace Asteria {

std::ostream & operator<<(std::ostream &os, const Function_header &head)
  {
    os <<head.get_func() <<'(';
    bool needs_comma = true;
    for(const auto &param : head.get_params()) {
      if(rocket::exchange(needs_comma, true)) {
        os <<',' <<' ';
      }
      os <<param;
    }
    os <<')';
    return os;
  }

}
