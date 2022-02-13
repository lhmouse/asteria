// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_BIT_MASK_HPP_
#define ROCKET_BIT_MASK_HPP_

#include "fwd.hpp"
#include "assert.hpp"

namespace rocket {

template<typename valueT>
class bit_mask
  {
    static_assert(is_unsigned<valueT>::value && !is_same<valueT, bool>::value,
        "`value_type` must be an unsigned integral type other than `bool`");

  public:
    using value_type = valueT;
    static constexpr size_t bit_count = sizeof(valueT) * 8;

  private:
    value_type m_value = valueT();

  public:
    constexpr
    bit_mask() noexcept
      { }

    constexpr
    bit_mask(initializer_list<size_t> indices) noexcept
      {
        for(size_t b : indices)
          this->set(b);
      }

  public:
    constexpr value_type
    value() const noexcept
      { return this->m_value;  }

    constexpr bool
    test(size_t b) const noexcept
      {
        ROCKET_ASSERT(b < bit_count);
        return (this->m_value >> b) & valueT(1);
      }

    constexpr bit_mask&
    set(size_t b, bool v = true) noexcept
      {
        ROCKET_ASSERT(b < bit_count);
        this->m_value &= ~(valueT(1) << b);
        this->m_value |= valueT(v) << b;
        return *this;
      }

    constexpr bit_mask&
    flip(size_t b) noexcept
      {
        ROCKET_ASSERT(b < bit_count);
        this->m_value ^= valueT(1) << b;
        return *this;
      }

    explicit constexpr operator
    bool() const noexcept
      { return this->value() == valueT();  }

    constexpr bool
    operator[](size_t b) const noexcept
      { return this->test(b);  }

    constexpr bit_mask&
    operator&=(const bit_mask& other) noexcept
      {
        this->m_value &= other.m_value;
        return *this;
      }

    constexpr bit_mask&
    operator^=(const bit_mask& other) noexcept
      {
        this->m_value ^= other.m_value;
        return *this;
      }

    constexpr bit_mask&
    operator|=(const bit_mask& other) noexcept
      {
        this->m_value |= other.m_value;
        return *this;
      }
  };

template<typename valueT>
constexpr bit_mask<valueT>
operator&(const bit_mask<valueT>& lhs, const bit_mask<valueT>& rhs) noexcept
  {
    auto temp = lhs;
    temp &= rhs;
    return temp;
  }

template<typename valueT>
constexpr bit_mask<valueT>
operator^(const bit_mask<valueT>& lhs, const bit_mask<valueT>& rhs) noexcept
  {
    auto temp = lhs;
    temp ^= rhs;
    return temp;
  }

template<typename valueT>
constexpr bit_mask<valueT>
operator|(const bit_mask<valueT>& lhs, const bit_mask<valueT>& rhs) noexcept
  {
    auto temp = lhs;
    temp |= rhs;
    return temp;
  }

}  // namespace rocket

#endif
