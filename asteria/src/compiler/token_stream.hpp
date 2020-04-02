// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_TOKEN_STREAM_HPP_
#define ASTERIA_COMPILER_TOKEN_STREAM_HPP_

#include "../fwd.hpp"
#include "../recursion_sentry.hpp"
#include "token.hpp"

namespace Asteria {

class Token_Stream
  {
  private:
    Recursion_Sentry m_sentry;
    cow_vector<Token> m_rtoks;  // Tokens are stored in reverse order.

  public:
    constexpr Token_Stream() noexcept
      = default;

  public:
    // These are accessors and modifiers of tokens in this stream.
    bool empty() const noexcept
      { return this->m_rtoks.empty();  }

    Token_Stream& clear() noexcept
      { return this->m_rtoks.clear(), *this;  }

    size_t size() const noexcept
      { return this->m_rtoks.size();  }

    const Token* peek_opt(size_t offset = 0) const noexcept
      { return this->m_rtoks.get_ptr(this->m_rtoks.size() + ~offset);  }

    Token_Stream& shift(size_t count = 1)
      { return this->m_rtoks.pop_back(count), *this;  }

    // This provides stack overflow protection.
    Recursion_Sentry copy_recursion_sentry() const
      { return this->m_sentry;  }

    const void* get_recursion_base() const noexcept
      { return this->m_sentry.get_base();  }

    Token_Stream& set_recursion_base(const void* base) noexcept
      { return this->m_sentry.set_base(base), *this;  }

    // This function parses characters from the input stream and fills tokens into `*this`.
    // The contents of `*this` are destroyed prior to any further operation.
    // This function throws a `Parser_Error` upon failure.
    Token_Stream& reload(tinybuf& cbuf, const cow_string& file, const Compiler_Options& opts);
  };

}  // namespace Asteria

#endif
