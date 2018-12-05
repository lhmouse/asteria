// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_BIND_FIRST_HPP_
#define ROCKET_BIND_FIRST_HPP_

#include "utilities.hpp"
#include "integer_sequence.hpp"
#include <tuple>  // std::tuple<>

namespace rocket {

template<typename funcT, typename ...firstT>
  class binder_first
  {
  public:
    template<typename ...restT>
      using result_type_for = decltype(::std::declval<const funcT &>()(::std::declval<const firstT &>()..., ::std::declval<restT>()...));

  private:
    funcT m_func;
    ::std::tuple<firstT...> m_first;

  public:
    template<typename xfuncT, typename ...xfirstT>
      constexpr binder_first(xfuncT &&xfunc, firstT &&...xfirst)
      : m_func(::std::forward<xfuncT>(xfunc)), m_first(::std::forward<firstT>(xfirst)...)
      {
      }

  private:
    template<typename ...restT, size_t ...indicesT>
      constexpr result_type_for<restT...> do_unpack_forward_then_invoke(index_sequence<indicesT...>, restT &...rest) const
      {
        return this->m_func(::std::get<indicesT>(this->m_first)..., ::std::forward<restT>(rest)...);
      }

  public:
    template<typename ...restT>
      constexpr result_type_for<restT...> operator()(restT &&...rest) const
      {
        return this->do_unpack_forward_then_invoke<restT...>(index_sequence_for<firstT...>(), rest...);
      }
  };

template<typename xfuncT, typename ...xfirstT>
  constexpr binder_first<typename decay<xfuncT>::type, typename decay<xfirstT>::type...> bind_first(xfuncT &&xfunc, xfirstT &&...xfirst)
  {
    return binder_first<typename decay<xfuncT>::type, typename decay<xfirstT>::type...>(::std::forward<xfuncT>(xfunc), ::std::forward<xfirstT>(xfirst)...);
  }

}

#endif
