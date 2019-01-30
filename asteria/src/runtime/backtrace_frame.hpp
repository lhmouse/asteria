// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_BACKTRACE_FRAME_HPP_
#define ASTERIA_RUNTIME_BACKTRACE_FRAME_HPP_

#include "../fwd.hpp"
#include "../syntax/source_location.hpp"

namespace Asteria {

class Backtrace_Frame
  {
  private:
    Source_Location m_sloc;
    CoW_String m_func;

  public:
    Backtrace_Frame(const Source_Location &xsloc, const CoW_String &xfunc) noexcept
      : m_sloc(xsloc), m_func(xfunc)
      {
      }
    Backtrace_Frame(const CoW_String &xfile, std::uint32_t xline, const CoW_String &xfunc) noexcept
      : m_sloc(xfile, xline), m_func(xfunc)
      {
      }

  public:
    const Source_Location & source_location() const noexcept
      {
        return this->m_sloc;
      }
    const CoW_String & source_file() const noexcept
      {
        return this->m_sloc.file();
      }
    std::uint32_t source_line() const noexcept
      {
        return this->m_sloc.line();
      }
    const CoW_String & function_prototype() const noexcept
      {
        return this->m_func;
      }
  };

}

#endif
