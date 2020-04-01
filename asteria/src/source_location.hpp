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
    long m_line;

  public:
    Source_Location() noexcept
      :
        m_file(sref("<empty>")), m_line(-1)
      {
      }
    Source_Location(const cow_string& xfile, long xline) noexcept
      :
        m_file(xfile), m_line(xline)
      {
      }

  public:
    const cow_string& file() const noexcept
      {
        return this->m_file;
      }
    const char* c_file() const noexcept
      {
        return this->m_file.c_str();
      }
    long line() const noexcept
      {
        return this->m_line;
      }

    tinyfmt& print(tinyfmt& fmt) const;
  };

inline tinyfmt& operator<<(tinyfmt& fmt, const Source_Location& sloc)
  {
    return sloc.print(fmt);
  }

}  // namespace Asteria

#endif
