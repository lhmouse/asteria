// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYNUMPUT_HPP_
#define ROCKET_TINYNUMPUT_HPP_

#include "utilities.hpp"

namespace rocket {

class tinynumput
  {
  private:
    char m_str[62];
    unsigned char m_off = 0;
    unsigned char m_end = 0;

  public:
    tinynumput() noexcept
      {
      }
    template<typename xvalueT, ROCKET_ENABLE_IF(is_scalar<xvalueT>::value)> explicit tinynumput(const xvalueT& xvalue) noexcept
      {
        this->put(xvalue);
      }

  public:
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
    tinynumput& put_BUT(bool value) noexcept;
    tinynumput& put_PPX(const void* value) noexcept;
    tinynumput& put_SIB(int32_t value) noexcept;
    tinynumput& put_SID(int32_t value) noexcept;
    tinynumput& put_SIX(int32_t value) noexcept;
    tinynumput& put_SUB(uint32_t value) noexcept;
    tinynumput& put_SUD(uint32_t value) noexcept;
    tinynumput& put_SUX(uint32_t value) noexcept;
    tinynumput& put_DIB(int64_t value) noexcept;
    tinynumput& put_DID(int64_t value) noexcept;
    tinynumput& put_DIX(int64_t value) noexcept;
    tinynumput& put_DUB(uint64_t value) noexcept;
    tinynumput& put_DUD(uint64_t value) noexcept;
    tinynumput& put_DUX(uint64_t value) noexcept;
    tinynumput& put_DFB(double value) noexcept;
    tinynumput& put_DFD(double value) noexcept;
    tinynumput& put_DFX(double value) noexcept;
    tinynumput& put_DFJ(double value) noexcept;
    tinynumput& put_DFE(double value) noexcept;
    tinynumput& put_DFA(double value) noexcept;

    // observers
    bool empty() const noexcept
      {
        return this->m_off == this->m_end;
      }
    size_t size() const noexcept
      {
        return static_cast<size_t>(this->m_end) - this->m_off;
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
    tinynumput& clear() noexcept
      {
        return (this->m_off = this->m_end), *this;
      }
    tinynumput& put(bool value) noexcept
      {
        return this->put_BUT(value);
      }
    tinynumput& put(const void* value) noexcept
      {
        return this->put_PPX(value);
      }
    tinynumput& put(signed char value) noexcept
      {
        return this->put_SID(value);
      }
    tinynumput& put(unsigned char value) noexcept
      {
        return this->put_SUD(value);
      }
    tinynumput& put(signed short value) noexcept
      {
        return this->put_SID(value);
      }
    tinynumput& put(unsigned short value) noexcept
      {
        return this->put_SUD(value);
      }
    tinynumput& put(signed value) noexcept
      {
        return this->put_SID(value);
      }
    tinynumput& put(unsigned value) noexcept
      {
        return this->put_SUD(value);
      }
    tinynumput& put(signed long value) noexcept
      {
        return this->put_DID(value);
      }
    tinynumput& put(unsigned long value) noexcept
      {
        return this->put_DUD(value);
      }
    tinynumput& put(signed long long value) noexcept
      {
        return this->put_DID(value);
      }
    tinynumput& put(unsigned long long value) noexcept
      {
        return this->put_DUD(value);
      }
    tinynumput& put(float value) noexcept
      {
        return this->put_DFD(value);
      }
    tinynumput& put(double value) noexcept
      {
        return this->put_DFD(value);
      }
  };

}  // namespace rocket

#endif
