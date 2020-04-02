// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASCII_NUMPUT_HPP_
#define ROCKET_ASCII_NUMPUT_HPP_

#include "utilities.hpp"

namespace rocket {

class ascii_numput
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
    ascii_numput() noexcept
      { this->clear();  }

    template<typename valueT, ROCKET_ENABLE_IF(is_scalar<valueT>::value)>
                            explicit ascii_numput(const valueT& value) noexcept
      { this->put(value);  }

  public:
    // accessors
    const char* begin() const noexcept
      { return this->m_bptr;  }

    const char* end() const noexcept
      { return this->m_eptr;  }

    bool empty() const noexcept
      { return this->m_bptr == this->m_eptr;  }

    size_t size() const noexcept
      { return static_cast<size_t>(this->m_eptr - this->m_bptr);  }

    const char* data() const noexcept
      { return this->m_bptr;  }

    ascii_numput& clear() noexcept
      {
        this->m_bptr = this->m_stor;
        this->m_eptr = this->m_stor;
        this->m_stor[0] = 0;
        return *this;
      }

    // explicit format functions
    // * boolean
    ascii_numput& put_TB(bool value) noexcept;
    // * pointer
    ascii_numput& put_XP(const void* value) noexcept;
    // * unsigned 64-bit integer in binary
    ascii_numput& put_BU(uint64_t value, size_t precision = 1) noexcept;
    // * unsigned 64-bit integer in hexadecimal
    ascii_numput& put_XU(uint64_t value, size_t precision = 1) noexcept;
    // * unsigned 64-bit integer in decimal
    ascii_numput& put_DU(uint64_t value, size_t precision = 1) noexcept;
    // * signed 64-bit integer in binary
    ascii_numput& put_BI(int64_t value, size_t precision = 1) noexcept;
    // * signed 64-bit integer in hexadecimal
    ascii_numput& put_XI(int64_t value, size_t precision = 1) noexcept;
    // * signed 64-bit integer in decimal
    ascii_numput& put_DI(int64_t value, size_t precision = 1) noexcept;
    // * IEEE-754 double-precision floating-point in binary
    ascii_numput& put_BF(double value, bool single = false) noexcept;
    // * IEEE-754 double-precision floating-point in binary scientific notation
    ascii_numput& put_BE(double value, bool single = false) noexcept;
    // * IEEE-754 double-precision floating-point in hexadecimal
    ascii_numput& put_XF(double value, bool single = false) noexcept;
    // * IEEE-754 double-precision floating-point in hexadecimal scientific notation
    ascii_numput& put_XE(double value, bool single = false) noexcept;
    // * IEEE-754 double-precision floating-point in decimal
    ascii_numput& put_DF(double value, bool single = false) noexcept;
    // * IEEE-754 double-precision floating-point in decimal scientific notation
    ascii_numput& put_DE(double value, bool single = false) noexcept;

    // default format functions
    ascii_numput& put(bool value) noexcept;
    ascii_numput& put(const void* value) noexcept;
    ascii_numput& put(unsigned char value) noexcept;
    ascii_numput& put(unsigned short value) noexcept;
    ascii_numput& put(unsigned value) noexcept;
    ascii_numput& put(unsigned long value) noexcept;
    ascii_numput& put(unsigned long long value) noexcept;
    ascii_numput& put(signed char value) noexcept;
    ascii_numput& put(signed short value) noexcept;
    ascii_numput& put(signed value) noexcept;
    ascii_numput& put(signed long value) noexcept;
    ascii_numput& put(signed long long value) noexcept;
    ascii_numput& put(float value) noexcept;
    ascii_numput& put(double value) noexcept;
  };

inline ascii_numput& ascii_numput::put(bool value) noexcept
  {
    this->put_TB(value);
    return *this;
  }

inline ascii_numput& ascii_numput::put(const void* value) noexcept
  {
    this->put_XP(value);
    return *this;
  }

inline ascii_numput& ascii_numput::put(unsigned char value) noexcept
  {
    this->put_DU(value);
    return *this;
  }

inline ascii_numput& ascii_numput::put(unsigned short value) noexcept
  {
    this->put_DU(value);
    return *this;
  }

inline ascii_numput& ascii_numput::put(unsigned value) noexcept
  {
    this->put_DU(value);
    return *this;
  }

inline ascii_numput& ascii_numput::put(unsigned long value) noexcept
  {
    this->put_DU(value);
    return *this;
  }

inline ascii_numput& ascii_numput::put(unsigned long long value) noexcept
  {
    this->put_DU(value);
    return *this;
  }

inline ascii_numput& ascii_numput::put(signed char value) noexcept
  {
    this->put_DI(value);
    return *this;
  }

inline ascii_numput& ascii_numput::put(signed short value) noexcept
  {
    this->put_DI(value);
    return *this;
  }

inline ascii_numput& ascii_numput::put(signed value) noexcept
  {
    this->put_DI(value);
    return *this;
  }

inline ascii_numput& ascii_numput::put(signed long value) noexcept
  {
    this->put_DI(value);
    return *this;
  }

inline ascii_numput& ascii_numput::put(signed long long value) noexcept
  {
    this->put_DI(value);
    return *this;
  }

inline ascii_numput& ascii_numput::put(float value) noexcept
  {
    this->put_DF(static_cast<double>(value), true);
    return *this;
  }

inline ascii_numput& ascii_numput::put(double value) noexcept
  {
    this->put_DF(value);
    return *this;
  }

}  // namespace rocket

#endif
