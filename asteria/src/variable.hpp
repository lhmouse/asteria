// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HPP_
#define ASTERIA_VARIABLE_HPP_

#include "fwd.hpp"
#include "value.hpp"

namespace Asteria
{

class Variable
  {
  private:
    Value m_value;
    bool m_immutable;

  public:
    explicit Variable(Value &&value, bool immutable = false)
      : m_value(std::move(value)), m_immutable(immutable)
      {
      }
    ~Variable();

    Variable(const Variable &) = delete;
    Variable & operator=(const Variable &) = delete;

  private:
    ROCKET_NORETURN void do_throw_immutable() const;

  public:
    const Value & get_value() const noexcept
      {
        return m_value;
      }
    void set_value(Value &&value)
      {
        if(m_immutable)  {
          do_throw_immutable();
        }
        m_value = std::move(value);
      }
    bool is_immutable() const noexcept
      {
        return m_immutable;
      }
    void set_immutable(bool immutable = true) noexcept
      {
        m_immutable = immutable;
      }
  };

}

#endif
