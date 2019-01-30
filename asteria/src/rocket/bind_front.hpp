// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_BIND_FRONT_RESULT_HPP_
#define ROCKET_BIND_FRONT_RESULT_HPP_

#include "utilities.hpp"
#include "integer_sequence.hpp"

namespace rocket {

template<typename funcT, typename ...firstT>
 class bind_front_result
  {
    template<typename, typename...>
     friend class bind_front_result;

  public:
    template<typename ...restT>
     using result_type_for = decltype(::std::declval<const funcT &>()(::std::declval<const firstT &>()..., ::std::declval<restT>()...));

  private:
    funcT m_func;
    tuple<firstT...> m_first;

  public:
    // piesewise construct
    template<typename yfuncT, typename ...yfirstT, ROCKET_ENABLE_IF(is_constructible<tuple<firstT...>, yfirstT &&...>::value)>
     constexpr bind_front_result(yfuncT &&yfunc, firstT &&...yfirst) noexcept(conjunction<is_nothrow_constructible<funcT, yfuncT &&>,
                                                                                           is_nothrow_constructible<tuple<firstT...>, yfirstT &&...>>::value)
      : m_func(::std::forward<yfuncT>(yfunc)),
        m_first(::std::forward<firstT>(yfirst)...)
      {
      }
    // conversion
    template<typename yfuncT, typename ...yfirstT>
     constexpr bind_front_result(const bind_front_result<yfuncT, yfirstT...> &other) noexcept(conjunction<is_nothrow_constructible<funcT, const yfuncT &>,
                                                                                                           is_nothrow_constructible<tuple<firstT...>, const tuple<yfirstT...> &>>::value)
      : m_func(other.m_func),
        m_first(other.m_first)
      {
      }
    template<typename yfuncT, typename ...yfirstT>
     constexpr bind_front_result(bind_front_result<yfuncT, yfirstT...> &&other) noexcept(conjunction<is_nothrow_constructible<funcT, yfuncT &&>,
                                                                                          is_nothrow_constructible<tuple<firstT...>, tuple<yfirstT...> &&>>::value)
      : m_func(::std::move(other.m_func)),
        m_first(::std::move(other.m_first))
      {
      }
    template<typename yfuncT, typename ...yfirstT>
     bind_front_result & operator=(const bind_front_result<yfuncT, yfirstT...> &other) noexcept(conjunction<is_nothrow_assignable<funcT, const yfuncT &>,
                                                                                                             is_nothrow_assignable<tuple<firstT...>, const tuple<yfirstT...> &>>::value)
      {
        this->m_func = other.m_func;
        this->m_first = other.m_first;
        return *this;
      }
    template<typename yfuncT, typename ...yfirstT>
     bind_front_result & operator=(bind_front_result<yfuncT, yfirstT...> &&other) noexcept(conjunction<is_nothrow_assignable<funcT, yfuncT &&>,
                                                                                                        is_nothrow_assignable<tuple<firstT...>, tuple<yfirstT...> &&>>::value)
      {
        this->m_func = ::std::move(other.m_func);
        this->m_first = ::std::move(other.m_first);
        return *this;
      }

  private:
    template<typename ...restT, size_t ...indicesT>
     constexpr result_type_for<restT...> do_unpack_forward_then_invoke(index_sequence<indicesT...>, restT &...rest) const
      {
#if defined(__cpp_lib_invoke) && (__cpp_lib_invoke >= 201411)
        return ::std::invoke(this->m_func,
#else
        return this->m_func(
#endif
          ::std::get<indicesT>(this->m_first)..., ::std::forward<restT>(rest)...);
      }

  public:
    // accessors
    const funcT & target() const noexcept
      {
        return this->m_func;
      }
    template<typename ...restT>
     constexpr result_type_for<restT...> operator()(restT &&...rest) const
      {
        return this->do_unpack_forward_then_invoke<restT...>(index_sequence_for<firstT...>(), rest...);
      }
  };

template<typename xfuncT, typename ...xfirstT>
 constexpr bind_front_result<typename decay<xfuncT>::type, typename decay<xfirstT>::type...> bind_front(xfuncT &&xfunc, xfirstT &&...xfirst)
  {
    return { ::std::forward<xfuncT>(xfunc), ::std::forward<xfirstT>(xfirst)... };
  }

}

#endif
