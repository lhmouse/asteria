// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_HPP_
#define ASTERIA_VALUE_HPP_

#include "fwd.hpp"
#include "details/value.ipp"

namespace asteria {

class Value
  {
  private:
    using storage = ::rocket::variant<
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
      )>;

    union {
      char m_bytes[sizeof(storage)];
      storage m_stor;
    };

  public:
    // Constructors and assignment operators
    constexpr
    Value(nullopt_t = nullopt) noexcept
      : m_bytes()
      { }

    template<typename XValT,
    ROCKET_ENABLE_IF(details_value::Valuable<XValT>::direct_init::value)>
    Value(XValT&& xval)
      noexcept(::std::is_nothrow_constructible<decltype(m_stor),
                        typename details_value::Valuable<XValT>::via_type&&>::value)
      : m_stor(typename details_value::Valuable<XValT>::via_type(::std::forward<XValT>(xval)))
      { }

    template<typename XValT,
    ROCKET_DISABLE_IF(details_value::Valuable<XValT>::direct_init::value)>
    Value(XValT&& xval)
      noexcept(::std::is_nothrow_assignable<decltype(m_stor)&,
                        typename details_value::Valuable<XValT>::via_type&&>::value)
      : m_bytes()
      { details_value::Valuable<XValT>::assign(this->m_stor, ::std::forward<XValT>(xval));  }

    template<typename XValT,
    ROCKET_ENABLE_IF_HAS_TYPE(typename details_value::Valuable<XValT>::via_type)>
    Value&
    operator=(XValT&& xval)
      noexcept(::std::is_nothrow_assignable<decltype(m_stor)&,
                        typename details_value::Valuable<XValT>::via_type&&>::value)
      { details_value::Valuable<XValT>::assign(this->m_stor, ::std::forward<XValT>(xval));
        return *this;  }

    Value(const Value& other) noexcept
      : m_stor(other.m_stor)
      { }

    Value&
    operator=(const Value& other) noexcept
      { this->m_stor = other.m_stor;
        return *this;  }

    Value(Value&& other) noexcept
      {
        // Don't play with this at home!
        ::std::memcpy(this->m_bytes, other.m_bytes, sizeof(storage));
        ::std::memset(other.m_bytes, 0, sizeof(storage));
      }

    Value&
    operator=(Value&& other) noexcept
      { return this->swap(other);  }

    Value&
    swap(Value& other) noexcept
      {
        // Don't play with this at home!
        char temp[sizeof(storage)];
        ::std::memcpy(temp, this->m_bytes, sizeof(storage));
        ::std::memcpy(this->m_bytes, other.m_bytes, sizeof(storage));
        ::std::memcpy(other.m_bytes, temp, sizeof(storage));
        return *this;
      }

  private:
    void
    do_get_variables_slow(Variable_HashMap& staged, Variable_HashMap& temp) const;

    ROCKET_PURE bool
    do_test_slow() const noexcept;

    ROCKET_PURE static Compare
    do_compare_slow(const Value& lhs, const Value& rhs) noexcept;

    [[noreturn]] void
    do_throw_type_mismatch(const char* desc) const;

    void
    do_destroy_variant_slow() noexcept;

  public:
    ~Value()
      {
        constexpr bmask32 recursive_types = { type_array, type_object };
        if(recursive_types.test(this->type()))
          this->do_destroy_variant_slow();
        else
          this->m_stor.~storage();
      }

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
      {
        if(this->type() == type_boolean)
          return this->m_stor.as<V_boolean>();

        this->do_throw_type_mismatch("`boolean`");
      }

    V_boolean&
    open_boolean()
      {
        if(this->type() == type_boolean)
          return this->m_stor.as<V_boolean>();

        this->do_throw_type_mismatch("`boolean`");
      }

    bool
    is_integer() const noexcept
      { return this->type() == type_integer;  }

    V_integer
    as_integer() const
      {
        if(this->type() == type_integer)
          return this->m_stor.as<V_integer>();

        this->do_throw_type_mismatch("`integer`");
      }

    V_integer&
    open_integer()
      {
        if(this->type() == type_integer)
          return this->m_stor.as<V_integer>();

        this->do_throw_type_mismatch("`integer`");
      }

