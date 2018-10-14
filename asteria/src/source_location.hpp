// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SOURCE_LOCATION_HPP_
#define ASTERIA_SOURCE_LOCATION_HPP_

#include "fwd.hpp"

namespace Asteria {

class Source_location
  {
  private:
    String m_file;
    Uint32 m_line;

  public:
    Source_location(String file, Uint32 line)
      : m_file(std::move(file)), m_line(line)
      {
      }

  public:
    const String & get_file() const noexcept
      {
        return this->m_file;
      }
    Uint32 get_line() const noexcept
      {
        return this->m_line;
      }
  };

extern std::ostream & operator<<(std::ostream &os, const Source_location &loc);

}

#endif
