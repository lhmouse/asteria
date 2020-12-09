// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_PARSER_ERROR_HPP_
#define ASTERIA_COMPILER_PARSER_ERROR_HPP_

#include "../fwd.hpp"
#include "../source_location.hpp"
#include <exception>

namespace asteria {

class Parser_Error
  : public virtual exception
  {
  private:
    Parser_Status m_stat;
    Source_Location m_sloc;
    size_t m_length;

    cow_string m_what;  // a comprehensive string that is human-readable.

  public:
    explicit
    Parser_Error(Parser_Status xstat, const Source_Location& xsloc, size_t xlength)
      : m_stat(xstat), m_sloc(xsloc), m_length(xlength)
      { this->do_compose_message();  }

    ASTERIA_INCOMPLET(Token_Stream)
    explicit
    Parser_Error(Parser_Status xstat, const Token_Stream& xtstrm)
      : m_stat(xstat), m_sloc(xtstrm.next_sloc()), m_length(xtstrm.next_length())
      { this->do_compose_message();  }

  private:
    void
    do_compose_message();

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(Parser_Error);

    const char*
    what()
      const noexcept override
      { return this->m_what.c_str();  }

    Parser_Status
    status()
      const noexcept
      { return this->m_stat;  }

    const char*
    what_status()
      const noexcept
      { return describe_parser_status(this->m_stat);  }

    const Source_Location&
    sloc()
      const noexcept
      { return this->m_sloc;  }

    const cow_string&
    file()
      const noexcept
      { return this->m_sloc.file();  }

    int
    line()
      const noexcept
      { return this->m_sloc.line();  }

    int
    offset()
      const noexcept
      { return this->m_sloc.offset();  }

    size_t
    length()
      const noexcept
      { return this->m_length;  }
  };

inline
bool
operator==(const Parser_Error& lhs, Parser_Status rhs)
  noexcept
  { return lhs.status() == rhs;  }

inline
bool
operator!=(const Parser_Error& lhs, Parser_Status rhs)
  noexcept
  { return lhs.status() != rhs;  }

inline
bool
operator==(Parser_Status lhs, const Parser_Error& rhs)
  noexcept
  { return lhs == rhs.status();  }

inline
bool
operator!=(Parser_Status lhs, const Parser_Error& rhs)
  noexcept
  { return lhs != rhs.status();  }

}  // namespace asteria

#endif
