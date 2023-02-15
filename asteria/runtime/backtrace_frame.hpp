// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_BACKTRACE_FRAME_
#define ASTERIA_RUNTIME_BACKTRACE_FRAME_

#include "../fwd.hpp"
#include "../value.hpp"
#include "../source_location.hpp"
#include <exception>
namespace asteria {

class Backtrace_Frame
  {
  private:
    Frame_Type m_type;
    Source_Location m_sloc;
    Value m_value;

  public:
    template<typename XValT>
    Backtrace_Frame(Frame_Type xtype, const Source_Location& xsloc, XValT&& xval)
      : m_type(xtype), m_sloc(xsloc), m_value(::std::forward<XValT>(xval))  { }

  public:
    Frame_Type
    type() const noexcept
      { return this->m_type;  }

    const char*
    what_type() const noexcept
      { return describe_frame_type(this->m_type);  }

    const Source_Location&
    sloc() const noexcept
      { return this->m_sloc;  }

    const cow_string&
    file() const noexcept
      { return this->m_sloc.file();  }

    int
    line() const noexcept
      { return this->m_sloc.line();  }

    int
    column() const noexcept
      { return this->m_sloc.column();  }

    const Value&
    value() const noexcept
      { return this->m_value;  }
  };

}  // namespace asteria
#endif
