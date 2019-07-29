// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_PARSER_HPP_
#define ASTERIA_COMPILER_PARSER_HPP_

#include "../fwd.hpp"
#include "parser_error.hpp"

namespace Asteria {

class Parser
  {
  public:
    enum State : uint8_t
      {
        state_empty    = 0,
        state_error    = 1,
        state_success  = 2,
      };

  private:
    variant<nullptr_t, Parser_Error, cow_vector<Statement>> m_stor;

  public:
    Parser() noexcept
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

    bool load(Token_Stream& tstrm, const Compiler_Options& options);
    void clear() noexcept;

    Parser_Error get_parser_error() const noexcept;
    const cow_vector<Statement>& get_statements() const;
  };

}  // namespace Asteria

#endif
