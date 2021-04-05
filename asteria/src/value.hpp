// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_HPP_
#define ASTERIA_VALUE_HPP_

#include "fwd.hpp"
#include "details/value.ipp"

namespace asteria {

class Value
  {
  private:
    ::rocket::variant<
      ROCKET_CDR(
        ,V_null      // 0,
        ,V_boolean   // 1,
        ,V_integer   // 2,
        ,V_real      // 3,
        ,V_string    // 4,
        ,V_opaque    // 5,
        ,V_function  // 6,
        ,V_array     // 7,
        ,V_object    // 8,
      )>
      m_stor;

  public:
    // Constructors and assignment operators
    constexpr
    Value(nullopt_t = nullopt) noexcept
      { }

    template<typename XValT,
    ROCKET_ENABLE_IF(details_value::Valuable<XValT>::direct_init::value)>
    Value(XValT&& xval) noexcept(::std::is_nothrow_constructible<decltype(m_stor),
                  typename details_value::Valuable<XValT>::via_type&&>::value)
      : m_stor(typename details_value::Valuable<XValT>::via_type(
                                        ::std::forward<XValT>(xval)))
      { }

    template<typename XValT,
    ROCKET_DISABLE_IF(details_value::Valuable<XValT>::direct_init::value)>
    Value(XValT&& xval) noexcept(::std::is_nothrow_assignable<decltype(m_stor)&,
                  typename details_value::Valuable<XValT>::via_type&&>::value)
      {
        details_value::Valuable<XValT>::assign(this->m_stor,
                                        ::std::forward<XValT>(xval));
      }

    template<typename XValT,
    ROCKET_ENABLE_IF_HAS_TYPE(typename details_value::Valuable<XValT>::via_type)>
    Value&
    operator=(XValT&& xval) noexcept(::std::is_nothrow_assignable<decltype(m_stor)&,
                  typename details_value::Valuable<XValT>::via_type&&>::value)
      {
        details_value::Valuable<XValT>::assign(this->m_stor,
                                           ::std::forward<XValT>(xval));
        return *this;
      }

  public:
    // Accessors
    Type
    type() const noexcept
      { return static_cast<Type>(this->m_stor.index());  }

    bool
    is_null() const noexcept
      { return this->type() == type_null;  }

    bool
    is_boolean() const noexcept
      { return this->type() == type_boolean;  }

    V_boolean
    as_boolean() const
      { return this->m_stor.as<type_boolean>();  }

    V_boolean&
    open_boolean()
      { return this->m_stor.as<type_boolean>();  }

    bool
    is_integer() const noexcept
      { return this->type() == type_integer;  }

    V_integer
    as_integer() const
      { return this->m_stor.as<type_integer>();  }

    V_integer&
    open_integer()
      { return this->m_stor.as<type_integer>();  }

    bool
    is_real() const noexcept
      { return this->type() == type_real;  }

    V_real
    as_real() const
      { return this->m_stor.as<type_real>();  }

    V_real&
    open_real()
      { return this->m_stor.as<type_real>();  }

    bool
    is_string() const noexcept
      { return this->type() == type_string;  }

    const V_string&
    as_string() const
      { return this->m_stor.as<type_string>();  }

    V_string&
    open_string()
      { return this->m_stor.as<type_string>();  }

    bool
    is_function() const noexcept
      { return this->type() == type_function;  }

    const V_function&
    as_function() const
      { return this->m_stor.as<type_function>();  }

    V_function&
    open_function()
      { return this->m_stor.as<type_function>();  }

    bool
    is_opaque() const noexcept
      { return this->type() == type_opaque;  }

    const V_opaque&
    as_opaque() const
      { return this->m_stor.as<type_opaque>();  }

    V_opaque&
    open_opaque()
      { return this->m_stor.as<type_opaque>();  }

    bool
    is_array() const noexcept
      { return this->type() == type_array;  }

    const V_array&
    as_array() const
      { return this->m_stor.as<type_array>();  }

    V_array&
    open_array()
      { return this->m_stor.as<type_array>();  }

    bool
    is_object() const noexcept
      { return this->type() == type_object;  }

    const V_object&
    as_object() const
      { return this->m_stor.as<type_object>();  }

    V_object&
    open_object()
      { return this->m_stor.as<type_object>();  }

    bool
    is_scalar() const noexcept
      {
        return details_value::imask({ this->type() }) &
               details_value::imask({ type_null, type_boolean, type_integer,
                                      type_real, type_string });
      }

    bool
    is_convertible_to_real() const noexcept
      {
        return details_value::imask({ this->type() }) &
               details_value::imask({ type_integer, type_real });
      }

    V_real
    convert_to_real() const
      {
        return this->is_integer()
            ? V_real(this->as_integer())
            : this->as_real();
      }

    V_real&
    mutate_into_real()
      {
        return this->is_integer()
            ? this->m_stor.emplace<type_real>(V_real(this->as_integer()))
            : this->open_real();
      }

    Value&
    swap(Value& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    // This performs the builtin conversion to boolean values.
    ROCKET_PURE_FUNCTION
    bool
    test() const noexcept;

    // This performs the builtin comparison with another value.
    ROCKET_PURE_FUNCTION
    Compare
    compare(const Value& other) const noexcept;

    // These are miscellaneous interfaces for debugging.
    tinyfmt&
    print(tinyfmt& fmt, bool escape = false) const;

    tinyfmt&
    dump(tinyfmt& fmt, size_t indent = 2, size_t hanging = 0) const;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback) const;
  };

inline
void
swap(Value& lhs, Value& rhs) noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const Value& value)
  { return value.print(fmt);  }

}  // namespace asteria

#endif
