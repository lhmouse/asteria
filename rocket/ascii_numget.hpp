// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASCII_NUMGET_
#define ROCKET_ASCII_NUMGET_

#include "fwd.hpp"
namespace rocket {

class ascii_numget
  {
  private:
    // Configuration
    char m_rdxp = '.';

    // Casting results
    bool m_ovfl = false;  // out of range; MSB lost
    bool m_udfl = false;  // too small; truncated to zero
    bool m_inxct = false;  // LSB lost

    // Parsing results
    bool m_sign = false;
    uint8_t m_cls = 0;
    uint8_t m_base = 0;
    bool m_more = false;

    int64_t m_exp = 0;
    uint64_t m_mant = 0;

  public:
    // Initializes the value zero.
    ascii_numget() noexcept = default;

    ascii_numget(const ascii_numget&) = delete;

    ascii_numget&
    operator=(const ascii_numget&) = delete;

  public:
    // accessors
    bool
    overflowed() const noexcept
      { return this->m_ovfl;  }

    bool
    underflowed() const noexcept
      { return this->m_udfl;  }

    bool
    inexact() const noexcept
      { return this->m_inxct;  }

    void
    clear() noexcept
      {
        this->m_sign = false;
        this->m_cls = 0;
        this->m_base = 0;
        this->m_exp = 0;
        this->m_mant = 0;
        this->m_more = false;
        this->m_ovfl = false;
        this->m_udfl = false;
        this->m_inxct = false;
      }

    // Gets and sets the radix point.
    char
    radix_point() const noexcept
      { return this->m_rdxp;  }

    void
    set_radix_point(char rdxp) noexcept
      { this->m_rdxp = rdxp;  }

    // * boolean as `true` or `false`
    size_t
    parse_TB(const char* str, size_t len) noexcept;

    // * unsigned 64-bit integer in binary
    size_t
    parse_BU(const char* str, size_t len) noexcept;

    // * unsigned 64-bit integer in hexadecimal
    size_t
    parse_XU(const char* str, size_t len) noexcept;

    // * unsigned 64-bit integer in decimal
    size_t
    parse_DU(const char* str, size_t len) noexcept;

    // * signed 64-bit integer in binary
    size_t
    parse_BI(const char* str, size_t len) noexcept;

    // * signed 64-bit integer in hexadecimal
    size_t
    parse_XI(const char* str, size_t len) noexcept;

    // * signed 64-bit integer in decimal
    size_t
    parse_DI(const char* str, size_t len) noexcept;

    // * IEEE-754 double-precision floating-point number in binary
    size_t
    parse_BD(const char* str, size_t len) noexcept;

    // * IEEE-754 double-precision floating-point number in hexadecimal
    size_t
    parse_XD(const char* str, size_t len) noexcept;

    // * IEEE-754 double-precision floating-point number in decimal
    size_t
    parse_DD(const char* str, size_t len) noexcept;

    // * unsigned 64-bit integer in any format
    size_t
    parse_U(const char* str, size_t len) noexcept;

    // * signed 64-bit integer in any format
    size_t
    parse_I(const char* str, size_t len) noexcept;

    // * IEEE-754 double-precision in any format
    size_t
    parse_D(const char* str, size_t len) noexcept;

    // * unsigned 64-bit integer
    void
    cast_U(uint64_t& value, uint64_t min, uint64_t max) noexcept;

    // * signed 64-bit integer
    void
    cast_I(int64_t& value, int64_t min, int64_t max) noexcept;

    // * IEEE-754 single-precision floating-point
    void
    cast_F(float& value, float min, float max) noexcept;

    // * IEEE-754 double-precision floating-point
    void
    cast_D(double& value, double min, double max) noexcept;

    // These functions delegate to those above. These are designed to
    // take strings in any format, and return the number of characters
    // that have been parsed successfully. Zero is returned if an input
    // string doesn't contain a number.
    void
    get(bool& value, const char* str, size_t len, size_t* acclen_opt = nullptr) noexcept
      {
        size_t acclen_temp;
        size_t& acclen = acclen_opt ? *acclen_opt : acclen_temp;
        acclen = this->parse_TB(str, len);

        int64_t temp;
        this->cast_I(temp, -1, 0);
        value = temp < 0;
      }

    void
    get(void*& value, const char* str, size_t len, size_t* acclen_opt = nullptr) noexcept
      {
        size_t acclen_temp;
        size_t& acclen = acclen_opt ? *acclen_opt : acclen_temp;
        acclen = this->parse_XU(str, len);

        uint64_t temp;
        constexpr auto min = numeric_limits<uintptr_t>::min();
        constexpr auto max = numeric_limits<uintptr_t>::max();
        this->cast_U(temp, min, max);
        value = (void*)(uintptr_t) temp;
      }

    template<typename valueT,
    ROCKET_ENABLE_IF(is_integral<valueT>::value && is_unsigned<valueT>::value)>
    void
    get(valueT& value, const char* str, size_t len, size_t* acclen_opt = nullptr) noexcept
      {
        size_t acclen_temp;
        size_t& acclen = acclen_opt ? *acclen_opt : acclen_temp;
        acclen = this->parse_U(str, len);

        uint64_t temp;
        constexpr auto min = numeric_limits<valueT>::min();
        constexpr auto max = numeric_limits<valueT>::max();
        this->cast_U(temp, min, max);
        value = (valueT) temp;
      }

    template<typename valueT,
    ROCKET_ENABLE_IF(is_integral<valueT>::value && is_signed<valueT>::value)>
    void
    get(valueT& value, const char* str, size_t len, size_t* acclen_opt = nullptr) noexcept
      {
        size_t acclen_temp;
        size_t& acclen = acclen_opt ? *acclen_opt : acclen_temp;
        acclen = this->parse_I(str, len);

        int64_t temp;
        constexpr auto min = numeric_limits<valueT>::min();
        constexpr auto max = numeric_limits<valueT>::max();
        this->cast_I(temp, min, max);
        value = (valueT) temp;
      }

    void
    get(float& value, const char* str, size_t len, size_t* acclen_opt = nullptr) noexcept
      {
        size_t acclen_temp;
        size_t& acclen = acclen_opt ? *acclen_opt : acclen_temp;
        acclen = this->parse_D(str, len);

        constexpr auto inf = numeric_limits<float>::infinity();
        this->cast_F(value, -inf, +inf);
      }

    void
    get(double& value, const char* str, size_t len, size_t* acclen_opt = nullptr) noexcept
      {
        size_t acclen_temp;
        size_t& acclen = acclen_opt ? *acclen_opt : acclen_temp;
        acclen = this->parse_D(str, len);

        constexpr auto inf = numeric_limits<double>::infinity();
        this->cast_D(value, -inf, +inf);
      }
  };

}  // namespace rocket
#endif
