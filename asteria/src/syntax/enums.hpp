// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_ENUMS_HPP_
#define ASTERIA_SYNTAX_ENUMS_HPP_

#include "../fwd.hpp"

namespace Asteria {

// Target of jump statements
enum Jump_Target : uint8_t
  {
    jump_target_unspec  = 0,
    jump_target_switch  = 1,
    jump_target_while   = 2,
    jump_target_for     = 3,
  };

}  // namespace Asteria

#endif
