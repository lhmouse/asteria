// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_
#define ASTERIA_VALUE_

#include "fwd.hpp"
#include "details/value.ipp"
namespace asteria {

class Value
  {
  private:
    using variant_type =
            ::rocket::variant<
                  V_null, V_boolean, V_integer, V_real, V_string,
                  V_opaque, V_function, V_array, V_object>;

    using bytes_type =
            ::std::aligned_storage<sizeof(variant_type), 16U>::type;

    union {
      variant_type m_stor;
      bytes_type m_bytes;
    };

  public:
    // Constructors and assignment operators
    constexpr Value(nullopt_t = nullopt) noexcept
      :
        m_bytes()
      { }

    template<typename xValue,
    ROCKET_ENABLE_IF(details_value::Valuable<xValue>::direct_init::value)>
    Value(xValue&& xval)
      noexcept(::std::is_nothrow_constructible<decltype(m_stor),
                   typename details_value::Valuable<xValue>::via_type&&>::value)
      :
        m_stor(typename details_value::Valuable<xValue>::via_type(forward<xValue>(xval)))
      { }

    template<typename xValue,
    ROCKET_DISABLE_IF(details_value::Valuable<xValue>::direct_init::value)>
    Value(xValue&& xval)
      noexcept(::std::is_nothrow_assignable<decltype(m_stor)&,
                   typename details_value::Valuable<xValue>::via_type&&>::value)
      :
        m_bytes()
      {
        details_value::Valuable<xValue>::assign(this->m_stor, forward<xValue>(xval));
      }

    template<typename xValue,
    ROCKET_ENABLE_IF_HAS_TYPE(typename details_value::Valuable<xValue>::via_type)>
    Value&
    operator=(xValue&& xval) &
      noexcept(::std::is_nothrow_assignable<decltype(m_stor)&,
                   typename details_value::Valuable<xValue>::via_type&&>::value)
      {
        details_value::Valuable<xValue>::assign(this->m_stor, forward<xValue>(xval));
        return *this;
      }

    Value(const Value& other) noexcept
      :
        m_stor(other.m_stor)
      { }

    Value&
    operator=(const Value& other) & noexcept
      {
        this->m_stor = other.m_stor;
        return *this;
      }

    Value(Value&& other) noexcept
      :
        m_bytes(::rocket::exchange(other.m_bytes))  // HACK
      { }

    Value&
    operator=(Value&& other) & noexcept
      {
        ::std::swap(this->m_bytes, other.m_bytes);  // HACK
        return *this;
      }

    Value&
    swap(Value& other) noexcept
      {
        ::std::swap(this->m_bytes, other.m_bytes);  // HACK
        return *this;
      }

  private:
    void
    do_destroy_variant_slow() noexcept;

    void
    do_collect_variables_slow(Variable_HashMap& staged, Variable_HashMap& temp) const;

    [[noreturn]]
    void
    do_throw_type_mismatch(const char* expecting) const;

    [[noreturn]]
    void
    do_throw_uncomparable_with(const Value& other) const;

  public:
    ~Value()
      {
        if(ROCKET_UNEXPECT(this->type() >= type_string))
          this->do_destroy_variant_slow();
      }

    // Accessors
    Type
    type() const noexcept
      { return static_cast<Type>(this->m_stor.index());  }

    bool
    is_null() const noexcept
      { return this->m_stor.index() == type_null;  }

    bool
    is_boolean() const noexcept
      { return this->m_stor.index() == type_boolean;  }

    V_boolean
    as_boolean() const
      {
        if(this->m_stor.index() == type_boolean)
          return this->m_stor.as<V_boolean>();

        this->do_throw_type_mismatch("`boolean`");
      }

    V_boolean&
    mut_boolean()
      {
        if(this->m_stor.index() == type_boolean)
          return this->m_stor.mut<V_boolean>();

        this->do_throw_type_mismatch("`boolean`");
      }

    bool
    is_integer() const noexcept
      { return this->m_stor.index() == type_integer;  }

    V_integer
    as_integer() const
      {
        if(this->m_stor.index() == type_integer)
          return this->m_stor.as<V_integer>();

        this->do_throw_type_mismatch("`integer`");
      }

    V_integer&
    mut_integer()
      {
        if(this->m_stor.index() == type_integer)
          return this->m_stor.mut<V_integer>();

        this->do_throw_type_mismatch("`integer`");
      }

    bool
    is_real() const noexcept
      { return (this->type() >= type_integer) && (this->type() <= type_real);  }

    V_real
    as_real() const
      {
        if(this->m_stor.index() == type_real)
          return this->m_stor.as<V_real>();

        if(this->m_stor.index() == type_integer)
          return static_cast<V_real>(this->m_stor.as<V_integer>());

        this->do_throw_type_mismatch("`integer` or `real`");
      }

    V_real&
    mut_real()
      {
        if(this->m_stor.index() == type_real)
          return this->m_stor.mut<V_real>();

        if(this->m_stor.index() == type_integer)
          return this->m_stor.emplace<V_real>(
                   static_cast<V_real>(this->m_stor.mut<V_integer>()));

        this->do_throw_type_mismatch("`integer` or `real`");
      }

    bool
    is_string() const noexcept
      { return this->m_stor.index() == type_string;  }

