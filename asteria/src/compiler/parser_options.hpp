// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_PARSER_OPTIONS_HPP_
#define ASTERIA_COMPILER_PARSER_OPTIONS_HPP_

#include "../fwd.hpp"

namespace Asteria {

struct Parser_Options
  {
    // Make single quotes behave similiar to double quotes. [useful when parsing JSON5 text]
    bool escapable_single_quote_string = false;
    // Parse keywords as identifiers. [useful when parsing JSON text]
    bool keyword_as_identifier = false;
    // Parse integer literals as real literals. [useful when parsing JSON text]
    bool integer_as_real = false;
    // Disable Tail Call Optimization (TCO).
    bool disable_tco = false;
  };

}  // namespace Asteria

#endif
