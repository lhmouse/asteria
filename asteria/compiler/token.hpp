// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_TOKEN_
#define ASTERIA_COMPILER_TOKEN_

#include "../fwd.hpp"
#include "../source_location.hpp"
namespace asteria {

class Token
  {
  public:
    struct S_keyword
      {
        Keyword kwrd;
        const char* str;
      };

    struct S_punctuator
      {
        Punctuator punct;
      };

    struct S_identifier
      {
        cow_string name;
      };

    struct S_integer_literal
      {
        int64_t val;
      };

    struct S_real_literal
      {
        double val;
      };

    struct S_string_literal
      {
        cow_string val;
      };

    enum Index : uint8_t
      {
        index_keyword          = 0,
        index_punctuator       = 1,
        index_identifier       = 2,
        index_integer_literal  = 3,
        index_real_literal     = 4,
        index_string_literal   = 5,
      };

  private:
    Source_Location m_sloc;
    uint32_t m_length;

    ASTERIA_VARIANT(
      m_stor
        , S_keyword          // 0
        , S_punctuator       // 1
        , S_identifier       // 2
        , S_integer_literal  // 3
        , S_real_literal     // 4
        , S_string_literal   // 5
      );

  public:
    template<typename xToken>
    constexpr Token(const Source_Location& xsloc, uint32_t xlen, xToken&& xtok)
      noexcept(::std::is_nothrow_constructible<decltype(m_stor), xToken&&>::value)
      :
        m_sloc(xsloc), m_length(xlen), m_stor(forward<xToken>(xtok))
      { }

    Token&
    swap(Token& other) noexcept
      {
        this->m_sloc.swap(other.m_sloc);
        ::std::swap(this->m_length, other.m_length);
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    Token(const Token&) = default;
    Token(Token&&) = default;
    Token& operator=(const Token&) & = default;
    Token& operator=(Token&&) & = default;
    ~Token();

    // accessors
    const Source_Location&
    sloc() const noexcept
      { return this->m_sloc;  }

    const cow_string&
    file() const noexcept
      { return this->m_sloc.file();  }

    int
    line() const noexcept
      { return this->m_sloc.line();  }

    int
    column() const noexcept
      { return this->m_sloc.column();  }

    uint32_t
    length() const noexcept
      { return this->m_length;  }

    bool
    is_keyword() const noexcept
      { return this->m_stor.index() == index_keyword;  }

    Keyword
    as_keyword() const
      { return this->m_stor.as<S_keyword>().kwrd;  }

    const char*
    as_keyword_c_str() const
      { return this->m_stor.as<S_keyword>().str;  }

    bool
    is_punctuator() const noexcept
      { return this->m_stor.index() == index_punctuator;  }

    Punctuator
    as_punctuator() const
      { return this->m_stor.as<S_punctuator>().punct;  }

    bool
    is_identifier() const noexcept
      { return this->m_stor.index() == index_identifier;  }

    const cow_string&
    as_identifier() const
      { return this->m_stor.as<S_identifier>().name;  }

    bool
    is_integer_literal() const noexcept
      { return this->m_stor.index() == index_integer_literal;  }

    int64_t
    as_integer_literal() const
      { return this->m_stor.as<S_integer_literal>().val;  }

    bool
    is_real_literal() const noexcept
      { return this->m_stor.index() == index_real_literal;  }

    double
    as_real_literal() const
      { return this->m_stor.as<S_real_literal>().val;  }

    bool
    is_string_literal() const noexcept
      { return this->m_stor.index() == index_string_literal;  }

    const cow_string&
    as_string_literal() const
      { return this->m_stor.as<S_string_literal>().val;  }
  };

inline
void
swap(Token& lhs, Token& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria
#endif
