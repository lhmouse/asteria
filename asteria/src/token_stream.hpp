// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TOKEN_STREAM_HPP_
#define ASTERIA_TOKEN_STREAM_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"
#include "parser_error.hpp"
#include "token.hpp"

namespace Asteria {

class Token_stream
  {
  public:
    enum State : Uint8
      {
        state_empty    = 0,
        state_error    = 1,
        state_success  = 2,
      };

  private:
    rocket::variant<std::nullptr_t, Parser_error, Vector<Token>> m_stor;  // Tokens are stored in reverse order.

  public:
    Token_stream() noexcept
      : m_stor()
      {
      }
    ~Token_stream();

   Token_stream(const Token_stream &)
     = delete;
   Token_stream & operator=(const Token_stream &)
     = delete;

  public:
    State state() const noexcept
      {
        return State(this->m_stor.index());
      }
    explicit operator bool () const noexcept
      {
        return this->state() == state_success;
      }

    bool load(std::istream &cstrm_io, const String &file);
    void clear() noexcept;

    Parser_error get_parser_error() const;
    bool empty() const noexcept;
    const Token * peek_opt() const noexcept;
    Token shift();
  };

}

#endif
