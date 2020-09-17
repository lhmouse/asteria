// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_HPP_
#define ASTERIA_VALUE_HPP_

#include "fwd.hpp"

namespace asteria {

class Value
  {
  public:
    using Storage = variant<
      ROCKET_CDR(
      , V_null      // 0,
      , V_boolean   // 1,
      , V_integer   // 2,
      , V_real      // 3,
      , V_string    // 4,
      , V_opaque    // 5,
      , V_function  // 6,
      , V_array     // 7,
      , V_object    // 8,
      )>;

    static_assert(::std::is_nothrow_copy_assignable<Storage>::value);

  private:
    Storage m_stor;

  public:
    Value(nullptr_t = nullptr)
    noexcept
      { }

    Value(bool xval)
    noexcept
      : m_stor(V_boolean(xval))
      { }

    Value(signed char xval)
    noexcept
      : m_stor(V_integer(xval))
      { }

    Value(signed short xval)
    noexcept
      : m_stor(V_integer(xval))
      { }

    Value(signed xval)
    noexcept
      : m_stor(V_integer(xval))
      { }

    Value(signed long xval)
    noexcept
      : m_stor(V_integer(xval))
      { }

    Value(signed long long xval)
    noexcept
      : m_stor(V_integer(xval))
      { }

    Value(float xval)
    noexcept
      : m_stor(V_real(xval))
      { }

    Value(double xval)
    noexcept
      : m_stor(V_real(xval))
      { }

    Value(cow_string xval)
    noexcept
      : m_stor(::std::move(xval))
      { }

    Value(cow_string::shallow_type xval)
    noexcept
      : m_stor(V_string(xval))
      { }

    Value(cow_opaque xval)
    noexcept
      { this->do_xassign<V_opaque&&>(xval, ::std::addressof(xval));  }

    template<typename OpaqueT,
    ROCKET_ENABLE_IF(::std::is_convertible<OpaqueT*, Abstract_Opaque*>::value)>
    Value(rcptr<OpaqueT> xval)
    noexcept
      { this->do_xassign<V_opaque>(xval, ::std::addressof(xval));  }

    Value(cow_function xval)
    noexcept
      { this->do_xassign<V_function&&>(xval, ::std::addressof(xval));  }

    template<typename FunctionT,
    ROCKET_ENABLE_IF(::std::is_convertible<FunctionT*, Abstract_Function*>::value)>
    Value(rcptr<FunctionT> xval)
    noexcept
      { this->do_xassign<V_function>(xval, ::std::addressof(xval));  }

    Value(cow_vector<Value> xval)
    noexcept
      : m_stor(::std::move(xval))
      { }

    Value(cow_dictionary<Value> xval)
    noexcept
      : m_stor(::std::move(xval))
      { }

    Value(initializer_list<Value> list)
      : m_stor(V_array(list.begin(), list.end()))
      { }

    template<typename KeyT>
    Value(initializer_list<pair<KeyT, Value>> list)
      : m_stor(V_object(list.begin(), list.end()))
      { }

    Value(const opt<bool>& xval)
    noexcept
      { this->do_xassign<V_boolean>(xval, xval); }

    Value(const opt<signed char>& xval)
    noexcept
      { this->do_xassign<V_integer>(xval, xval);  }

    Value(const opt<signed short>& xval)
    noexcept
      { this->do_xassign<V_integer>(xval, xval);  }

    Value(const opt<signed>& xval)
    noexcept
      { this->do_xassign<V_integer>(xval, xval);  }

    Value(const opt<signed long>& xval)
    noexcept
      { this->do_xassign<V_integer>(xval, xval);  }

    Value(const opt<signed long long>& xval)
    noexcept
      { this->do_xassign<V_integer>(xval, xval);  }

    Value(const opt<float>& xval)
    noexcept
      { this->do_xassign<V_real>(xval, xval);  }

    Value(const opt<double>& xval)
    noexcept
      { this->do_xassign<V_real>(xval, xval);  }

    Value(const opt<cow_string>& xval)
    noexcept
      { this->do_xassign<const V_string&>(xval, xval);  }

    Value(opt<cow_string>&& xval)
    noexcept
      { this->do_xassign<V_string&&>(xval, xval);  }

    Value(const opt<cow_vector<Value>>& xval)
    noexcept
      { this->do_xassign<const V_array&>(xval, xval);  }

    Value(opt<cow_vector<Value>>&& xval)
    noexcept
      { this->do_xassign<V_array&&>(xval, xval);  }

    Value(const opt<cow_dictionary<Value>>& xval)
    noexcept
      { this->do_xassign<const V_object&>(xval, xval);  }

    Value(opt<cow_dictionary<Value>>&& xval)
    noexcept
      { this->do_xassign<V_object&&>(xval, xval);  }

    Value&
    operator=(nullptr_t)
    noexcept
      {
        this->m_stor = V_null();
        return *this;
      }

