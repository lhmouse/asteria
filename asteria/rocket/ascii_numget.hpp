// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASCII_NUMGET_HPP_
#define ROCKET_ASCII_NUMGET_HPP_

#include "utilities.hpp"

namespace rocket {

class ascii_numget
  {
  private:
    union {
      struct {
        // These indicate characteristics of the result.
        union {
          struct {
            uint8_t m_succ : 1;  // parsed successfully
            uint8_t m_ovfl : 1;  // overflowed
            uint8_t m_udfl : 1;  // underflowed (floating point only)
            uint8_t m_inxc : 1;  // inexact
            uint8_t        : 4;
          };
          uint8_t m_stat;
        };
        // These compose the result number. [1]
        uint8_t m_vcls : 2;  // 0 finite, 1 infinitesimal, 2 infinity, 3 QNaN
        uint8_t m_sign : 1;  // sign bit
        uint8_t m_erdx : 1;  // base of exponent is `m_base` (binary otherwise)
        uint8_t m_madd : 1;  // non-zero digits follow `m_mant`
        uint8_t        : 3;
        uint8_t m_base : 8;  // radix
        uint8_t        : 8;
      };
      uint32_t m_bits;
    };
    // These compose the result number. [2]
    int32_t m_expo;  // the exponent (see `m_exp2`)
    uint64_t m_mant;  // the mantissa (see `m_base`)

  public:
    ascii_numget() noexcept
      {
        this->clear();
      }
    template<typename valueT, ROCKET_ENABLE_IF(is_scalar<valueT>::value)>
        ascii_numget(valueT& value, const char*& bptr, const char* eptr) noexcept
      {
        this->get(value, bptr, eptr);
      }

  public:
    // accessors
    explicit operator bool () const noexcept
      {
        return this->m_succ;
      }
    bool overflowed() const noexcept
      {
        return this->m_ovfl;
      }
    bool underflowed() const noexcept
      {
        return this->m_udfl;
      }
    bool inexact() const noexcept
      {
        return this->m_inxc;
      }

    bool is_finite() const noexcept
      {
        return this->m_vcls == 0;
      }
    bool is_infinitesimal() const noexcept
      {
        return this->m_vcls == 1;
      }
    bool is_infinity() const noexcept
      {
        return this->m_vcls == 2;
      }
    bool is_nan() const noexcept
      {
        return this->m_vcls == 3;
      }
    ascii_numget& clear() noexcept
      {
        this->m_bits = 0;
        this->m_expo = 0;
        this->m_mant = 0;
        return *this;
      }

    // explicit parse functions
    // * boolean
    ascii_numget& parse_B(const char*& bptr, const char* eptr) noexcept;
    // * pointer
    ascii_numget& parse_P(const char*& bptr, const char* eptr) noexcept;
    // * unsigned 64-bit integer
    ascii_numget& parse_U(const char*& bptr, const char* eptr) noexcept;
    // * signed 64-bit integer
    ascii_numget& parse_I(const char*& bptr, const char* eptr) noexcept;
    // * IEEE-754 double-precision floating-point
    ascii_numget& parse_F(const char*& bptr, const char* eptr) noexcept;

    // explicit case functions
    // * unsigned 64-bit integer
    ascii_numget& cast_U(uint64_t& value, uint64_t lower, uint64_t upper) noexcept;
    // * signed 64-bit integer
    ascii_numget& cast_I(int64_t& value, int64_t lower, int64_t upper) noexcept;
    // * IEEE-754 double-precision floating-point
    ascii_numget& cast_F(double& value, double lower, double upper, bool single = false) noexcept;

    // default parse functions
    ascii_numget& get(bool& value, const char*& bptr, const char* eptr) noexcept
      {
        value = false;
        if(this->parse_B(bptr, eptr)) {
          uint64_t temp;
          this->cast_U(temp, 0, 1);
          value = static_cast<bool>(static_cast<unsigned>(temp));
        }
        return *this;
      }
    ascii_numget& get(void*& value, const char*& bptr, const char* eptr) noexcept
      {
        value = nullptr;
        if(this->parse_P(bptr, eptr)) {
          uint64_t temp;
          this->cast_U(temp, 0, numeric_limits<uintptr_t>::max());
          value = reinterpret_cast<void*>(static_cast<uintptr_t>(temp));
        }
        return *this;
      }
    template<typename valueT, ROCKET_ENABLE_IF(is_integral<valueT>::value && is_unsigned<valueT>::value)>
        ascii_numget& get(valueT& value, const char*& bptr, const char* eptr) noexcept
      {
        value = 0;
        if(this->parse_U(bptr, eptr)) {
          uint64_t temp;
          this->cast_U(temp, numeric_limits<valueT>::min(),
                             numeric_limits<valueT>::max());
          value = static_cast<valueT>(temp);
        }
        return *this;
      }
    template<typename valueT, ROCKET_ENABLE_IF(is_integral<valueT>::value && is_signed<valueT>::value)>
        ascii_numget& get(valueT& value, const char*& bptr, const char* eptr) noexcept
      {
        value = 0;
        if(this->parse_I(bptr, eptr)) {
          int64_t temp;
          this->cast_I(temp, numeric_limits<valueT>::min(),
                             numeric_limits<valueT>::max());
          value = static_cast<valueT>(temp);
        }
        return *this;
      }
    template<typename valueT, ROCKET_ENABLE_IF(is_floating_point<valueT>::value)>
        ascii_numget& get(valueT& value, const char*& bptr, const char* eptr) noexcept
      {
        value = 0;
        if(this->parse_F(bptr, eptr)) {
          double temp;
          this->cast_F(temp, -numeric_limits<valueT>::infinity(),
                             numeric_limits<valueT>::infinity(), is_same<valueT, float>::value);
          value = static_cast<valueT>(temp);
        }
        return *this;
      }
  };

}  // namespace rocket

#endif
