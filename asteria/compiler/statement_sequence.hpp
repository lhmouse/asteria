// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_STATEMENT_SEQUENCE_
#define ASTERIA_COMPILER_STATEMENT_SEQUENCE_

#include "../fwd.hpp"
namespace asteria {

class Statement_Sequence
  {
  private:
    Compiler_Options m_opts;
    cow_vector<Statement> m_stmts;

  public:
    explicit constexpr
    Statement_Sequence(const Compiler_Options& opts)
      noexcept
      :
        m_opts(opts)
      { }

  public:
    Statement_Sequence(const Statement_Sequence&) = default;
    Statement_Sequence(Statement_Sequence&&) = default;
    Statement_Sequence& operator=(const Statement_Sequence&) & = default;
    Statement_Sequence& operator=(Statement_Sequence&&) & = default;
    ~Statement_Sequence();

    // accessors
    const Compiler_Options&
    get_options()
      const noexcept
      { return this->m_opts;  }

    Compiler_Options&
    mut_options()
      noexcept
      { return this->m_opts;  }

    void
    set_options(const Compiler_Options& opts)
      noexcept
      { this->m_opts = opts;  }

    bool
    empty()
      const noexcept
      { return this->m_stmts.empty();  }

    const cow_vector<Statement>&
    get_statements()
      const noexcept
      { return this->m_stmts;  }

    cow_vector<Statement>&
    mut_statements()
      noexcept
      { return this->m_stmts;  }

    void
    clear()
      noexcept;

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
