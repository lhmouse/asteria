// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TOKEN_STREAM_HPP_
#define ASTERIA_TOKEN_STREAM_HPP_

#include "fwd.hpp"

namespace Asteria {

class Token_stream
  {
  private:
    Vector<Token> m_rseq;  // Tokens are stored in reverse order.

  public:
    Token_stream() noexcept;
    ~Token_stream();

    Token_stream(Token_stream &&) noexcept;
    Token_stream & operator=(Token_stream &&) noexcept;

  public:
    Parser_result load(std::istream &sis_inout, const String &file);
    void clear() noexcept;

    bool empty() const noexcept;
    const Token * peek_opt() const noexcept;
    Token shift();
  };

}

#endif
