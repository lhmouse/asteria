// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASCII_NUMPUT_
#define ROCKET_ASCII_NUMPUT_

#include "fwd.hpp"
#include "xassert.hpp"
namespace rocket {

class ascii_numput
  {
  private:
    // Configuration
    char m_rdxp = '.';

    // This storage must be sufficient for the longest result, which
    // at the moment is signed 64-bit integer in binary (`-0b111...1`
    // takes 68 bytes along with the null terminator).
    char m_stor[71];

    // These pointers may point to static, immutable storage.
    const char* m_data = "";
    uint32_t m_size = 0;

  public:
    // Initializes an empty string.
    ascii_numput()
      noexcept = default;

    ascii_numput(const ascii_numput&) = delete;
    ascii_numput& operator=(const ascii_numput&) = delete;

  public:
    // accessors
    const char*
    begin()
      const noexcept
      { return this->m_data;  }

    const char*
    end()
      const noexcept
      { return this->m_data + this->m_size;  }

    bool
    empty()
      const noexcept
      { return this->m_size == 0;  }

    const char*
    data()
      const noexcept
      { return this->m_data;  }

    const char*
    c_str()
      const noexcept
      { return this->m_data;  }

    size_t
    size()
      const noexcept
      { return this->m_size;  }

    size_t
    length()
      const noexcept
      { return this->m_size;  }

    char
    operator[](size_t k)
      const noexcept
      {
        ROCKET_ASSERT(k <= this->m_size);
        return this->m_data[k];
      }

    void
    clear()
      noexcept
      {
        this->m_stor[0] = 0;
        this->m_data = this->m_stor;
        this->m_size = 0;
      }

    // Gets and sets the radix point.
    char
    radix_point()
      const noexcept
      { return this->m_rdxp;  }

    void
    set_radix_point(char rdxp)
      noexcept
      { this->m_rdxp = rdxp;  }

    // * boolean as `true` or `false`
    void
    put_TB(bool value)
      noexcept;

    // * pointer as an unsigned integer in hexadecimal
    void
    put_XP(const volatile void* value)
      noexcept;

    // * unsigned 64-bit integer in binary
    void
    put_BU(uint64_t value, uint32_t precision = 1)
      noexcept;

    // * unsigned 64-bit integer in hexadecimal
    void
    put_XU(uint64_t value, uint32_t precision = 1)
      noexcept;

    // * unsigned 64-bit integer in decimal
    void
    put_DU(uint64_t value, uint32_t precision = 1)
      noexcept;

    // * signed 64-bit integer in binary
    void
    put_BI(int64_t value, uint32_t precision = 1)
      noexcept;

    // * signed 64-bit integer in hexadecimal
    void
    put_XI(int64_t value, uint32_t precision = 1)
      noexcept;

    // * signed 64-bit integer in decimal
    void
    put_DI(int64_t value, uint32_t precision = 1)
      noexcept;

    // * IEEE-754 single-precision floating-point in binary
    void
    put_BF(float value)
      noexcept;

    // * IEEE-754 single-precision floating-point in binary scientific notation
    void
    put_BEF(float value)
      noexcept;

    // * IEEE-754 single-precision floating-point in hexadecimal
    void
    put_XF(float value)
      noexcept;

    // * IEEE-754 single-precision floating-point in hexadecimal scientific notation
    void
    put_XEF(float value)
      noexcept;

    // * IEEE-754 single-precision floating-point in decimal
    void
    put_DF(float value)
      noexcept;

    // * IEEE-754 single-precision floating-point in decimal scientific notation
    void
    put_DEF(float value)
      noexcept;

    // * IEEE-754 single-precision floating-point in binary
    void
    put_BD(double value)
      noexcept;

    // * IEEE-754 double-precision floating-point in binary scientific notation
    void
    put_BED(double value)
      noexcept;

    // * IEEE-754 double-precision floating-point in hexadecimal
    void
    put_XD(double value)
      noexcept;

    // * IEEE-754 double-precision floating-point in hexadecimal scientific notation
    void
    put_XED(double value)
      noexcept;

    // * IEEE-754 double-precision floating-point in decimal
    void
    put_DD(double value)
      noexcept;

    // * IEEE-754 double-precision floating-point in decimal scientific notation
    void
    put_DED(double value)
      noexcept;

    // These are easy functions that delegate to those above, passing
    // their default arguments. These functions are designed to produce
    // lossless outputs.
    void
    put(bool value)
      noexcept
      {
        this->put_TB(value);
      }

    void
    put(const volatile void* value)
      noexcept
      {
        this->put_XP(value);
      }

    void
    put(unsigned char value)
      noexcept
      {
        this->put_DU(value);
      }

    void
    put(signed char value)
      noexcept
      {
        this->put_DI(value);
      }

    void
    put(unsigned short value)
      noexcept
      {
        this->put_DU(value);
      }

    void
    put(short value)
      noexcept
      {
        this->put_DI(value);
      }

    void
    put(unsigned int value)
      noexcept
      {
        this->put_DU(value);
      }

    void
    put(int value)
      noexcept
      {
        this->put_DI(value);
      }

    void
    put(unsigned long value)
      noexcept
      {
        this->put_DU(value);
      }

    void
    put(long value)
      noexcept
      {
        this->put_DI(value);
      }

    void
    put(unsigned long long value)
      noexcept
      {
        this->put_DU(value);
      }

    void
    put(long long value)
      noexcept
      {
        this->put_DI(value);
      }

    void
    put(float value)
      noexcept
      {
        this->put_DF(value);
      }

    void
    put(double value)
      noexcept
      {
        this->put_DD(value);
      }
  };

}  // namespace rocket
#endif