    Value&
    operator=(bool xval)
    noexcept
      {
        this->m_stor = V_boolean(xval);
        return *this;
      }

    Value&
    operator=(signed char xval)
    noexcept
      {
        this->m_stor = V_integer(xval);
        return *this;
      }

    Value&
    operator=(signed short xval)
    noexcept
      {
        this->m_stor = V_integer(xval);
        return *this;
      }

    Value&
    operator=(signed xval)
    noexcept
      {
        this->m_stor = V_integer(xval);
        return *this;
      }

    Value&
    operator=(signed long xval)
    noexcept
      {
        this->m_stor = V_integer(xval);
        return *this;
      }

    Value&
    operator=(signed long long xval)
    noexcept
      {
        this->m_stor = V_integer(xval);
        return *this;
      }

    Value&
    operator=(float xval)
    noexcept
      {
        this->m_stor = V_real(xval);
        return *this;
      }

    Value&
    operator=(double xval)
    noexcept
      {
        this->m_stor = V_real(xval);
        return *this;
      }

    Value&
    operator=(cow_string xval)
    noexcept
      {
        this->m_stor = V_string(::std::move(xval));
        return *this;
      }

    Value&
    operator=(cow_string::shallow_type xval)
    noexcept
      {
        this->m_stor = V_string(xval);
        return *this;
      }

    Value&
    operator=(cow_opaque xval)
    noexcept
      {
        this->do_xassign<V_opaque&&>(xval, ::std::addressof(xval));
        return *this;
      }

    template<typename OpaqueT,
    ROCKET_ENABLE_IF(::std::is_convertible<OpaqueT*, Abstract_Opaque*>::value)>
    Value&
    operator=(rcptr<OpaqueT> xval)
    noexcept
      {
        this->do_xassign<V_opaque>(xval, ::std::addressof(xval));
        return *this;
      }

    Value&
    operator=(cow_function xval)
    noexcept
      {
        this->do_xassign<V_function&&>(xval, ::std::addressof(xval));
        return *this;
      }

    template<typename FunctionT,
    ROCKET_ENABLE_IF(::std::is_convertible<FunctionT*, Abstract_Function*>::value)>
    Value&
    operator=(rcptr<FunctionT> xval)
    noexcept
      {
        this->do_xassign<V_function>(xval, ::std::addressof(xval));
        return *this;
      }

    Value&
    operator=(cow_vector<Value> xval)
    noexcept
      {
        this->m_stor = ::std::move(xval);
        return *this;
      }

    Value&
    operator=(cow_dictionary<Value> xval)
    noexcept
      {
        this->m_stor = ::std::move(xval);
        return *this;
      }

    Value&
    operator=(initializer_list<Value> list)
      {
        if(this->is_array())
          this->m_stor.as<type_array>().assign(list.begin(), list.end());
        else
          this->m_stor.emplace<type_array>(list);
        return *this;
      }

    template<typename KeyT>
    Value&
    operator=(initializer_list<pair<KeyT, Value>> list)
      {
        if(this->is_object())
          this->m_stor.as<type_object>().assign(list.begin(), list.end());
        else
          this->m_stor.emplace<type_object>(list);
        return *this;
      }

    Value&
    operator=(const opt<bool>& xval)
    noexcept
      {
        this->do_xassign<V_boolean>(xval, xval);
        return *this;
      }

    Value&
    operator=(const opt<signed char>& xval)
    noexcept
      {
        this->do_xassign<V_integer>(xval, xval);
        return *this;
      }

    Value&
    operator=(const opt<signed short>& xval)
    noexcept
      {
        this->do_xassign<V_integer>(xval, xval);
        return *this;
      }

    Value&
    operator=(const opt<signed>& xval)
    noexcept
      {
        this->do_xassign<V_integer>(xval, xval);
        return *this;
      }

    Value&
    operator=(const opt<signed long>& xval)
    noexcept
      {
        this->do_xassign<V_integer>(xval, xval);
        return *this;
      }

    Value&
    operator=(const opt<signed long long>& xval)
    noexcept
      {
        this->do_xassign<V_integer>(xval, xval);
        return *this;
      }

    Value&
    operator=(const opt<float>& xval)
    noexcept
      {
        this->do_xassign<V_real>(xval, xval);
        return *this;
      }

    Value&
    operator=(const opt<double>& xval)
    noexcept
      {
        this->do_xassign<V_real>(xval, xval);
        return *this;
      }

    Value&
    operator=(const opt<cow_string>& xval)
    noexcept
      {
        this->do_xassign<const V_string&>(xval, xval);
        return *this;
      }

    Value&
    operator=(opt<cow_string>&& xval)
    noexcept
      {
        this->do_xassign<V_string&&>(xval, xval);
        return *this;
      }

