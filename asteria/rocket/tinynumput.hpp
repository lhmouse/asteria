// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYNUMPUT_HPP_
#define ROCKET_TINYNUMPUT_HPP_

#include "utilities.hpp"
#include "assert.hpp"

namespace rocket {

class tinynumput
  {
  private:
    // These pointers may point to static, immutable storage.
    const char* m_bptr;
    const char* m_eptr;
    // This storage must be sufficient for the longest result, which at the moment
    // is signed 64-bit integer in binary (`"-0b111...1"` takes 68 bytes along with
    // the null terminator).
    // The size is a multiple of eight to prevent padding bytes.
    static constexpr size_t M = 71;
    char m_stor[M+1];

  public:
    tinynumput() noexcept
      {
        this->clear();
      }
    template<typename valueT, ROCKET_ENABLE_IF(is_scalar<valueT>::value)>
        explicit tinynumput(const valueT& value) noexcept
      {
        this->put(value);
      }

  public:
    // explicit formatters
    // * boolean
    tinynumput& put_TB(bool value) noexcept;
    // * pointer
    tinynumput& put_XP(const void* value) noexcept;
    // * unsigned 32-bit integer in binary
    tinynumput& put_BU(uint32_t value, size_t precision = 1) noexcept;
    // * unsigned 64-bit integer in binary
    tinynumput& put_BQ(uint64_t value, size_t precision = 1) noexcept;
    // * unsigned 32-bit integer in decimal
    tinynumput& put_DU(uint32_t value, size_t precision = 1) noexcept;
    // * unsigned 64-bit integer in decimal
    tinynumput& put_DQ(uint64_t value, size_t precision = 1) noexcept;
    // * unsigned 32-bit integer in hexadecimal
    tinynumput& put_XU(uint32_t value, size_t precision = 1) noexcept;
    // * unsigned 64-bit integer in hexadecimal
    tinynumput& put_XQ(uint64_t value, size_t precision = 1) noexcept;
    // * signed 32-bit integer in binary
    tinynumput& put_BI(int32_t value, size_t precision = 1) noexcept;
    // * signed 64-bit integer in binary
    tinynumput& put_BL(int64_t value, size_t precision = 1) noexcept;
    // * signed 32-bit integer in decimal
    tinynumput& put_DI(int32_t value, size_t precision = 1) noexcept;
    // * signed 64-bit integer in decimal
    tinynumput& put_DL(int64_t value, size_t precision = 1) noexcept;
    // * signed 32-bit integer in hexadecimal
    tinynumput& put_XI(int32_t value, size_t precision = 1) noexcept;
    // * signed 64-bit integer in hexadecimal
    tinynumput& put_XL(int64_t value, size_t precision = 1) noexcept;
    // * IEEE-754 double-precision floating-point in binary
    tinynumput& put_BF(double value) noexcept;
    // * IEEE-754 double-precision floating-point in binary scientific notation
    tinynumput& put_BE(double value) noexcept;
    // * IEEE-754 double-precision floating-point in hexadecimal
    tinynumput& put_XF(double value) noexcept;
    // * IEEE-754 double-precision floating-point in hexadecimal scientific notation
    tinynumput& put_XE(double value) noexcept;
    // * IEEE-754 double-precision floating-point in decimal
    tinynumput& put_DF(double value) noexcept;
    // * IEEE-754 double-precision floating-point in decimal scientific notation
    tinynumput& put_DE(double value) noexcept;

    // accessors
    const char* begin() const noexcept
      {
        return this->m_bptr;
      }
    const char* end() const noexcept
      {
        return this->m_eptr;
      }
    bool empty() const noexcept
      {
        return this->m_bptr == this->m_eptr;
      }
    size_t size() const noexcept
      {
        return static_cast<size_t>(this->m_eptr - this->m_bptr);
      }
    const char* data() const noexcept
      {
        return this->m_bptr;
      }
    tinynumput& clear() noexcept
      {
        this->m_bptr = this->m_stor;
        this->m_eptr = this->m_stor;
        this->m_stor[0] = 0;
        return *this;
      }

    // default formatters
    tinynumput& put(bool value) noexcept
      {
        return this->put_TB(value);
      }
    tinynumput& put(const void* value) noexcept
      {
        return this->put_XP(value);
      }
    tinynumput& put(unsigned char value) noexcept
      {
        return this->put_DU(value);
      }
    tinynumput& put(signed char value) noexcept
      {
        return this->put_DI(value);
      }
    tinynumput& put(unsigned short value) noexcept
      {
        return this->put_DU(value);
      }
    tinynumput& put(signed short value) noexcept
      {
        return this->put_DI(value);
      }
    tinynumput& put(unsigned value) noexcept
      {
        return this->put_DU(value);
      }
    tinynumput& put(signed value) noexcept
      {
        return this->put_DI(value);
      }
    tinynumput& put(unsigned long value) noexcept
      {
        return this->put_DQ(value);
      }
    tinynumput& put(signed long value) noexcept
      {
        return this->put_DL(value);
      }
    tinynumput& put(unsigned long long value) noexcept
      {
        return this->put_DQ(value);
      }
    tinynumput& put(signed long long value) noexcept
      {
        return this->put_DL(value);
      }
    tinynumput& put(float value) noexcept
      {
        return this->put_DF(value);
      }
    tinynumput& put(double value) noexcept
      {
        return this->put_DF(value);
      }
  };

}  // namespace rocket

#endif
