// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_BACKTRACE_FRAME_HPP_
#define ASTERIA_RUNTIME_BACKTRACE_FRAME_HPP_

#include "../fwd.hpp"
#include "value.hpp"
#include "../syntax/source_location.hpp"

namespace Asteria {

class Backtrace_Frame
  {
  public:
    enum Ftype : std::uint8_t
      {
        ftype_native  = 0,
        ftype_throw   = 1,
        ftype_catch   = 2,
        ftype_func    = 3,
      };

  public:
    ROCKET_PURE_FUNCTION static const char* stringify_ftype(Ftype xftype) noexcept;

  private:
    Source_Location m_sloc;
    Ftype m_ftype;
    Value m_value;

  public:
    template<typename XvalueT> Backtrace_Frame(const Source_Location& xsloc, Ftype xftype, XvalueT&& xvalue)
      : m_sloc(xsloc), m_ftype(xftype), m_value(rocket::forward<XvalueT>(xvalue))
      {
      }
    template<typename XvalueT> Backtrace_Frame(const Cow_String& xfile, std::int64_t xline, Ftype xftype, XvalueT&& xvalue)
      : m_sloc(xfile, xline), m_ftype(xftype), m_value(rocket::forward<XvalueT>(xvalue))
      {
      }
    ~Backtrace_Frame();

  public:
    const Source_Location& location() const noexcept
      {
        return this->m_sloc;
      }
    const Cow_String& file() const noexcept
      {
        return this->m_sloc.file();
      }
    std::int64_t line() const noexcept
      {
        return this->m_sloc.line();
      }
    Ftype ftype() const noexcept
      {
        return this->m_ftype;
      }
    const Value& value() const noexcept
      {
        return this->m_value;
      }
  };

}  // namespace Asteria

#endif
