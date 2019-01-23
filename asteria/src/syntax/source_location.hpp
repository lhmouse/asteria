// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_SOURCE_LOCATION_HPP_
#define ASTERIA_SYNTAX_SOURCE_LOCATION_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Source_Location
  {
  private:
    CoW_String m_file;
    std::uint32_t m_line;

  public:
    Source_Location(CoW_String file, std::uint32_t line) noexcept
      : m_file(std::move(file)), m_line(line)
      {
      }

  public:
    const CoW_String & get_file() const noexcept
      {
        return this->m_file;
      }
    std::uint32_t get_line() const noexcept
      {
        return this->m_line;
      }

    void print(std::ostream &os) const;
  };

inline std::ostream & operator<<(std::ostream &os, const Source_Location &loc)
  {
    loc.print(os);
    return os;
  }

}

#endif
