// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_PARSER_HPP_
#define ASTERIA_COMPILER_PARSER_HPP_

#include "../fwd.hpp"
#include "parser_error.hpp"
#include "../syntax/block.hpp"
#include "../rocket/variant.hpp"
#include "../rocket/cow_vector.hpp"

namespace Asteria {

class Parser
  {
  public:
    enum State : std::size_t
      {
        state_empty    = 0,
        state_error    = 1,
        state_success  = 2,
      };

  private:
    rocket::variant<std::nullptr_t, Parser_error, rocket::cow_vector<Statement>> m_stor;

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

    Parser_error get_parser_error() const noexcept;
    bool empty() const noexcept;

    bool load(Token_stream &tstrm_io);
    void clear() noexcept;
    Block extract_document();
  };

}

#endif
