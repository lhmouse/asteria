// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_STATEMENT_SEQUENCE_HPP_
#define ASTERIA_COMPILER_STATEMENT_SEQUENCE_HPP_

#include "../fwd.hpp"
#include "statement.hpp"

namespace Asteria {

class Statement_Sequence
  {
  private:
    cow_vector<Statement> m_stmts;

  public:
    Statement_Sequence() noexcept
      { }
    Statement_Sequence(Token_Stream& tstrm, const Compiler_Options& opts)
      {
        this->reload(tstrm, opts);
      }

  public:
    bool empty() const noexcept
      {
        return this->m_stmts.size();
      }
    Statement_Sequence& clear() noexcept
      {
        return this->m_stmts.clear(), *this;
      }

    // These are accessors to the statements in this sequence.
    // Note that the sequence cannot be modified.
    size_t size() const noexcept
      {
        return this->m_stmts.size();
      }
    const Statement& at(size_t index) const
      {
        return this->m_stmts.at(index);
      }
    const cow_vector<Statement>& get_statements() const noexcept
      {
        return this->m_stmts;
      }

    // This function parses tokens from the input stream and fills statements into `*this`.
    // The contents of `*this` are destroyed prior to any further operation.
    // This function throws a `Parser_Error` upon failure.
    Statement_Sequence& reload(Token_Stream& tstrm, const Compiler_Options& opts);
  };

}  // namespace Asteria

#endif
