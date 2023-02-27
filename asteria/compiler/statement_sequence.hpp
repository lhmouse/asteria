// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_STATEMENT_SEQUENCE_
#define ASTERIA_COMPILER_STATEMENT_SEQUENCE_

#include "../fwd.hpp"
#include "statement.hpp"
namespace asteria {

class Statement_Sequence
  {
  private:
    Compiler_Options m_opts;
    cow_vector<Statement> m_stmts;

  public:
    explicit constexpr
    Statement_Sequence(const Compiler_Options& opts) noexcept
      : m_opts(opts)  { }

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(Statement_Sequence);

    // These are accessors and modifiers of options for parsing.
    const Compiler_Options&
    get_options() const noexcept
      {
        return this->m_opts;
      }

    Compiler_Options&
    mut_options() noexcept
      {
        return this->m_opts;
      }

    void
    set_options(const Compiler_Options& opts) noexcept
      {
        this->m_opts = opts;
      }

    // These are accessors to the statements in this sequence.
    // Note that the sequence cannot be modified.
    bool
    empty() const noexcept
      { return this->m_stmts.empty();  }

    operator
    const cow_vector<Statement>&() const noexcept
      { return this->m_stmts;  }

    void
    clear() noexcept
      {
        this->m_stmts.clear();
      }

    // This function parses tokens from the input stream and fills
    // statements into `*this`. The contents of `*this` are destroyed.
    // This function throws a `Compiler_Error` upon failure.
    void
    reload(Token_Stream&& tstrm);

    // This function parses a single expression (without trailing
    // semicolons) instead of a series of statements, as a `return`
    // statement.
    // This function throws a `Compiler_Error` upon failure.
    void
    reload_oneline(Token_Stream&& tstrm);
  };

}  // namespace asteria
#endif