    const V_string&
    as_string() const
      {
        if(this->m_stor.index() == type_string)
          return this->m_stor.as<V_string>();

        this->do_throw_type_mismatch("`string`");
      }

    V_string&
    mut_string()
      {
        if(this->m_stor.index() == type_string)
          return this->m_stor.mut<V_string>();

        this->do_throw_type_mismatch("`string`");
      }

    bool
    is_function() const noexcept
      { return this->m_stor.index() == type_function;  }

    const V_function&
    as_function() const
      {
        if(this->m_stor.index() == type_function)
          return this->m_stor.as<V_function>();

        this->do_throw_type_mismatch("`function`");
      }

    V_function&
    mut_function()
      {
        if(this->m_stor.index() == type_function)
          return this->m_stor.mut<V_function>();

        this->do_throw_type_mismatch("`function`");
      }

    bool
    is_opaque() const noexcept
      { return this->m_stor.index() == type_opaque;  }

    const V_opaque&
    as_opaque() const
      {
        if(this->m_stor.index() == type_opaque)
          return this->m_stor.as<V_opaque>();

        this->do_throw_type_mismatch("`opaque`");
      }

    V_opaque&
    mut_opaque()
      {
        if(this->m_stor.index() == type_opaque)
          return this->m_stor.mut<V_opaque>();

        this->do_throw_type_mismatch("`opaque`");
      }

    bool
    is_array() const noexcept
      { return this->m_stor.index() == type_array;  }

    const V_array&
    as_array() const
      {
        if(this->m_stor.index() == type_array)
          return this->m_stor.as<V_array>();

        this->do_throw_type_mismatch("`array`");
      }

    V_array&
    mut_array()
      {
        if(this->m_stor.index() == type_array)
          return this->m_stor.mut<V_array>();

        this->do_throw_type_mismatch("`array`");
      }

    bool
    is_object() const noexcept
      { return this->m_stor.index() == type_object;  }

    const V_object&
    as_object() const
      {
        if(this->m_stor.index() == type_object)
          return this->m_stor.as<V_object>();

        this->do_throw_type_mismatch("`object`");
      }

    V_object&
    mut_object()
      {
        if(this->m_stor.index() == type_object)
          return this->m_stor.mut<V_object>();

        this->do_throw_type_mismatch("`object`");
      }

    // This is used by garbage collection.
    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
      {
        if(this->type() >= type_opaque)
          this->do_collect_variables_slow(staged, temp);
      }

    // This performs the builtin conversion to boolean values.
    bool
    test() const noexcept
      {
        switch(this->m_stor.index()) {
          case type_null:
            return false;

          case type_boolean:
            return this->m_stor.as<V_boolean>();

          case type_integer:
            return this->m_stor.as<V_integer>() != 0;

          case type_real:
            return this->m_stor.as<V_real>() != 0;

          case type_string:
            return this->m_stor.as<V_string>().size() != 0;

          case type_array:
            return this->m_stor.as<V_array>().size() != 0;

          default:  // opaque, function, object
            return true;
        }
      }

    // These perform the builtin comparison.
    Compare
    compare_numeric_partial(V_integer other) const noexcept;

    Compare
    compare_numeric_total(V_integer other) const
      {
        auto cmp = this->compare_numeric_partial(other);
        if(cmp == compare_unordered)
          this->do_throw_uncomparable_with(other);
        return cmp;
      }

    Compare
    compare_numeric_partial(V_real other) const noexcept;

    Compare
    compare_numeric_total(V_real other) const
      {
        auto cmp = this->compare_numeric_partial(other);
        if(cmp == compare_unordered)
          this->do_throw_uncomparable_with(other);
        return cmp;
      }

    Compare
    compare_partial(const Value& other) const;

    Compare
    compare_total(const Value& other) const
      {
        auto cmp = this->compare_partial(other);
        if(cmp == compare_unordered)
          this->do_throw_uncomparable_with(other);
        return cmp;
      }

    // These are miscellaneous interfaces for debugging.
    tinyfmt&
    print(tinyfmt& fmt) const;

    bool
    print_to_stderr() const;

    tinyfmt&
    dump(tinyfmt& fmt, size_t indent = 2, size_t hanging = 0) const;

    bool
    dump_to_stderr(size_t indent = 2, size_t hanging = 0) const;
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

extern template class ::rocket::variant<::asteria::V_null, ::asteria::V_boolean,
    ::asteria::V_integer, ::asteria::V_real, ::asteria::V_string, ::asteria::V_opaque,
    ::asteria::V_function, ::asteria::V_array,  ::asteria::V_object>;
extern template class ::rocket::cow_vector<::asteria::Value>;
extern template class ::rocket::cow_hashmap<::rocket::prehashed_string,
    ::asteria::Value, ::rocket::prehashed_string::hash>;
extern template class ::rocket::optional<::asteria::V_boolean>;
extern template class ::rocket::optional<::asteria::V_integer>;
extern template class ::rocket::optional<::asteria::V_real>;
extern template class ::rocket::optional<::asteria::V_string>;
extern template class ::rocket::optional<::asteria::V_opaque>;
extern template class ::rocket::optional<::asteria::V_function>;
extern template class ::rocket::optional<::asteria::V_array>;
extern template class ::rocket::optional<::asteria::V_object>;
#endif
