// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_
#  error Please include <asteria/value.hpp> instead.
#endif
namespace asteria {
namespace details_value {

template<typename xValue, bool xEnabled = true>
struct Valuable
  {
    static constexpr bool is_enabled = false;
    static constexpr bool is_noexcept = true;
  };

template<>
struct Valuable<Value>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage, typename xValue>
    static
    void
    assign(xStorage& stor, xValue&& xval)
      {
        stor = forward<xValue>(xval).m_stor;
      }
  };

template<>
struct Valuable<nullopt_t>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, V_null xval)
      {
        stor = xval;
      }
  };

template<>
struct Valuable<bool>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, V_boolean xval)
      {
        stor = xval;
      }
  };

template<>
struct Valuable<signed char>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, V_integer xval)
      {
        stor = xval;
      }
  };

template<>
struct Valuable<short>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, V_integer xval)
      {
        stor = xval;
      }
  };

template<>
struct Valuable<int>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, V_integer xval)
      {
        stor = xval;
      }
  };

template<>
struct Valuable<long>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, V_integer xval)
      {
        stor = xval;
      }
  };

template<>
struct Valuable<long long>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, V_integer xval)
      {
        stor = xval;
      }
  };

template<>
struct Valuable<float>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, V_real xval)
      {
        stor = xval;
      }
  };

template<>
struct Valuable<double>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, V_real xval)
      {
        stor = xval;
      }
  };

template<>
struct Valuable<cow_string>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage, typename xValue>
    static
    void
    assign(xStorage& stor, xValue&& xval)
      {
        stor = forward<xValue>(xval);
      }
  };

template<>
struct Valuable<cow_opaque>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage, typename xValue>
    static
    void
    assign(xStorage& stor, xValue&& xval)
      {
        stor = forward<xValue>(xval);
      }
  };

template<>
struct Valuable<cow_function>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage, typename xValue>
    static
    void
    assign(xStorage& stor, xValue&& xval)
      {
        stor = forward<xValue>(xval);
      }
  };

template<>
struct Valuable<cow_vector<Value>>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage, typename xValue>
    static
    void
    assign(xStorage& stor, xValue&& xval)
      {
        stor = forward<xValue>(xval);
      }
  };

template<>
struct Valuable<cow_dictionary<Value>>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage, typename xValue>
    static
    void
    assign(xStorage& stor, xValue&& xval)
      {
        stor = forward<xValue>(xval);
      }
  };

template<>
struct Valuable<cow_string::shallow_type>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, cow_string::shallow_type sh)
      {
        stor = V_string(sh);
      }
  };

template<size_t N>
struct Valuable<const char (*)[N]>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, const char (*ps)[N])
      {
        stor = V_string(ps);
      }
  };

template<typename xOpaque>
struct Valuable<refcnt_ptr<xOpaque>, ::std::is_base_of<Abstract_Opaque, xOpaque>::value>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage, typename xPointer>
    static
    void
    assign(xStorage& stor, xPointer&& xptr)
      {
        if(!xptr)
          stor = V_null();
        else
          stor = V_opaque(forward<xPointer>(xptr));
      }
  };

template<typename xFunction>
struct Valuable<refcnt_ptr<xFunction>, ::std::is_base_of<Abstract_Function, xFunction>::value>
  {
    static constexpr bool is_enabled = true;
    static constexpr bool is_noexcept = true;

    template<typename xStorage, typename xPointer>
    static
    void
    assign(xStorage& stor, xPointer&& xptr)
      {
        if(!xptr)
          stor = V_null();
        else
          stor = V_function(forward<xPointer>(xptr));
      }
  };

template<typename tValue>
struct Valuable<::rocket::optional<tValue>>
  {
    static constexpr bool is_enabled = Valuable<tValue>::is_enabled;
    static constexpr bool is_noexcept = Valuable<tValue>::is_noexcept;

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, const ::rocket::optional<tValue>& opt)
      {
        if(!opt)
          stor = V_null();
        else
          Valuable<tValue>::assign(stor, *opt);
      }

    template<typename xStorage>
    static
    void
    assign(xStorage& stor, ::rocket::optional<tValue>&& opt)
      {
        if(!opt)
          stor = V_null();
        else
          Valuable<tValue>::assign(stor, move(*opt));
      }
  };

template<typename xTuple, typename xSequence>
struct good_tuple;

template<typename xTuple, size_t... Ns>
struct good_tuple<xTuple, ::std::index_sequence<Ns...>>
  {
    static constexpr bool is_enabled =
        (Valuable<typename ::std::tuple_element<Ns, xTuple>::type>::is_enabled && ...);

    template<typename tTuple>
    static
    void
    assign_array(V_array& temp, tTuple&& tuple)
      {
        (Valuable<typename ::std::tuple_element<Ns, xTuple>::type>::assign(
                     temp.mut(Ns), ::std::get<Ns>(forward<tTuple>(tuple))), ...);
      }
  };

template<typename xTuple>
struct Valuable<xTuple, ::std::tuple_size<xTuple>::value >= 0>
  {
    using my_good_tuple = good_tuple<
        xTuple, ::std::make_index_sequence<::std::tuple_size<xTuple>::value>>;

    static constexpr bool is_enabled = my_good_tuple::is_enabled;
    static constexpr bool is_noexcept = false;

    template<typename xStorage, typename tTuple>
    static
    void
    assign(xStorage& stor, tTuple&& tuple)
      {
        V_array temp(::std::tuple_size<xTuple>::value);
        my_good_tuple::assign_array(temp, forward<tTuple>(tuple));
        stor = move(temp);
      }
  };

}  // namespace details_value
}  // namespace asteria
