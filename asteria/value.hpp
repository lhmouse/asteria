// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_
#define ASTERIA_VALUE_

#include "fwd.hpp"
#include "details/value.ipp"
namespace asteria {

class Value
  {
  private:
    template<typename xValue, typename xEnable>
    friend struct details_value::Valuable;

    template<typename xValue>
    using my_Valuable = details_value::Valuable<
            typename ::rocket::remove_cvref<xValue>::type>;

    using variant_type = ::rocket::variant<ASTERIA_TYPES_AIXE9XIG_(V)>;
    using bytes_type = ::std::aligned_storage<sizeof(variant_type), 16>::type;

    union {
      bytes_type m_bytes;
      variant_type m_stor;
    };

  public:
    // Constructors and assignment operators
    constexpr
    Value(nullopt_t = nullopt) noexcept
      :
        m_bytes()
      { }

    template<typename xValue,
    ROCKET_ENABLE_IF(my_Valuable<xValue>::is_enabled)>
    Value(xValue&& xval) noexcept(my_Valuable<xValue>::is_noexcept)
      :
        m_bytes()
      {
        ROCKET_ASSERT(this->m_stor.index() == 0);
        my_Valuable<xValue>::assign(this->m_stor, forward<xValue>(xval));
      }

    template<typename xValue,
    ROCKET_ENABLE_IF(my_Valuable<xValue>::is_enabled)>
    Value&
    operator=(xValue&& xval) & noexcept(my_Valuable<xValue>::is_noexcept)
      {
        my_Valuable<xValue>::assign(this->m_stor, forward<xValue>(xval));
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
        if(ROCKET_UNEXPECT(this->m_stor.index() > type_real))
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
        if(ROCKET_EXPECT(this->m_stor.index() == type_boolean))
          return this->m_stor.as<V_boolean>();

        this->do_throw_type_mismatch("`boolean`");
      }

    V_boolean&
    mut_boolean() noexcept
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_boolean))
          return this->m_stor.mut<V_boolean>();

        return this->m_stor.emplace<V_boolean>();
      }

    bool
    is_integer() const noexcept
      { return this->m_stor.index() == type_integer;  }

    V_integer
    as_integer() const
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_integer))
          return this->m_stor.as<V_integer>();

        this->do_throw_type_mismatch("`integer`");
      }

    V_integer&
    mut_integer() noexcept
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_integer))
          return this->m_stor.mut<V_integer>();

        return this->m_stor.emplace<V_integer>();
      }

    bool
    is_real() const noexcept
      { return (this->type() >= type_integer) && (this->type() <= type_real);  }

    V_real
    as_real() const
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_real))
          return this->m_stor.as<V_real>();

        if(ROCKET_EXPECT(this->m_stor.index() == type_integer))
          return static_cast<V_real>(this->m_stor.as<V_integer>());

        this->do_throw_type_mismatch("`integer` or `real`");
      }

    V_real&
    mut_real() noexcept
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_real))
          return this->m_stor.mut<V_real>();

        if(ROCKET_EXPECT(this->m_stor.index() == type_integer))
          return this->m_stor.emplace<V_real>(static_cast<V_real>(this->m_stor.mut<V_integer>()));

        return this->m_stor.emplace<V_real>();
      }

    bool
    is_string() const noexcept
      { return this->m_stor.index() == type_string;  }

    const V_string&
    as_string() const
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_string))
          return this->m_stor.as<V_string>();

        this->do_throw_type_mismatch("`string`");
      }

    V_string&
    mut_string() noexcept
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_string))
          return this->m_stor.mut<V_string>();

        return this->m_stor.emplace<V_string>();
      }

    bool
    is_function() const noexcept
      { return this->m_stor.index() == type_function;  }

    const V_function&
    as_function() const
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_function))
          return this->m_stor.as<V_function>();

        this->do_throw_type_mismatch("`function`");
      }

    V_function&
    mut_function() noexcept
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_function))
          return this->m_stor.mut<V_function>();

        return this->m_stor.emplace<V_function>();
      }

    bool
    is_opaque() const noexcept
      { return this->m_stor.index() == type_opaque;  }

    const V_opaque&
    as_opaque() const
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_opaque))
          return this->m_stor.as<V_opaque>();

        this->do_throw_type_mismatch("`opaque`");
      }

    V_opaque&
    mut_opaque() noexcept
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_opaque))
          return this->m_stor.mut<V_opaque>();

        return this->m_stor.emplace<V_opaque>();
      }

    bool
    is_array() const noexcept
      { return this->m_stor.index() == type_array;  }

    const V_array&
    as_array() const
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_array))
          return this->m_stor.as<V_array>();

        this->do_throw_type_mismatch("`array`");
      }

    V_array&
    mut_array() noexcept
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_array))
          return this->m_stor.mut<V_array>();

        return this->m_stor.emplace<V_array>();
      }

    bool
    is_object() const noexcept
      { return this->m_stor.index() == type_object;  }

    const V_object&
    as_object() const
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_object))
          return this->m_stor.as<V_object>();

        this->do_throw_type_mismatch("`object`");
      }

    V_object&
    mut_object() noexcept
      {
        if(ROCKET_EXPECT(this->m_stor.index() == type_object))
          return this->m_stor.mut<V_object>();

        return this->m_stor.emplace<V_object>();
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
        switch(this->m_stor.index())
          {
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
    print_to(tinyfmt& fmt) const;

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
  { return value.print_to(fmt);  }

}  // namespace asteria
extern template class ::rocket::variant<ASTERIA_TYPES_AIXE9XIG_(::asteria::V)>;
extern template class ::rocket::optional<::asteria::Value>;
extern template class ::rocket::cow_vector<::asteria::Value>;
extern template class ::rocket::cow_hashmap<::asteria::phcow_string,
  ::asteria::Value, ::asteria::phcow_string::hash>;
#endif
