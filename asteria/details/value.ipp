// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_
#  error Please include <asteria/value.hpp> instead.
#endif
namespace asteria {
namespace details_value {

// This controls implicit conversions to `Value` from other types.
// The main templte is undefined and is SFINAE-friendly.
// A member type alias `via_type` shall be provided, which shall designate
// one of the `V_*` candidates.
template<typename xVal, typename = void>
struct Valuable_impl;

template<>
struct Valuable_impl<nullopt_t>
  {
    using direct_init  = ::std::true_type;
    using via_type     = V_null;

    template<typename xStor>
    static
    void
    assign(xStor& stor, nullopt_t)
      {
        stor = V_null();
      }
  };

template<>
struct Valuable_impl<bool>
  {
    using direct_init  = ::std::true_type;
    using via_type     = V_boolean;

    template<typename xStor, typename xValue>
    static
    void
    assign(xStor& stor, xValue&& xval)
      {
        stor = V_boolean(xval);
      }
  };

template<typename xInteger>
struct Valuable_impl<xInteger, typename ::std::enable_if<
            ::rocket::is_any_type_of<xInteger,
                        signed char, short, int, long, long long>::value
        >::type>
  {
    using direct_init  = ::std::true_type;
    using via_type     = V_integer;

    template<typename xStor, typename xValue>
    static
    void
    assign(xStor& stor, xValue&& xval)
      {
        stor = V_integer(xval);
      }
  };

template<typename xFloat>
struct Valuable_impl<xFloat, typename ::std::enable_if<
            ::rocket::is_any_type_of<xFloat,
                                 float, double>::value
        >::type>
  {
    using direct_init  = ::std::true_type;
    using via_type     = V_real;

    template<typename xStor, typename xValue>
    static
    void
    assign(xStor& stor, xValue&& xval)
      {
        stor = V_real(xval);
      }
  };

template<typename xString>
struct Valuable_impl<xString, typename ::std::enable_if<
            ::rocket::is_any_type_of<xString,
                        cow_string::shallow_type, cow_string>::value
        >::type>
  {
    using direct_init  = ::std::true_type;
    using via_type     = V_string;

    template<typename xStor, typename xValue>
    static
    void
    assign(xStor& stor, xValue&& xval)
      {
        stor = V_string(forward<xValue>(xval));
      }
  };

template<size_t N>
struct Valuable_impl<const char (*)[N]>
  {
    using direct_init  = ::std::true_type;
    using via_type     = V_string;

    template<typename xStor, typename xValue>
    static
    void
    assign(xStor& stor, xValue&& xval)
      {
        stor = V_string(forward<xValue>(xval));
      }
  };

template<>
struct Valuable_impl<cow_opaque>
  {
    using direct_init  = ::std::false_type;
    using via_type     = V_opaque;

    template<typename xStor, typename xValue>
    static
    void
    assign(xStor& stor, xValue&& xval)
      {
        if(xval)
          stor = V_opaque(forward<xValue>(xval));
        else
          stor = V_null();
      }
  };

template<typename xOpaque>
struct Valuable_impl<refcnt_ptr<xOpaque>, typename ::std::enable_if<
            ::std::is_convertible<xOpaque*,
                                  Abstract_Opaque*>::value
        >::type>
  {
    using direct_init  = ::std::false_type;
    using via_type     = V_opaque;

    template<typename xStor, typename xValue>
    static
    void
    assign(xStor& stor, xValue&& xval)
      {
        if(xval)
          stor = V_opaque(forward<xValue>(xval));
        else
          stor = V_null();
      }
  };

template<>
struct Valuable_impl<cow_function>
  {
    using direct_init  = ::std::false_type;
    using via_type     = V_function;

    template<typename xStor, typename xValue>
    static
    void
    assign(xStor& stor, xValue&& xval)
      {
        if(xval)
          stor = V_function(forward<xValue>(xval));
        else
          stor = V_null();
      }
  };

template<typename xFunction>
struct Valuable_impl<refcnt_ptr<xFunction>, typename ::std::enable_if<
            ::std::is_convertible<xFunction*,
                                  const Abstract_Function*>::value
        >::type>
  {
    using direct_init  = ::std::false_type;
    using via_type     = V_function;

    template<typename xStor, typename xValue>
    static
    void
    assign(xStor& stor, xValue&& xval)
      {
        if(xval)
          stor = V_function(forward<xValue>(xval));
        else
          stor = V_null();
      }
  };

template<>
struct Valuable_impl<V_array>
  {
    using direct_init  = ::std::true_type;
    using via_type     = V_array;

    template<typename xStor, typename xValue>
    static
    void
    assign(xStor& stor, xValue&& xval)
      {
        stor = V_array(forward<xValue>(xval));
      }
  };

template<>
struct Valuable_impl<V_object>
  {
    using direct_init  = ::std::true_type;
    using via_type     = V_object;

    template<typename xStor, typename xValue>
    static
    void
    assign(xStor& stor, xValue&& xval)
      {
        stor = V_object(forward<xValue>(xval));
      }
  };

template<typename xValue, size_t N>
struct Valuable_impl<xValue [N]>
  {
    using direct_init  = ::std::false_type;
    using via_type     = V_array;

    template<typename xStor, typename xArr>
    static
    void
    assign(xStor& stor, xArr&& xarr)
      {
        V_array arr;
        arr.reserve(N);
        for(size_t k = 0;  k != N;  ++k)
          arr.emplace_back(static_cast<typename ::std::conditional<
                  ::std::is_lvalue_reference<xArr&&>::value,
                           const xValue&, xValue&&>::type>(xarr[k]));
        stor = move(arr);
      }
  };

template<typename xTuple>
struct Valuable_impl<xTuple, typename ::std::conditional<
               bool(::std::tuple_size<xTuple>::value), void, void
        >::type>
  {
    using direct_init  = ::std::false_type;
    using via_type     = V_array;

    template<size_t... N, typename xTup>
    static
    void
    unpack_tuple_aux(V_array& arr, ::std::index_sequence<N...>,
                     xTup&& xtup)
      {
        int dummy[] = { (static_cast<void>(arr.emplace_back(
               ::std::get<N>(forward<xTup>(xtup)))), 1)...  };
        (void)dummy;
      }

    template<typename xStor, typename xTup>
    static
    void
    assign(xStor& stor, xTup&& xtup)
      {
        V_array arr;
        constexpr size_t N = ::std::tuple_size<xTuple>::value;
        arr.reserve(N);
        unpack_tuple_aux(arr, ::std::make_index_sequence<N>(),
                        forward<xTup>(xtup));
        stor = move(arr);
      }
  };

template<typename xValue>
struct Valuable_impl<opt<xValue>, typename ::std::conditional<
               true, void, typename Valuable_impl<xValue>::via_type
        >::type>
  {
    using direct_init  = ::std::false_type;
    using via_type     = typename Valuable_impl<xValue>::via_type;

    template<typename xStor, typename xOpt>
    static
    void
    assign(xStor& stor, xOpt&& xopt)
      {
        if(xopt)
          Valuable_impl<xValue>::assign(stor,
              static_cast<typename ::std::conditional<
                  ::std::is_lvalue_reference<xOpt&&>::value,
                            const xValue&, xValue&&>::type>(*xopt));
        else
          stor = V_null();
      }
  };

template<typename xValue>
using Valuable = Valuable_impl<typename ::rocket::remove_cvref<xValue>::type, void>;

}  // namespace details_value
}  // namespace asteria
