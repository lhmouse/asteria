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

    template<typename xStor, typename xValueT>
    static
    void
    assign(xStor& stor, xValueT&& xval)
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

    template<typename xStor, typename xValueT>
    static
    void
    assign(xStor& stor, xValueT&& xval)
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

    template<typename xStor, typename xValueT>
    static
    void
    assign(xStor& stor, xValueT&& xval)
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

    template<typename xStor, typename xValueT>
    static
    void
    assign(xStor& stor, xValueT&& xval)
      {
        stor = V_string(forward<xValueT>(xval));
      }
  };

template<size_t N>
struct Valuable_impl<const char (*)[N]>
  {
    using direct_init  = ::std::true_type;
    using via_type     = V_string;

    template<typename xStor, typename xValueT>
    static
    void
    assign(xStor& stor, xValueT&& xval)
      {
        stor = V_string(forward<xValueT>(xval));
      }
  };

template<>
struct Valuable_impl<cow_opaque>
  {
    using direct_init  = ::std::false_type;
    using via_type     = V_opaque;

    template<typename xStor, typename xValueT>
    static
    void
    assign(xStor& stor, xValueT&& xval)
      {
        if(xval)
          stor = V_opaque(forward<xValueT>(xval));
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

    template<typename xStor, typename xValueT>
    static
    void
    assign(xStor& stor, xValueT&& xval)
      {
        if(xval)
          stor = V_opaque(forward<xValueT>(xval));
        else
          stor = V_null();
      }
  };

template<>
struct Valuable_impl<cow_function>
  {
    using direct_init  = ::std::false_type;
    using via_type     = V_function;

    template<typename xStor, typename xValueT>
    static
    void
    assign(xStor& stor, xValueT&& xval)
      {
        if(xval)
          stor = V_function(forward<xValueT>(xval));
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

    template<typename xStor, typename xValueT>
    static
    void
    assign(xStor& stor, xValueT&& xval)
      {
        if(xval)
          stor = V_function(forward<xValueT>(xval));
        else
          stor = V_null();
      }
  };

template<>
struct Valuable_impl<V_array>
  {
    using direct_init  = ::std::true_type;
    using via_type     = V_array;

    template<typename xStor, typename xValueT>
    static
    void
    assign(xStor& stor, xValueT&& xval)
      {
        stor = V_array(forward<xValueT>(xval));
      }
  };

template<>
struct Valuable_impl<V_object>
  {
    using direct_init  = ::std::true_type;
    using via_type     = V_object;

    template<typename xStor, typename xValueT>
    static
    void
    assign(xStor& stor, xValueT&& xval)
      {
        stor = V_object(forward<xValueT>(xval));
      }
  };

template<typename xValueT, size_t N>
struct Valuable_impl<xValueT [N]>
  {
    using direct_init  = ::std::false_type;
    using via_type     = V_array;

    template<typename xStor, typename xArrT>
    static
    void
    assign(xStor& stor, xArrT&& xarr)
      {
        V_array arr;
        arr.reserve(N);
        for(size_t k = 0;  k != N;  ++k)
          arr.emplace_back(static_cast<typename ::std::conditional<
                  ::std::is_lvalue_reference<xArrT&&>::value,
                           const xValueT&, xValueT&&>::type>(xarr[k]));
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

    template<size_t... N, typename xTupT>
    static
    void
    unpack_tuple_aux(V_array& arr, ::std::index_sequence<N...>,
                     xTupT&& xtup)
      {
        int dummy[] = { (static_cast<void>(arr.emplace_back(
               ::std::get<N>(forward<xTupT>(xtup)))), 1)...  };
        (void)dummy;
      }

    template<typename xStor, typename xTupT>
    static
    void
    assign(xStor& stor, xTupT&& xtup)
      {
        V_array arr;
        constexpr size_t N = ::std::tuple_size<xTuple>::value;
        arr.reserve(N);
        unpack_tuple_aux(arr, ::std::make_index_sequence<N>(),
                        forward<xTupT>(xtup));
        stor = move(arr);
      }
  };

template<typename xValueT>
struct Valuable_impl<opt<xValueT>, typename ::std::conditional<
               true, void, typename Valuable_impl<xValueT>::via_type
        >::type>
  {
    using direct_init  = ::std::false_type;
    using via_type     = typename Valuable_impl<xValueT>::via_type;

    template<typename xStor, typename xOptT>
    static
    void
    assign(xStor& stor, xOptT&& xopt)
      {
        if(xopt)
          Valuable_impl<xValueT>::assign(stor,
              static_cast<typename ::std::conditional<
                  ::std::is_lvalue_reference<xOptT&&>::value,
                            const xValueT&, xValueT&&>::type>(*xopt));
        else
          stor = V_null();
      }
  };

template<typename xValueT>
using Valuable = Valuable_impl<typename ::rocket::remove_cvref<xValueT>::type, void>;

inline
void
do_break_line(tinyfmt& fmt, size_t step, size_t next)
  {
    if(step == 0) {
      // When `step` is zero, separate fields with a single space.
      fmt.putc(' ');
    }
    else {
      // Otherwise, terminate the current line, and indent the next.
      static constexpr char spaces[] = "                       ";
      fmt.putc('\n');
      for(size_t t = next, p;  (p = ::rocket::min(t, sizeof(spaces) - 1)) != 0;  t -= p)
        fmt.putn(spaces, p);
    }
  }

}  // namespace details_value
}  // namespace asteria
