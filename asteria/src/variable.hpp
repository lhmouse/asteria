// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HPP_
#define ASTERIA_VARIABLE_HPP_

#include "fwd.hpp"
#include "value.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

class Variable : public rocket::refcounted_base<Variable>
  {
  private:
    Value m_value;
    bool m_immutable;
    long m_gcref;  // This is uninitialized by default.

  public:
    Variable()
      : m_value(), m_immutable(false)
      {
      }
    template<typename XvalueT,
      typename std::enable_if<std::is_constructible<Value, XvalueT &&>::value>::type * = nullptr>
        Variable(XvalueT &&value, bool immutable)
      : m_value(std::forward<XvalueT>(value)), m_immutable(immutable)
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Variable);

  private:
    [[noreturn]] void do_throw_immutable() const;

  public:
    const Value & get_value() const noexcept
      {
        return this->m_value;
      }
    Value & get_mutable_value() noexcept
      {
        if(this->m_immutable) {
          this->do_throw_immutable();
        }
        return this->m_value;
      }
    bool is_immutable() const noexcept
      {
        return this->m_immutable;
      }
    template<typename XvalueT,
      typename std::enable_if<std::is_constructible<Value, XvalueT &&>::value>::type * = nullptr>
        void set_value(XvalueT &&value)
      {
        if(this->m_immutable) {
          this->do_throw_immutable();
        }
        this->m_value = std::forward<XvalueT>(value);
      }
    template<typename XvalueT,
      typename std::enable_if<std::is_constructible<Value, XvalueT &&>::value>::type * = nullptr>
        void reset(XvalueT &&value, bool immutable)
      {
        this->m_value = std::forward<XvalueT>(value);
        this->m_immutable = immutable;
      }

    long get_gcref() const noexcept
      {
        return this->m_gcref;
      }
    void set_gcref(long gcref) noexcept
      {
        this->m_gcref = gcref;
      }

    void enumerate_variables(const Abstract_variable_callback &callback) const;
  };

}

#endif
