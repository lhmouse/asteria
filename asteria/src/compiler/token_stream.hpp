// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_TOKEN_STREAM_HPP_
#define ASTERIA_COMPILER_TOKEN_STREAM_HPP_

#include "../fwd.hpp"
#include "parser_options.hpp"
#include "parser_error.hpp"
#include "token.hpp"

namespace Asteria {

class Token_Stream
  {
  public:
    enum State : std::uint8_t
      {
        state_empty    = 0,
        state_error    = 1,
        state_success  = 2,
      };

  private:
    Variant<std::nullptr_t, Parser_Error, Cow_Vector<Token>> m_stor;  // Tokens are stored in reverse order.

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

    bool load(std::streambuf& cbuf, const Cow_String& file, const Parser_Options& options);
    void clear() noexcept;

    Parser_Error get_parser_error() const noexcept;
    bool empty() const noexcept;
    const Token* peek_opt() const;
    void shift();
  };

}  // namespace Asteria

#endif
