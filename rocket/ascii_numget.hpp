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
    bool m_ovfl;  // out of range; MSB lost
    bool m_udfl;  // too small; truncated to zero
    bool m_inxct;  // LSB lost

    // Parsing results
    bool m_sign;
    uint8_t m_cls;
    uint8_t m_base;
    bool m_more;

    int64_t m_exp;
    uint64_t m_mant;

  public:
    // Initializes the value zero.
    ascii_numget() noexcept
      {
        this->clear();
      }

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

    explicit operator
    bool() const noexcept
      { return this->m_ovfl == false;  }

    ascii_numget&
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
        return *this;
      }

    // Gets and sets the radix point.
    char
    radix_point() const noexcept
      { return this->m_rdxp;  }

    ascii_numget&
    set_radix_point(char rdxp) noexcept
      {
        this->m_rdxp = rdxp;
        return *this;
      }

    // * boolean as `true` or `false`
    ascii_numget&
    parse_TB(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * unsigned 64-bit integer in binary
    ascii_numget&
    parse_BU(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * unsigned 64-bit integer in hexadecimal
    ascii_numget&
    parse_XU(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * unsigned 64-bit integer in decimal
    ascii_numget&
    parse_DU(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * signed 64-bit integer in binary
    ascii_numget&
    parse_BI(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * signed 64-bit integer in hexadecimal
    ascii_numget&
    parse_XI(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * signed 64-bit integer in decimal
    ascii_numget&
    parse_DI(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * IEEE-754 double-precision floating-point number in binary
    ascii_numget&
    parse_BD(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * IEEE-754 double-precision floating-point number in hexadecimal
    ascii_numget&
    parse_XD(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * IEEE-754 double-precision floating-point number in decimal
    ascii_numget&
    parse_DD(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * unsigned 64-bit integer in any format
    ascii_numget&
    parse_U(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * signed 64-bit integer in any format
    ascii_numget&
    parse_I(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * IEEE-754 double-precision in any format
    ascii_numget&
    parse_D(const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept;

    // * unsigned 64-bit integer
    ascii_numget&
    cast_U(uint64_t& value, uint64_t min, uint64_t max) noexcept;

    // * signed 64-bit integer
    ascii_numget&
    cast_I(int64_t& value, int64_t min, int64_t max) noexcept;

    // * IEEE-754 single-precision floating-point
    ascii_numget&
    cast_F(float& value, float min, float max) noexcept;

    // * IEEE-754 double-precision floating-point
    ascii_numget&
    cast_D(double& value, double min, double max) noexcept;

    // These are easy functions that delegate to those above, passing
    // their default arguments. These functions are designed to take
    // strings in any format.
    ascii_numget&
    get(bool& value, const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept
      {
        uint64_t temp;
        this->parse_TB(str, len, outlen_opt);

        this->cast_U(temp, 0, 1);
        value = temp & 1;
        return *this;
      }

    ascii_numget&
    get(void*& value, const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept
      {
        uint64_t temp;
        this->parse_XU(str, len, outlen_opt);
        this->cast_U(temp, 0U, numeric_limits<uintptr_t>::max());
        value = (void*)(uintptr_t) temp;
        return *this;
      }

    template<typename valueT,
    ROCKET_ENABLE_IF(is_integral<valueT>::value && is_unsigned<valueT>::value)>
    ascii_numget&
    get(valueT& value, const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept
      {
        uint64_t temp;
        this->parse_U(str, len, outlen_opt);
        this->cast_U(temp, numeric_limits<valueT>::min(), numeric_limits<valueT>::max());
        value = (valueT) temp;
        return *this;
      }

    template<typename valueT,
    ROCKET_ENABLE_IF(is_integral<valueT>::value && is_signed<valueT>::value)>
    ascii_numget&
    get(valueT& value, const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept
      {
        int64_t temp;
        this->parse_I(str, len, outlen_opt);
        this->cast_I(temp, numeric_limits<valueT>::min(), numeric_limits<valueT>::max());
        value = (valueT) temp;
        return *this;
      }

    ascii_numget&
    get(float& value, const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept
      {
        this->parse_D(str, len, outlen_opt);
        this->cast_F(value, - numeric_limits<float>::infinity(), numeric_limits<float>::infinity());
        return *this;
      }

    ascii_numget&
    get(double& value, const char* str, size_t len, size_t* outlen_opt = nullptr) noexcept
      {
        this->parse_D(str, len, outlen_opt);
        this->cast_D(value, - numeric_limits<double>::infinity(), numeric_limits<double>::infinity());
        return *this;
      }
  };

}  // namespace rocket
#endif
