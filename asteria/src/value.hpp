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
    Value(XValT&& xval)
      noexcept(::std::is_nothrow_constructible<decltype(m_stor),
                  typename details_value::Valuable<XValT>::via_type&&>::value)
      : m_stor(typename details_value::Valuable<XValT>::via_type(
                                        ::std::forward<XValT>(xval)))
      { }

    template<typename XValT,
    ROCKET_DISABLE_IF(details_value::Valuable<XValT>::direct_init::value)>
    Value(XValT&& xval)
      noexcept(::std::is_nothrow_assignable<decltype(m_stor)&,
                  typename details_value::Valuable<XValT>::via_type&&>::value)
      {
        details_value::Valuable<XValT>::assign(this->m_stor,
                                        ::std::forward<XValT>(xval));
      }

    template<typename XValT,
    ROCKET_ENABLE_IF_HAS_TYPE(typename details_value::Valuable<XValT>::via_type)>
    Value&
    operator=(XValT&& xval)
      noexcept(::std::is_nothrow_assignable<decltype(m_stor)&,
                  typename details_value::Valuable<XValT>::via_type&&>::value)
      {
        details_value::Valuable<XValT>::assign(this->m_stor,
                                           ::std::forward<XValT>(xval));
        return *this;
      }

  private:
    Variable_Callback&
    do_enumerate_variables_slow(Variable_Callback& callback) const;

    ROCKET_PURE bool
    do_test_slow() const noexcept;

    ROCKET_PURE static Compare
    do_compare_slow(const Value& lhs, const Value& rhs) noexcept;

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
      { return this->m_stor.as<V_boolean>();  }

    V_boolean&
    open_boolean()
      { return this->m_stor.as<V_boolean>();  }

    bool
    is_integer() const noexcept
      { return this->type() == type_integer;  }

    V_integer
    as_integer() const
      { return this->m_stor.as<V_integer>();  }

    V_integer&
    open_integer()
      { return this->m_stor.as<V_integer>();  }

    bool
    is_real() const noexcept
      {
        return (this->type() == type_integer) ||
               (this->type() == type_real);
      }

    V_real
    as_real() const
      {
        return this->is_integer()
             ? V_real(this->m_stor.as<V_integer>())
             : this->m_stor.as<V_real>();
      }

    V_real&
    open_real()
      {
        return this->is_integer()
             ? this->m_stor.emplace<V_real>(
                   static_cast<V_real>(this->m_stor.as<V_integer>()))
             : this->m_stor.as<V_real>();
      }

    bool
    is_string() const noexcept
      { return this->type() == type_string;  }

    const V_string&
    as_string() const
      { return this->m_stor.as<V_string>();  }

    V_string&
    open_string()
      { return this->m_stor.as<V_string>();  }

    bool
    is_function() const noexcept
      { return this->type() == type_function;  }

    const V_function&
    as_function() const
      { return this->m_stor.as<V_function>();  }

    V_function&
    open_function()
      { return this->m_stor.as<V_function>();  }

    bool
    is_opaque() const noexcept
      { return this->type() == type_opaque;  }

    const V_opaque&
    as_opaque() const
      { return this->m_stor.as<V_opaque>();  }

    V_opaque&
    open_opaque()
      { return this->m_stor.as<V_opaque>();  }

    bool
    is_array() const noexcept
      { return this->type() == type_array;  }

    const V_array&
    as_array() const
      { return this->m_stor.as<V_array>();  }

    V_array&
    open_array()
      { return this->m_stor.as<V_array>();  }

    bool
    is_object() const noexcept
      { return this->type() == type_object;  }

    const V_object&
    as_object() const
      { return this->m_stor.as<V_object>();  }

    V_object&
    open_object()
      { return this->m_stor.as<V_object>();  }

    bool
    is_scalar() const noexcept
      {
        return (1 << this->type()) &
               (1 << type_null | 1 << type_boolean | 1 << type_integer |
                1 << type_real | 1 << type_string);
      }

    Value&
    swap(Value& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    // This is used by garbage collection.
    Variable_Callback&
    enumerate_variables(Variable_Callback& callback) const
      {
        return this->is_scalar()
            ? callback
            : this->do_enumerate_variables_slow(callback);
      }

    // This performs the builtin conversion to boolean values.
    bool
    test() const noexcept
      {
        return this->is_null() ? false
            : this->is_boolean() ? this->as_boolean()
            : this->do_test_slow();
      }

    // This performs the builtin comparison with another value.
    Compare
    compare(const Value& other) const noexcept
      { return this->do_compare_slow(*this, other);  }

    // These are miscellaneous interfaces for debugging.
    tinyfmt&
    print(tinyfmt& fmt, bool escape = false) const;

    tinyfmt&
    dump(tinyfmt& fmt, size_t indent = 2, size_t hanging = 0) const;
  };

inline void
swap(Value& lhs, Value& rhs) noexcept
  { lhs.swap(rhs);  }

inline tinyfmt&
operator<<(tinyfmt& fmt, const Value& value)
  { return value.print(fmt);  }

}  // namespace asteria

#endif
