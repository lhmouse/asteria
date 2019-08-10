// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_BACKTRACE_FRAME_HPP_
#define ASTERIA_RUNTIME_BACKTRACE_FRAME_HPP_

#include "../fwd.hpp"
#include "value.hpp"
#include "../syntax/source_location.hpp"
#include <exception>

namespace Asteria {

class Backtrace_Frame
  {
  private:
    Frame_Type m_type;
    Source_Location m_sloc;
    Value m_value;

  public:
    template<typename XvalueT> Backtrace_Frame(Frame_Type xtype, const Source_Location& xsloc, XvalueT&& xvalue)
      : m_type(xtype), m_sloc(xsloc), m_value(rocket::forward<XvalueT>(xvalue))
      {
      }
    template<typename XvalueT> Backtrace_Frame(Frame_Type xtype, const cow_string& xfile, int64_t xline, XvalueT&& xvalue)
      : m_type(xtype), m_sloc(xfile, xline), m_value(rocket::forward<XvalueT>(xvalue))
      {
      }
    ~Backtrace_Frame();

  public:
    Frame_Type type() const noexcept
      {
        return this->m_type;
      }
    const char* what_type() const noexcept
      {
        return describe_frame_type(this->m_type);
      }
    const Source_Location& sloc() const noexcept
      {
        return this->m_sloc;
      }
    const cow_string& file() const noexcept
      {
        return this->m_sloc.file();
      }
    int64_t line() const noexcept
      {
        return this->m_sloc.line();
      }
    const Value& value() const noexcept
      {
        return this->m_value;
      }
  };

}  // namespace Asteria

#endif
