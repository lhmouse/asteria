// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_TOKEN_STREAM_HPP_
#define ASTERIA_COMPILER_TOKEN_STREAM_HPP_

#include "../fwd.hpp"
#include "token.hpp"

namespace Asteria {

class Token_Stream
  {
  private:
    cow_vector<Token> m_tokens;  // Tokens are stored in reverse order.

  public:
    Token_Stream() noexcept
      {
      }
    Token_Stream(tinybuf& cbuf, const cow_string& file, const Compiler_Options& opts)
      {
        this->reload(cbuf, file, opts);
      }

  public:
    bool empty() const noexcept
      {
        return this->m_tokens.empty();
      }
    Token_Stream& clear() noexcept
      {
        return this->m_tokens.clear(), *this;
      }

    // These are accessors and modifiers of tokens in this stream.
    size_t size() const noexcept
      {
        return this->m_tokens.size();
      }
    const Token* peek_opt(size_t offset = 0) const noexcept
      {
        if(ROCKET_UNEXPECT(offset >= this->m_tokens.size())) {
          return nullptr;
        }
        return std::addressof(this->m_tokens.rbegin()[ptrdiff_t(offset)]);
      }
    Token_Stream& shift(size_t count = 1)
      {
        this->m_tokens.pop_back(count);
        return *this;
      }

    // This function parses characters from the input streambuf and fills tokens into `*this`.
    // The contents of `*this` are destroyed prior to any further operation.
    // This function throws a `Parser_Error` upon failure.
    Token_Stream& reload(tinybuf& cbuf, const cow_string& file, const Compiler_Options& opts);
  };

}  // namespace Asteria

#endif
