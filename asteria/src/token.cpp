// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "token.hpp"
#include "utilities.hpp"

namespace Asteria {

Token::Token(Token &&) noexcept = default;
Token & Token::operator=(Token &&) noexcept = default;
Token::~Token() = default;

}
