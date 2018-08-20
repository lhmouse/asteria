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
    explicit Variable(Value value, bool immutable = false)
      : m_value(std::move(value)), m_immutable(immutable)
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
        return m_value;
      }
    Value & get_value() noexcept
      {
        return m_value;
      }
    bool is_immutable() const noexcept
      {
        return m_immutable;
      }
    Value & set_value(Value value, bool immutable = false)
      {
        if(m_immutable)  {
          do_throw_immutable();
        }
        auto &res = m_value = std::move(value);
        m_immutable = immutable;
        return res;
      }
    bool & set_immutable(bool immutable = true) noexcept
      {
        return m_immutable = immutable;
      }
  };

}

#endif