    Value&
    operator=(const opt<cow_vector<Value>>& xval)
    noexcept
      {
        this->do_xassign<const V_array&>(xval, xval);
        return *this;
      }

    Value&
    operator=(opt<cow_vector<Value>>&& xval)
    noexcept
      {
        this->do_xassign<V_array&&>(xval, xval);
        return *this;
      }

    Value&
    operator=(const opt<cow_dictionary<Value>>& xval)
    noexcept
      {
        this->do_xassign<const V_object&>(xval, xval);
        return *this;
      }

    Value&
    operator=(opt<cow_dictionary<Value>>&& xval)
    noexcept
      {
        this->do_xassign<V_object&&>(xval, xval);
        return *this;
      }

  private:
    template<typename CastT, typename ChkT, typename PtrT>
    void
    do_xassign(ChkT&& chk, PtrT&& ptr)
      {
        if(chk)
          this->m_stor = static_cast<CastT>(*ptr);
        else
          this->m_stor = V_null();
      }

  public:
    Type
    type()
    const noexcept
      { return static_cast<Type>(this->m_stor.index());  }

    const char*
    what_type()
    const noexcept
      { return describe_type(this->type());  }

    bool
    is_null()
    const noexcept
      { return this->type() == type_null;  }

    bool
    is_boolean()
    const noexcept
      { return this->type() == type_boolean;  }

    V_boolean
    as_boolean()
    const
      { return this->m_stor.as<type_boolean>();  }

    V_boolean&
    open_boolean()
      { return this->m_stor.as<type_boolean>();  }

    bool
    is_integer()
    const noexcept
      { return this->type() == type_integer;  }

    V_integer
    as_integer()
    const
      { return this->m_stor.as<type_integer>();  }

    V_integer&
    open_integer()
      { return this->m_stor.as<type_integer>();  }

    bool
    is_real()
    const noexcept
      { return this->type() == type_real;  }

    V_real
    as_real()
    const
      { return this->m_stor.as<type_real>();  }

    V_real&
    open_real()
      { return this->m_stor.as<type_real>();  }

    bool
    is_string()
    const noexcept
      { return this->type() == type_string;  }

    const V_string&
    as_string()
    const
      { return this->m_stor.as<type_string>();  }

    V_string&
    open_string()
      { return this->m_stor.as<type_string>();  }

    bool
    is_function()
    const noexcept
      { return this->type() == type_function;  }

    const V_function&
    as_function()
    const
      { return this->m_stor.as<type_function>();  }

    V_function&
    open_function()
      { return this->m_stor.as<type_function>();  }

    bool
    is_opaque()
    const noexcept
      { return this->type() == type_opaque;  }

    const V_opaque&
    as_opaque()
    const
      { return this->m_stor.as<type_opaque>();  }

    V_opaque&
    open_opaque()
      { return this->m_stor.as<type_opaque>();  }

    bool
    is_array()
    const noexcept
      { return this->type() == type_array;  }

    const V_array&
    as_array()
    const
      { return this->m_stor.as<type_array>();  }

    V_array&
    open_array()
      { return this->m_stor.as<type_array>();  }

    bool
    is_object()
    const noexcept
      { return this->type() == type_object;  }

    const V_object&
    as_object()
    const
      { return this->m_stor.as<type_object>();  }

    V_object&
    open_object()
      { return this->m_stor.as<type_object>();  }

    bool
    is_convertible_to_real()
    const noexcept
      { return this->is_integer() || this->is_real();  }

    V_real
    convert_to_real()
    const
      { return this->is_integer()
                 ? V_real(this->as_integer())
                 : this->as_real();  }

    V_real&
    mutate_into_real()
      { return this->is_integer()
                 ? this->m_stor.emplace<type_real>(V_real(this->as_integer()))
                 : this->open_real();  }

    Value&
    swap(Value& other)
    noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    // This performs the builtin conversion to boolean values.
    ROCKET_PURE_FUNCTION
    bool
    test()
    const noexcept;

    // This performs the builtin comparison with another value.
    ROCKET_PURE_FUNCTION
    Compare
    compare(const Value& other)
    const noexcept;

    // These functions are used by the garbage collector.
    // Read `runtime/collector.cpp` for details.
    ROCKET_PURE_FUNCTION
    bool
    unique()
    const noexcept;

    ROCKET_PURE_FUNCTION
    long
    use_count()
    const noexcept;

    ROCKET_PURE_FUNCTION
    long
    gcref_split()
    const noexcept;

    // These are miscellaneous interfaces for debugging.
    tinyfmt&
    print(tinyfmt& fmt, bool escape = false)
    const;

    tinyfmt&
    dump(tinyfmt& fmt, size_t indent = 2, size_t hanging = 0)
    const;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const;
  };

inline
void
swap(Value& lhs, Value& rhs)
noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const Value& value)
  { return value.print(fmt);  }

}  // namespace asteria

#endif
