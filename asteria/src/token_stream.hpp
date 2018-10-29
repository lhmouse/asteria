// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TOKEN_STREAM_HPP_
#define ASTERIA_TOKEN_STREAM_HPP_

#include "fwd.hpp"
#include "parser_error.hpp"
#include "token.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Token_stream
  {
  public:
    enum State : std::uint8_t
      {
        state_empty    = 0,
        state_error    = 1,
        state_success  = 2,
      };

  private:
    rocket::variant<std::nullptr_t, Parser_error, rocket::cow_vector<Token>> m_stor;  // Tokens are stored in reverse order.

  public:
    Token_stream() noexcept
      : m_stor()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Token_stream);

  public:
    State state() const noexcept
      {
        return State(this->m_stor.index());
      }
    explicit operator bool () const noexcept
      {
        return this->state() == state_success;
      }

    Parser_error get_parser_error() const noexcept;
    bool empty() const noexcept;

    bool load(std::istream &cstrm_io, const rocket::cow_string &file);
    void clear() noexcept;
    const Token * peek_opt() const noexcept;
    Token shift();
  };

}

#endif
