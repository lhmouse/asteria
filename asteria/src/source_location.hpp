// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SOURCE_LOCATION_HPP_
#define ASTERIA_SOURCE_LOCATION_HPP_

#include "fwd.hpp"

namespace Asteria {

class Source_Location
  {
  private:
    cow_string m_file;
    int m_line;
    int m_offset;

  public:
    Source_Location(nullopt_t = nullopt)
    noexcept
      : m_file(::rocket::sref("<unknown>")), m_line(-1), m_offset(-1)
      { }

    Source_Location(const cow_string& xfile, int xline, int xoffset)
    noexcept
      : m_file(xfile), m_line(xline), m_offset(xoffset)
      { }

  public:
    const cow_string&
    file()
    const noexcept
      { return this->m_file;  }

    const char*
    c_file()
    const noexcept
      { return this->m_file.c_str();  }

    int
    line()
    const noexcept
      { return this->m_line;  }

    int
    offset()
    const noexcept
      { return this->m_offset;  }

    Source_Location&
    swap(Source_Location& other)
    noexcept
      {
        this->m_file.swap(other.m_file);
        ::std::swap(this->m_line, other.m_line);
        ::std::swap(this->m_offset, other.m_offset);
        return *this;
      }

    tinyfmt&
    print(tinyfmt& fmt)
    const;
  };

inline
void
swap(Source_Location& lhs, Source_Location& rhs)
noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const Source_Location& sloc)
  { return sloc.print(fmt);  }

}  // namespace Asteria

#endif
