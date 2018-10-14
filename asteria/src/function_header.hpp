// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FUNCTION_HEADER_HPP_
#define ASTERIA_FUNCTION_HEADER_HPP_

#include "fwd.hpp"
#include "source_location.hpp"

namespace Asteria {

class Function_header
  {
  private:
    Source_location m_loc;
    String m_func;
    Vector<String> m_params;

  public:
    Function_header(Source_location loc, String func, Vector<String> params)
      : m_loc(std::move(loc)), m_func(std::move(func)), m_params(std::move(params))
      {
      }
    Function_header(String file, Uint32 line, String func, Vector<String> params)
      : m_loc(std::move(file), line), m_func(std::move(func)), m_params(std::move(params))
      {
      }

  public:
    const Source_location & get_location() const noexcept
      {
        return this->m_loc;
      }
    const String & get_file() const noexcept
      {
        return this->m_loc.get_file();
      }
    Uint32 get_line() const noexcept
      {
        return this->m_loc.get_line();
      }
    const String & get_func() const noexcept
      {
        return this->m_func;
      }
    const Vector<String> & get_params() const noexcept
      {
        return this->m_params;
      }
  };

extern std::ostream & operator<<(std::ostream &os, const Function_header &head);

}

#endif
