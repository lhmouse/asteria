// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXCEPTION_HPP_
#define ASTERIA_EXCEPTION_HPP_

#include "fwd.hpp"
#include "value.hpp"
#include <exception>

namespace Asteria {

class Exception : public virtual std::exception
  {
  private:
    String m_file;
    Uint32 m_line;
    Value m_value;

    Bivector<String, Uint32> m_backtrace;

  public:
    template<typename XvalueT, typename std::enable_if<std::is_constructible<Value, XvalueT &&>::value>::type * = nullptr>
      Exception(String file, Uint32 line, XvalueT &&value)
        : m_file(std::move(file)), m_line(line), m_value(std::forward<XvalueT>(value))
        {
        }
    explicit Exception(const std::exception &stdex)
      : m_file(String::shallow("<native code>")), m_line(0), m_value(D_string(stdex.what()))
      {
      }
    ~Exception();

  public:
    const String & get_file() const noexcept
      {
        return this->m_file;
      }
    Uint32 get_line() const noexcept
      {
        return this->m_line;
      }
    const Value & get_value() const noexcept
      {
        return this->m_value;
      }

    const Bivector<String, Uint32> & get_backtrace() const noexcept
      {
        return this->m_backtrace;
      }
    Bivector<String, Uint32> & get_backtrace() noexcept
      {
        return this->m_backtrace;
      }
    void append_backtrace(String file, Uint32 line)
      {
        this->m_backtrace.emplace_back(std::move(file), line);
      }

    const char * what() const noexcept override;
  };

}

#endif
