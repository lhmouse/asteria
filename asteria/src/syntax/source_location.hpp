// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_SOURCE_LOCATION_HPP_
#define ASTERIA_SYNTAX_SOURCE_LOCATION_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Source_Location
  {
  private:
    cow_string m_file;
    int32_t m_line;

  public:
    Source_Location(cow_string xfile, int32_t xline) noexcept
      : m_file(rocket::move(xfile)), m_line(xline)
      {
      }

  public:
    const cow_string& file() const noexcept
      {
        return this->m_file;
      }
    int32_t line() const noexcept
      {
        return this->m_line;
      }

    std::ostream& print(std::ostream& ostrm) const;
  };

inline std::ostream& operator<<(std::ostream& ostrm, const Source_Location& sloc)
  {
    return sloc.print(ostrm);
  }

}  // namespace Asteria

#endif
