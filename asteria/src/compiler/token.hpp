// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_TOKEN_HPP_
#define ASTERIA_COMPILER_TOKEN_HPP_

#include "../fwd.hpp"
#include "parser_error.hpp"

namespace Asteria {

class Token
  {
  public:
    struct S_keyword
      {
        Keyword kwrd;
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
    using Xvariant = variant<
      ROCKET_CDR(
        , S_keyword          // 0,
        , S_punctuator       // 1,
        , S_identifier       // 2,
        , S_integer_literal  // 3,
        , S_real_literal     // 4,
        , S_string_literal   // 5,
      )>;
    static_assert(std::is_nothrow_copy_assignable<Xvariant>::value, "???");

  private:
    cow_string m_file;
    long m_line;
    size_t m_offset;
    size_t m_length;
    Xvariant m_stor;

  public:
    template<typename XtokT, ASTERIA_SFINAE_CONSTRUCT(Xvariant, XtokT&&)> Token(const cow_string& xfile, int32_t xline,
                                                                                size_t xoffset, size_t xlength, XtokT&& xtok)
      :
        m_file(xfile), m_line(xline), m_offset(xoffset), m_length(xlength),
        m_stor(rocket::forward<XtokT>(xtok))
      {
      }

  public:
    const cow_string& file() const noexcept
      {
        return this->m_file;
      }
    long line() const noexcept
      {
        return this->m_line;
      }
    size_t offset() const noexcept
      {
        return this->m_offset;
      }
    size_t length() const noexcept
      {
        return this->m_length;
      }

    Index index() const noexcept
      {
        return static_cast<Index>(this->m_stor.index());
      }

    bool is_keyword() const noexcept
      {
        return this->index() == index_keyword;
      }
    Keyword as_keyword() const
      {
        return this->m_stor.as<index_keyword>().kwrd;
      }

    bool is_punctuator() const noexcept
      {
        return this->index() == index_punctuator;
      }
    Punctuator as_punctuator() const
      {
        return this->m_stor.as<index_punctuator>().punct;
      }

    bool is_identifier() const noexcept
      {
        return this->index() == index_identifier;
      }
    const cow_string& as_identifier() const
      {
        return this->m_stor.as<index_identifier>().name;
      }

    bool is_integer_literal() const noexcept
      {
        return this->index() == index_integer_literal;
      }
    int64_t as_integer_literal() const
      {
        return this->m_stor.as<index_integer_literal>().val;
      }

    bool is_real_literal() const noexcept
      {
        return this->index() == index_real_literal;
      }
    double as_real_literal() const
      {
        return this->m_stor.as<index_real_literal>().val;
      }

    bool is_string_literal() const noexcept
      {
        return this->index() == index_string_literal;
      }
    const cow_string& as_string_literal() const
      {
        return this->m_stor.as<index_string_literal>().val;
      }

    Token& swap(Token& other) noexcept
      {
        this->m_file.swap(other.m_file);
        std::swap(this->m_line, other.m_line);
        std::swap(this->m_offset, other.m_offset);
        std::swap(this->m_length, other.m_length);
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    tinyfmt& print(tinyfmt& fmt) const;
  };

inline void swap(Token& lhs, Token& rhs) noexcept
  {
    lhs.swap(rhs);
  }

inline tinyfmt& operator<<(tinyfmt& fmt, const Token& token)
  {
    return token.print(fmt);
  }

}  // namespace Asteria

#endif
