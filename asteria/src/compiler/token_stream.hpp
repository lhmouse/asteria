// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_TOKEN_STREAM_HPP_
#define ASTERIA_COMPILER_TOKEN_STREAM_HPP_

#include "../fwd.hpp"
#include "parser_options.hpp"
#include "parser_error.hpp"
#include "token.hpp"
#include "../rocket/variant.hpp"

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
    rocket::variant<std::nullptr_t, Parser_Error, CoW_Vector<Token>> m_stor;  // Tokens are stored in reverse order.

  public:
    Token_Stream() noexcept
      : m_stor()
      {
      }

    Token_Stream(const Token_Stream &)
      = delete;
    Token_Stream & operator=(const Token_Stream &)
      = delete;

  public:
    explicit operator bool () const noexcept
      {
        return this->m_stor.get<CoW_Vector<Token>>() != nullptr;
      }
    State state() const noexcept
      {
        return static_cast<State>(this->m_stor.index());
      }

    Parser_Error get_parser_error() const noexcept;
    bool empty() const noexcept;

    bool load(std::istream &cstrm_io, const CoW_String &file, const Parser_Options &options);
    void clear() noexcept;
    const Token * peek_opt() const noexcept;
    Token shift();
  };

}

#endif
