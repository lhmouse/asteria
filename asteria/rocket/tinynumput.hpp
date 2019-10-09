// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYNUMPUT_HPP_
#define ROCKET_TINYNUMPUT_HPP_

#include "utilities.hpp"

namespace rocket {

class tiny_numput
  {
  private:
    char m_str[62];
    unsigned char m_off = 0;
    unsigned char m_end = 0;

  public:
    tiny_numput() noexcept
      {
      }

  public:
    // observers
    bool empty() const noexcept
      {
        return this->m_off == this->m_end;
      }
    size_t size() const noexcept
      {
        return this->m_end - this->m_off;
      }
    const char* begin() const noexcept
      {
        return this->m_str + this->m_off;
      }
    const char* end() const noexcept
      {
        return this->m_str + this->m_end;
      }
    const char* data() const noexcept
      {
        return this->m_str + this->m_off;
      }

    // modifiers
    tiny_numput& clear() noexcept
      {
        return (this->m_off = this->m_end), *this;
      }
    tiny_numput& put(bool value) noexcept
      {
        return this->put_BUT(value);
      }
    tiny_numput& put(const void* value) noexcept
      {
        return this->put_PPX(value);
      }
    tiny_numput& put(signed char value) noexcept
      {
        return this->put_SID(value);
      }
    tiny_numput& put(unsigned char value) noexcept
      {
        return this->put_SUD(value);
      }
    tiny_numput& put(signed short value) noexcept
      {
        return this->put_SID(value);
      }
    tiny_numput& put(unsigned short value) noexcept
      {
        return this->put_SUD(value);
      }
    tiny_numput& put(signed value) noexcept
      {
        return this->put_SID(value);
      }
    tiny_numput& put(unsigned value) noexcept
      {
        return this->put_SUD(value);
      }
    tiny_numput& put(signed long value) noexcept
      {
        return this->put_DID(value);
      }
    tiny_numput& put(unsigned long value) noexcept
      {
        return this->put_DUD(value);
      }
    tiny_numput& put(signed long long value) noexcept
      {
        return this->put_DID(value);
      }
    tiny_numput& put(unsigned long long value) noexcept
      {
        return this->put_DUD(value);
      }
    tiny_numput& put(double value) noexcept
      {
        return this->put_DFD(value);
      }

    // explicit formatters
    //   B  : bit
    //   P  : pointer
    //   S  : 32-bit (single)
    //   D  : 64-bit (double)
    //    P : pointer
    //    I : signed integer
    //    U : unsigned integer
    //    F : floating-point
    //     T: "true" / "false"
    //     B: binary, default
    //     D: decimal, default
    //     X: hexadecimal, default
    //     J: binary, scientific
    //     E: decimal, scientific
    //     A: hexadecimal, scientific
    tiny_numput& put_BUT(bool value) noexcept;
    tiny_numput& put_PPX(const void* value) noexcept;
    tiny_numput& put_SIB(int32_t value) noexcept;
    tiny_numput& put_SID(int32_t value) noexcept;
    tiny_numput& put_SIX(int32_t value) noexcept;
    tiny_numput& put_SUB(uint32_t value) noexcept;
    tiny_numput& put_SUD(uint32_t value) noexcept;
    tiny_numput& put_SUX(uint32_t value) noexcept;
    tiny_numput& put_DIB(int64_t value) noexcept;
    tiny_numput& put_DID(int64_t value) noexcept;
    tiny_numput& put_DIX(int64_t value) noexcept;
    tiny_numput& put_DUB(uint64_t value) noexcept;
    tiny_numput& put_DUD(uint64_t value) noexcept;
    tiny_numput& put_DUX(uint64_t value) noexcept;
    tiny_numput& put_DFB(double value) noexcept;
    tiny_numput& put_DFD(double value) noexcept;
    tiny_numput& put_DFX(double value) noexcept;
    tiny_numput& put_DFJ(double value) noexcept;
    tiny_numput& put_DFE(double value) noexcept;
    tiny_numput& put_DFA(double value) noexcept;
  };

}  // namespace rocket

#endif
