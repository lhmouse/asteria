// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_PARSER_OPTIONS_HPP_
#define ASTERIA_COMPILER_PARSER_OPTIONS_HPP_

#include "../fwd.hpp"

namespace Asteria {

enum class Parser_Options : std::uint32_t
  {
    none                  =  0x00000000,
    integer_as_real       =  0x00000001,  // Tokenize `integer` literals as `real` literals.
  };

constexpr Parser_Options operator&(Parser_Options lhs, Parser_Options rhs) noexcept
  {
    return Parser_Options(std::uint32_t(lhs) & std::uint32_t(rhs));
  }
constexpr Parser_Options operator|(Parser_Options lhs, Parser_Options rhs) noexcept
  {
    return Parser_Options(std::uint32_t(lhs) | std::uint32_t(rhs));
  }
constexpr Parser_Options operator^(Parser_Options lhs, Parser_Options rhs) noexcept
  {
    return Parser_Options(std::uint32_t(lhs) ^ std::uint32_t(rhs));
  }

inline Parser_Options & operator&=(Parser_Options &lhs, Parser_Options rhs) noexcept
  {
    return lhs = lhs & rhs;
  }
inline Parser_Options & operator|=(Parser_Options &lhs, Parser_Options rhs) noexcept
  {
    return lhs = lhs | rhs;
  }
inline Parser_Options & operator^=(Parser_Options &lhs, Parser_Options rhs) noexcept
  {
    return lhs = lhs ^ rhs;
  }

}

#endif
