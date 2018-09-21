// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PARSER_HPP_
#define ASTERIA_PARSER_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"
#include "parser_result.hpp"
#include "block.hpp"

namespace Asteria {

class Parser
  {
  public:
    enum State : Uint8
      {
        state_empty    = 0,
        state_error    = 1,
        state_success  = 2,
      };

  private:
    rocket::variant<std::nullptr_t, Parser_result, Block> m_stor;

  public:
    Parser() noexcept
      : m_stor()
      {
      }
    ~Parser();

    Parser(Parser &&) noexcept;
    Parser & operator=(Parser &&) noexcept;

  public:
    State state() const noexcept
      {
        return static_cast<State>(this->m_stor.index());
      }
    explicit operator bool () const noexcept
      {
        return this->state() == state_success;
      }

    Parser_result load(Token_stream &toks_io);
    void clear() noexcept;

    Parser_result get_result() const;
    const Block & get_document() const;
    Block extract_document();
  };

}

#endif