    bool
    is_real() const noexcept
      {
        if(this->type() == type_real)
          return true;

        if(this->type() == type_integer)
          return true;

        return false;
      }

    V_real
    as_real() const
      {
        if(this->type() == type_real)
          return this->m_stor.as<V_real>();

        if(this->type() == type_integer)
          return static_cast<V_real>(this->m_stor.as<V_integer>());

        this->do_throw_type_mismatch("`integer` or `real`");
      }

    V_real&
    open_real()
      {
        if(this->type() == type_real)
          return this->m_stor.as<V_real>();

        if(this->type() == type_integer)
          return this->m_stor.emplace<V_real>(
                static_cast<V_real>(this->m_stor.as<V_integer>()));

        this->do_throw_type_mismatch("`integer` or `real`");
      }

    bool
    is_string() const noexcept
      { return this->type() == type_string;  }

    const V_string&
    as_string() const
      {
        if(this->type() == type_string)
          return this->m_stor.as<V_string>();

        this->do_throw_type_mismatch("`string`");
      }

    V_string&
    open_string()
      {
        if(this->type() == type_string)
          return this->m_stor.as<V_string>();

        this->do_throw_type_mismatch("`string`");
      }

    bool
    is_function() const noexcept
      { return this->type() == type_function;  }

    const V_function&
    as_function() const
      {
        if(this->type() == type_function)
          return this->m_stor.as<V_function>();

        this->do_throw_type_mismatch("`function`");
      }

    V_function&
    open_function()
      {
        if(this->type() == type_function)
          return this->m_stor.as<V_function>();

        this->do_throw_type_mismatch("`function`");
      }

    bool
    is_opaque() const noexcept
      { return this->type() == type_opaque;  }

    const V_opaque&
    as_opaque() const
      {
        if(this->type() == type_opaque)
          return this->m_stor.as<V_opaque>();

        this->do_throw_type_mismatch("`opaque`");
      }

    V_opaque&
    open_opaque()
      {
        if(this->type() == type_opaque)
          return this->m_stor.as<V_opaque>();

        this->do_throw_type_mismatch("`opaque`");
      }

    bool
    is_array() const noexcept
      { return this->type() == type_array;  }

    const V_array&
    as_array() const
      {
        if(this->type() == type_array)
          return this->m_stor.as<V_array>();

        this->do_throw_type_mismatch("`array`");
      }

    V_array&
    open_array()
      {
        if(this->type() == type_array)
          return this->m_stor.as<V_array>();

        this->do_throw_type_mismatch("`array`");
      }

    bool
    is_object() const noexcept
      { return this->type() == type_object;  }

    const V_object&
    as_object() const
      {
        if(this->type() == type_object)
          return this->m_stor.as<V_object>();

        this->do_throw_type_mismatch("`object`");
      }

    V_object&
    open_object()
      {
        if(this->type() == type_object)
          return this->m_stor.as<V_object>();

        this->do_throw_type_mismatch("`object`");
      }

    bool
    is_scalar() const noexcept
      {
        constexpr bmask32 scalar_types = { type_null,
            type_boolean, type_integer, type_real, type_string };
        return scalar_types.test(this->type());
      }

    // This is used by garbage collection.
    void
    get_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
      {
        if(!this->is_scalar())
          this->do_get_variables_slow(staged, temp);
      }

    // This performs the builtin conversion to boolean values.
    bool
    test() const noexcept
      {
        if(this->is_null())
          return false;
        else if(this->is_boolean())
          return this->as_boolean();
        else
          return this->do_test_slow();
      }

    // This performs the builtin comparison with another value.
    Compare
    compare(const Value& other) const noexcept
      { return this->do_compare_slow(*this, other);  }

    // These are miscellaneous interfaces for debugging.
    tinyfmt&
    print(tinyfmt& fmt, bool escape = false) const;

    bool
    print_to_stderr(bool escape = false) const;

    tinyfmt&
    dump(tinyfmt& fmt, size_t indent = 2, size_t hanging = 0) const;

    bool
    dump_to_stderr(size_t indent = 2, size_t hanging = 0) const;
  };

inline void
swap(Value& lhs, Value& rhs) noexcept
  { lhs.swap(rhs);  }

inline tinyfmt&
operator<<(tinyfmt& fmt, const Value& value)
  { return value.print(fmt);  }

}  // namespace asteria

#endif
