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

// Infix operator precedences
enum Precedence : uint8_t
  {
    precedence_multiplicative  =  1,
    precedence_additive        =  2,
    precedence_shift           =  3,
    precedence_relational      =  4,
    precedence_equality        =  5,
    precedence_bitwise_and     =  6,
    precedence_bitwise_xor     =  7,
    precedence_bitwise_or      =  8,
    precedence_logical_and     =  9,
    precedence_logical_or      = 10,
    precedence_coalescence     = 11,
    precedence_assignment      = 12,
    precedence_lowest          = 99,
  };

}  // namespace Asteria

#endif
