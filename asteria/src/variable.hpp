// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HPP_
#define ASTERIA_VARIABLE_HPP_

#include "fwd.hpp"
#include "value.hpp"

namespace Asteria {

class Variable : public Rcbase<Variable>
  {
  private:
    Value m_value;
    bool m_immutable;

  public:
    Variable()
      : m_value(), m_immutable(false)
      {
      }
    template<typename XvalueT, typename std::enable_if<std::is_constructible<Value, XvalueT &&>::value>::type * = nullptr>
      Variable(XvalueT &&value, bool immutable)
        : m_value(std::forward<XvalueT>(value)), m_immutable(immutable)
        {
        }
    ~Variable();

    Variable(const Variable &)
      = delete;
    Variable & operator=(const Variable &)
      = delete;

  private:
    [[noreturn]] void do_throw_immutable() const;

  public:
    const Value & get_value() const noexcept
      {
        return this->m_value;
      }
    Value & get_value() noexcept
      {
        return this->m_value;
      }
    bool is_immutable() const noexcept
      {
        return this->m_immutable;
      }
    template<typename XvalueT, typename std::enable_if<std::is_constructible<Value, XvalueT &&>::value>::type * = nullptr>
      void set_value(XvalueT &&value)
        {
          if(this->m_immutable) {
            this->do_throw_immutable();
          }
          this->m_value = std::forward<XvalueT>(value);
        }
    template<typename XvalueT, typename std::enable_if<std::is_constructible<Value, XvalueT &&>::value>::type * = nullptr>
      void reset(XvalueT &&value, bool immutable)
        {
          this->m_value = std::forward<XvalueT>(value);
          this->m_immutable = immutable;
        }
  };

}

#endif
