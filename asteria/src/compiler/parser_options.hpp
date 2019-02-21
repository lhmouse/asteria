// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_PARSER_OPTIONS_HPP_
#define ASTERIA_COMPILER_PARSER_OPTIONS_HPP_

#include "../fwd.hpp"

namespace Asteria {

struct Parser_Options
  {
    bool integer_as_real : 1;
    bool keyword_as_identifier : 1;
    bool escapable_single_quote_string : 1;

    constexpr Parser_Options() noexcept
      : integer_as_real(false), keyword_as_identifier(false), escapable_single_quote_string(false)
      {
      }
  };

}

#endif
