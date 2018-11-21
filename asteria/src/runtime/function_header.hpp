// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_FUNCTION_HEADER_HPP_
#define ASTERIA_RUNTIME_FUNCTION_HEADER_HPP_

#include "../fwd.hpp"
#include "source_location.hpp"

namespace Asteria {

class Function_header
  {
  private:
    Source_location m_loc;
    rocket::prehashed_string m_func;
    rocket::cow_vector<rocket::prehashed_string> m_params;

  public:
    template<typename ...XparamsT>
      Function_header(Source_location loc, rocket::prehashed_string func, XparamsT &&...xparams)
      : m_loc(std::move(loc)), m_func(std::move(func)), m_params(std::forward<XparamsT>(xparams)...)
      {
      }
    template<typename ...XparamsT>
      Function_header(rocket::cow_string file, std::uint32_t line, rocket::prehashed_string func, XparamsT &&...xparams)
      : m_loc(std::move(file), line), m_func(std::move(func)), m_params(std::forward<XparamsT>(xparams)...)
      {
      }

  public:
    const Source_location & get_location() const noexcept
      {
        return this->m_loc;
      }
    const rocket::cow_string & get_file() const noexcept
      {
        return this->m_loc.get_file();
      }
    std::uint32_t get_line() const noexcept
      {
        return this->m_loc.get_line();
      }
    const rocket::prehashed_string & get_func() const noexcept
      {
        return this->m_func;
      }
    std::size_t get_param_count() const noexcept
      {
        return this->m_params.size();
      }
    const rocket::prehashed_string & get_param_name(std::size_t index) const
      {
        return this->m_params.at(index);
      }
  };

extern std::ostream & operator<<(std::ostream &os, const Function_header &head);

}

#endif
