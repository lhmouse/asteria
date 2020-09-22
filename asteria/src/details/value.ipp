// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_HPP_
#  error Please include <asteria/value.hpp> instead.
#endif

namespace asteria {
namespace details_value {

// This controls implicit conversions to `Value` from other types.
// The main templte is undefined and is SFINAE-friendly.
// A member type alias `via_type` shall be provided, which shall designate one of the `V_*` candidates.
// If `nullable` is `false_type`, `ValT` shall be convertible to `via_type`.
template<typename ValT, typename = void>
struct Valuable_decayed;

template<>
struct Valuable_decayed<nullptr_t>
  {
    using via_type  = V_null;
    using nullable  = ::std::false_type;

    template<typename StorT>
    static
    void
    assign(StorT& stor, nullptr_t)
      {
        stor = V_null();
      }
  };

template<>
struct Valuable_decayed<bool>
  {
    using via_type  = V_boolean;
    using nullable  = ::std::false_type;

    template<typename StorT, typename XValT>
    static
    void
    assign(StorT& stor, XValT&& xval)
      {
        stor = V_boolean(xval);
      }
  };

template<typename IntegerT>
struct Valuable_decayed<IntegerT, typename ::std::enable_if<
            ::rocket::is_any_type_of<IntegerT,
                        signed char, short, int, long, long long>::value
        >::type>
  {
    using via_type  = V_integer;
    using nullable  = ::std::false_type;

    template<typename StorT, typename XValT>
    static
    void
    assign(StorT& stor, XValT&& xval)
      {
        stor = V_integer(xval);
      }
  };

template<typename FloatT>
struct Valuable_decayed<FloatT, typename ::std::enable_if<
            ::rocket::is_any_type_of<FloatT,
                                 float, double>::value
        >::type>
  {
    using via_type  = V_real;
    using nullable  = ::std::false_type;

    template<typename StorT, typename XValT>
    static
    void
    assign(StorT& stor, XValT&& xval)
      {
        stor = V_real(xval);
      }
  };

template<typename StringT>
struct Valuable_decayed<StringT, typename ::std::enable_if<
            ::rocket::is_any_type_of<StringT,
                        cow_string::shallow_type, cow_string>::value
        >::type>
  {
    using via_type  = V_string;
    using nullable  = ::std::false_type;

    template<typename StorT, typename XValT>
    static
    void
    assign(StorT& stor, XValT&& xval)
      {
        stor = V_string(::std::forward<XValT>(xval));
      }
  };

template<>
struct Valuable_decayed<cow_opaque>
  {
    using via_type  = V_opaque;
    using nullable  = ::std::true_type;

    template<typename StorT, typename XValT>
    static
    void
    assign(StorT& stor, XValT&& xval)
      {
        if(xval)
          stor = V_opaque(::std::forward<XValT>(xval));
        else
          stor = V_null();
      }
  };

template<typename OpaqueT>
struct Valuable_decayed<rcptr<OpaqueT>, typename ::std::enable_if<
            ::std::is_convertible<OpaqueT*,
                                  Abstract_Opaque*>::value
        >::type>
  {
    using via_type  = V_opaque;
    using nullable  = ::std::true_type;

    template<typename StorT, typename XValT>
    static
    void
    assign(StorT& stor, XValT&& xval)
      {
        if(xval)
          stor = V_opaque(::std::forward<XValT>(xval));
        else
          stor = V_null();
      }
  };

template<>
struct Valuable_decayed<cow_function>
  {
    using via_type  = V_function;
    using nullable  = ::std::true_type;

    template<typename StorT, typename XValT>
    static
    void
    assign(StorT& stor, XValT&& xval)
      {
        if(xval)
          stor = V_function(::std::forward<XValT>(xval));
        else
          stor = V_null();
      }
  };

template<typename FunctionT>
struct Valuable_decayed<rcptr<FunctionT>, typename ::std::enable_if<
            ::std::is_convertible<FunctionT*,
                                  const Abstract_Function*>::value
        >::type>
  {
    using via_type  = V_function;
    using nullable  = ::std::true_type;

    template<typename StorT, typename XValT>
    static
    void
    assign(StorT& stor, XValT&& xval)
      {
        if(xval)
          stor = V_function(::std::forward<XValT>(xval));
        else
          stor = V_null();
      }
  };

template<>
struct Valuable_decayed<cow_vector<Value>>
  {
    using via_type  = V_array;
    using nullable  = ::std::false_type;

    template<typename StorT, typename XValT>
    static
    void
    assign(StorT& stor, XValT&& xval)
      {
        stor = V_array(::std::forward<XValT>(xval));
      }
  };

template<>
struct Valuable_decayed<cow_dictionary<Value>>
  {
    using via_type  = V_object;
    using nullable  = ::std::false_type;

    template<typename StorT, typename XValT>
    static
    void
    assign(StorT& stor, XValT&& xval)
      {
        stor = V_object(::std::forward<XValT>(xval));
      }
  };

template<typename XValT>
struct Valuable_decayed<opt<XValT>, typename ::std::conditional<
               true, void, typename Valuable_decayed<XValT>::via_type
        >::type>
  {
    using via_type  = typename Valuable_decayed<XValT>::via_type;
    using nullable  = ::std::true_type;

    template<typename StorT, typename XOptT>
    static
    void
    assign(StorT& stor, XOptT&& xopt)
      {
        if(xopt)
          stor = static_cast<typename ::std::conditional<
                       ::std::is_lvalue_reference<XOptT&&>::value,
                                       const XValT&, XValT&&>::type>(*xopt);
        else
          stor = V_null();
      }
  };

template<typename XValT>
using Valuable = Valuable_decayed<typename ::std::decay<XValT>::type, void>;

}  // namespace details_value
}  // namespace asteria
