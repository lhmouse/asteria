// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_SOURCE_LOCATION_HPP_
#define ASTERIA_SYNTAX_SOURCE_LOCATION_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Source_location
  {
  private:
    rocket::cow_string m_file;
    std::uint32_t m_line;

  public:
    Source_location(rocket::cow_string file, std::uint32_t line) noexcept
      : m_file(std::move(file)), m_line(line)
      {
      }

  public:
    const rocket::cow_string & get_file() const noexcept
      {
        return this->m_file;
      }
    std::uint32_t get_line() const noexcept
      {
        return this->m_line;
      }
  };

extern std::ostream & operator<<(std::ostream &os, const Source_location &loc);

}

#endif
