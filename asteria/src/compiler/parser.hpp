// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_PARSER_HPP_
#define ASTERIA_COMPILER_PARSER_HPP_

#include "../fwd.hpp"
#include "parser_options.hpp"
#include "parser_error.hpp"
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

    Parser(const Parser &)
      = delete;
    Parser & operator=(const Parser &)
      = delete;

  public:
    explicit operator bool () const noexcept
      {
        return this->m_stor.get<Cow_Vector<Statement>>() != nullptr;
      }
    State state() const noexcept
      {
        return static_cast<State>(this->m_stor.index());
      }

    Parser_Error get_parser_error() const noexcept;
    bool empty() const noexcept;

    bool load(Token_Stream &tstrm_io, const Parser_Options &options);
    void clear() noexcept;
    Cow_Vector<Statement> get_statements() const noexcept;
  };

}

#endif
