// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_PARSER_HPP_
#define ASTERIA_COMPILER_PARSER_HPP_

#include "../fwd.hpp"
#include "parser_error.hpp"
#include "../syntax/block.hpp"
#include "../rocket/variant.hpp"

namespace Asteria {

class Parser
  {
  public:
    enum State : std::uint8_t
      {
        state_empty    = 0,
        state_error    = 1,
        state_success  = 2,
      };

  private:
    rocket::variant<std::nullptr_t, Parser_Error, Cow_Vector<Statement>> m_stor;

  public:
    Parser() noexcept
      : m_stor()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Parser);

  public:
    State state() const noexcept
      {
        return State(this->m_stor.index());
      }
    explicit operator bool () const noexcept
      {
        return this->state() == state_success;
      }

    Parser_Error get_parser_error() const noexcept;
    bool empty() const noexcept;

    bool load(Token_Stream &tstrm_io);
    void clear() noexcept;
    Block extract_document();
  };

}

#endif
