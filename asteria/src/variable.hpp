// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HPP_
#define ASTERIA_VARIABLE_HPP_

#include "fwd.hpp"
#include "value.hpp"

namespace Asteria {

class Variable : public Sbase<Variable>
  {
  private:
    Value m_value;
    bool m_immutable;

  public:
    explicit Variable(Value value, bool immutable = false) noexcept
      : m_value(std::move(value)), m_immutable(immutable)
      {
      }
    ~Variable();

    Variable(const Variable &)
      = delete;
    Variable & operator=(const Variable &)
      = delete;

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

    Value & set_value(Value value, bool immutable = false);
    bool & set_immutable(bool immutable = true) noexcept;
  };

}

#endif
