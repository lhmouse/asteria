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
    Value m_value;

  public:
    template<typename XvalueT, typename std::enable_if<std::is_constructible<Value, XvalueT &&>::value>::type * = nullptr>
      explicit Exception(XvalueT &&value)
        : m_value(std::forward<XvalueT>(value))
        {
        }
    ~Exception();

  public:
    const Value & get_value() const noexcept
      {
        return this->m_value;
      }
    Value & get_value() noexcept
      {
        return this->m_value;
      }

    const char * what() const noexcept override;
  };

}

#endif
