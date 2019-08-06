// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_TOKEN_STREAM_HPP_
#define ASTERIA_COMPILER_TOKEN_STREAM_HPP_

#include "../fwd.hpp"
#include "parser_error.hpp"
#include "token.hpp"

namespace Asteria {

class Token_Stream
  {
  public:
    enum State : uint8_t
      {
        state_empty    = 0,
        state_error    = 1,
        state_success  = 2,
      };

  private:
    variant<nullptr_t, Parser_Error, cow_vector<Token>> m_stor;  // Tokens are stored in reverse order.

  public:
    Token_Stream() noexcept
      {
      }

  public:
    explicit operator bool () const noexcept
      {
        return this->m_stor.index() == state_success;
      }
    State state() const noexcept
      {
        return static_cast<State>(this->m_stor.index());
      }

    bool load(std::streambuf& sbuf, const cow_string& file, const Compiler_Options& options);
    void clear() noexcept;

    Parser_Error get_parser_error() const noexcept;
    bool empty() const noexcept;
    const Token* peek_opt(size_t ahead = 0) const;
    void shift(size_t count = 1);
  };

}  // namespace Asteria

#endif
