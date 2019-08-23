// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_SOURCE_LOCATION_HPP_
#define ASTERIA_RUNTIME_SOURCE_LOCATION_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Source_Location
  {
  private:
    cow_string m_file;
    int32_t m_line;

  public:
    Source_Location() noexcept
      : m_file(rocket::sref("<empty>")), m_line(-1)
      {
      }
    Source_Location(cow_string xfile, int32_t xline) noexcept
      : m_file(rocket::move(xfile)), m_line(xline)
      {
      }
    template<typename FirstT, typename SecondT> Source_Location(const pair<FirstT, SecondT>& xpair) noexcept
      : m_file(xpair.first), m_line(xpair.second)
      {
      }
    template<typename FirstT, typename SecondT> Source_Location(pair<FirstT, SecondT>&& xpair) noexcept
      : m_file(rocket::move(xpair.first)), m_line(xpair.second)
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
