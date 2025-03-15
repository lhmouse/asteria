// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SOURCE_LOCATION_
#define ASTERIA_SOURCE_LOCATION_

#include "fwd.hpp"
namespace asteria {

class Source_Location
  {
  private:
    cow_string m_file;
    int m_line;
    int m_column;

  public:
    Source_Location() noexcept
      :
        m_file(&"[unknown]"), m_line(-1), m_column(-1)
      { }

    Source_Location(cow_stringR xfile, int xline, int xcolumn) noexcept
      :
        m_file(xfile), m_line(xline), m_column(xcolumn)
      { }

    Source_Location&
    swap(Source_Location& other) noexcept
      {
        this->m_file.swap(other.m_file);
        ::std::swap(this->m_line, other.m_line);
        ::std::swap(this->m_column, other.m_column);
        return *this;
      }

  public:
    const cow_string&
    file() const noexcept
      { return this->m_file;  }

    const char*
    c_file() const noexcept
      { return this->m_file.c_str();  }

    int
    line() const noexcept
      { return this->m_line;  }

    int
    column() const noexcept
      { return this->m_column;  }

    tinyfmt&
    print_to(tinyfmt& fmt) const;
  };

inline
void
swap(Source_Location& lhs, Source_Location& rhs) noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const Source_Location& sloc)
  { return sloc.print_to(fmt);  }

}  // namespace asteria
#endif
