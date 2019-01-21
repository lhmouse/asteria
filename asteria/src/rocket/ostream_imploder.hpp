// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_OSTREAM_IMPLODER_HPP_
#define ROCKET_OSTREAM_IMPLODER_HPP_

#include "utilities.hpp"

namespace rocket {

template<typename iteratorT, typename differenceT, typename delimiterT,
         typename filterT>
  class ostream_imploder
  {
  private:
    iteratorT m_begin;
    differenceT m_count;
    delimiterT m_delimiter;
    filterT m_filter;

  public:
    template<typename yiteratorT, typename ydifferenceT, typename ydelimiterT, typename yfilterT,
             ROCKET_ENABLE_IF(conjunction<is_constructible<iteratorT, yiteratorT &&>, is_constructible<differenceT, ydifferenceT &&>, is_constructible<delimiterT, ydelimiterT &&>,
                                          is_constructible<filterT, yfilterT &&>>::value)>
    constexpr ostream_imploder(yiteratorT &&ybegin, ydifferenceT &&ycount, ydelimiterT &&ydelimiter,
                               yfilterT &&yfilter) noexcept(conjunction<is_nothrow_constructible<iteratorT, yiteratorT &&>, is_nothrow_constructible<differenceT, ydifferenceT &&>,
                                                                        is_nothrow_constructible<delimiterT, ydelimiterT &&>, is_nothrow_constructible<filterT, yfilterT &&>>::value)
      : m_begin(::std::forward<yiteratorT>(ybegin)), m_count(::std::forward<ydifferenceT>(ycount)), m_delimiter(::std::forward<ydelimiterT>(ydelimiter)),
        m_filter(::std::forward<yfilterT>(yfilter))
      {
      }

  public:
    constexpr const iteratorT & begin() const noexcept
      {
        return this->m_begin;
      }
    constexpr const differenceT & count() const noexcept
      {
        return this->m_count;
      }
    constexpr const delimiterT & delimiter() const noexcept
      {
        return this->m_delimiter;
      }
    constexpr const filterT & filter() const noexcept
      {
        return this->m_filter;
      }
  };

template<typename charT, typename traitsT,
         typename iteratorT, typename differenceT, typename delimiterT, typename filterT>
  basic_ostream<charT, traitsT> & operator<<(basic_ostream<charT, traitsT> &os, const ostream_imploder<iteratorT, differenceT, delimiterT, filterT> &imploder)
  {
    auto cur = imploder.begin();
    auto rem = imploder.count();
    // Deal with nasty commas.
    switch(rem) {
    default:
      while(--rem != 0) {
        os << imploder.filter()(*cur) << imploder.delimiter();
        ++cur;
      }
      // Fallthrough.
    case 1:
      os << imploder.filter()(*cur);
      // Fallthrough.
    case 0:
      break;
    }
    return os;
  }

template<typename iteratorT, typename differenceT, typename delimiterT,
         typename filterT>
  constexpr ostream_imploder<typename decay<iteratorT>::type, typename decay<differenceT>::type, typename decay<delimiterT>::type,
                             typename decay<filterT>::type> ostream_implode(iteratorT &&begin, differenceT &&count, delimiterT &&delimiter,
                                                                            filterT &&filter)
  {
    return { ::std::forward<iteratorT>(begin), ::std::forward<differenceT>(count), ::std::forward<delimiterT>(delimiter),
             ::std::forward<filterT>(filter) };
  }

template<typename iteratorT, typename differenceT, typename delimiterT>
  constexpr ostream_imploder<typename decay<iteratorT>::type, typename decay<differenceT>::type, typename decay<delimiterT>::type,
                             identity> ostream_implode(iteratorT &&begin, differenceT &&count, delimiterT &&delimiter)
  {
    return { ::std::forward<iteratorT>(begin), ::std::forward<differenceT>(count), ::std::forward<delimiterT>(delimiter),
             identity() };
  }

}

#endif
