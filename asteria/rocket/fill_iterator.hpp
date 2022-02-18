// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FILL_ITERATOR_HPP_
#define ROCKET_FILL_ITERATOR_HPP_

#include "fwd.hpp"

namespace rocket {

template<typename elementT>
class fill_iterator
  {
  public:
    using iterator_category  = ::std::random_access_iterator_tag;
    using value_type         = typename remove_cv<elementT>::type;
    using pointer            = const elementT*;
    using reference          = const elementT&;
    using difference_type    = ptrdiff_t;

  private:
    difference_type m_pos;
    value_type m_value;

  public:
    template<typename... paramsT>
    explicit constexpr
    fill_iterator(difference_type pos, paramsT&&... params)
      noexcept(is_nothrow_constructible<value_type, paramsT&&...>::value)
      : m_pos(pos), m_value(::std::forward<paramsT>(params)...)
      { }

  public:
    constexpr difference_type
    tell() const noexcept
      { return this->m_pos;  }

    fill_iterator&
    seek(difference_type pos) noexcept
      { this->m_pos = pos;  return *this;  }

    constexpr reference
    operator*() const noexcept
      { return this->m_value;  }

    reference
    operator[](difference_type) const noexcept
      { return this->m_value;  }
  };

template<typename elementT>
fill_iterator<elementT>&
operator++(fill_iterator<elementT>& rhs) noexcept
  { return rhs.seek(rhs.tell() + 1);  }

template<typename elementT>
fill_iterator<elementT>&
operator--(fill_iterator<elementT>& rhs) noexcept
  { return rhs.seek(rhs.tell() - 1);  }

template<typename elementT>
fill_iterator<elementT>
operator++(fill_iterator<elementT>& lhs, int) noexcept
  {
    auto res = lhs;
    lhs.seek(lhs.tell() + 1);
    return res;
  }

template<typename elementT>
fill_iterator<elementT>
operator--(fill_iterator<elementT>& lhs, int) noexcept
  {
    auto res = lhs;
    lhs.seek(lhs.tell() - 1);
    return res;
  }

template<typename elementT>
fill_iterator<elementT>&
operator+=(fill_iterator<elementT>& lhs, typename fill_iterator<elementT>::difference_type rhs) noexcept
  { return lhs.seek(lhs.tell() + rhs);  }

template<typename elementT>
fill_iterator<elementT>&
operator-=(fill_iterator<elementT>& lhs, typename fill_iterator<elementT>::difference_type rhs) noexcept
  { return lhs.seek(lhs.tell() - rhs);  }

template<typename elementT>
fill_iterator<elementT>
operator+(const fill_iterator<elementT>& lhs, typename fill_iterator<elementT>::difference_type rhs) noexcept
  {
    auto res = lhs;
    res.seek(res.tell() + rhs);
    return res;
  }

template<typename elementT>
fill_iterator<elementT>
operator-(const fill_iterator<elementT>& lhs, typename fill_iterator<elementT>::difference_type rhs) noexcept
  {
    auto res = lhs;
    res.seek(res.tell() - rhs);
    return res;
  }

template<typename elementT>
fill_iterator<elementT>
operator+(typename fill_iterator<elementT>::difference_type lhs, const fill_iterator<elementT>& rhs) noexcept
  {
    auto res = rhs;
    res.seek(res.tell() + lhs);
    return res;
  }

template<typename elementT>
typename fill_iterator<elementT>::difference_type
operator-(const fill_iterator<elementT>& lhs, const fill_iterator<elementT>& rhs) noexcept
  {
    return lhs.tell() - rhs.tell();
  }

template<typename elementT>
bool
operator==(const fill_iterator<elementT>& lhs, const fill_iterator<elementT>& rhs) noexcept
  { return lhs.tell() == rhs.tell();  }

template<typename elementT>
bool
operator!=(const fill_iterator<elementT>& lhs, const fill_iterator<elementT>& rhs) noexcept
  { return lhs.tell() != rhs.tell();  }

template<typename elementT>
bool
operator<(const fill_iterator<elementT>& lhs, const fill_iterator<elementT>& rhs) noexcept
  { return lhs.tell() < rhs.tell();  }

template<typename elementT>
bool
operator>(const fill_iterator<elementT>& lhs, const fill_iterator<elementT>& rhs) noexcept
  { return lhs.tell() > rhs.tell();  }

template<typename elementT>
bool
operator<=(const fill_iterator<elementT>& lhs, const fill_iterator<elementT>& rhs) noexcept
  { return lhs.tell() <= rhs.tell();  }

template<typename elementT>
bool
operator>=(const fill_iterator<elementT>& lhs, const fill_iterator<elementT>& rhs) noexcept
  { return lhs.tell() >= rhs.tell();  }

}  // namespace rocket

#endif
