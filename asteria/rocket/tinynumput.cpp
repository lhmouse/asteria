// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "tinynumput.hpp"
#include <cmath>

namespace rocket {

tinynumput& tinynumput::put_TB(bool value) noexcept
  {
    char* bp;
    char* ep;
    // Get the template string literal. The string is immutable.
    if(value) {
      bp = const_cast<char*>("true");
      ep = bp + 4;
    }
    else {
      bp = const_cast<char*>("false");
      ep = bp + 5;
    }
    // Set the string. The internal storage is unused.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_XP(const void* value) noexcept
  {
    char* bp = this->m_stor;
    char* ep = bp;
    // Write the hexadecimal prefix.
    *(ep++) = '0';
    *(ep++) = 'x';
    // Write all digits in normal order.
    // This is done by extract digits from the more significant side.
    uintptr_t reg = reinterpret_cast<uintptr_t>(value);
    constexpr int nbits = numeric_limits<uintptr_t>::digits;
    for(int i = 0; i != nbits / 4; ++i) {
      // Shift a digit out.
      size_t dval = reg >> (nbits - 4);
      reg = reg << 4;
      // Write this digit.
      *(ep++) = "0123456789ABCDEF"[dval];
    }
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

    namespace {

    template<uint8_t radixT, typename valueT> char* do_put_backward_U(char*& bp, const valueT& value,
                                                                      size_t width = 1)
      {
        char* stop = bp - width;
        // Write digits backwards.
        valueT reg = value;
        while(reg != 0) {
          // Shift a digit out.
          size_t dval = reg % radixT;
          reg = reg / radixT;
          // Write this digit.
          *(--bp) = "0123456789ABCDEF"[dval];
        }
        // Pad the string to at least the width requested.
        while(bp > stop) {
          // Fill a zero.
          *(--bp) = '0';
        }
        return bp;
      }

    }  // namespace

tinynumput& tinynumput::put_BU(uint32_t value) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Write digits backwards.
    do_put_backward_U<2>(bp, value);
    // Prepend the binary prefix.
    *(--bp) = 'b';
    *(--bp) = '0';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_BQ(uint64_t value) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Write digits backwards.
    do_put_backward_U<2>(bp, value);
    // Prepend the binary prefix.
    *(--bp) = 'b';
    *(--bp) = '0';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_DU(uint32_t value) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Write digits backwards.
    do_put_backward_U<10>(bp, value);
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_DQ(uint64_t value) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Write digits backwards.
    do_put_backward_U<10>(bp, value);
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_XU(uint32_t value) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Write digits backwards.
    do_put_backward_U<16>(bp, value);
    // Prepend the hexadecimal prefix.
    *(--bp) = 'x';
    *(--bp) = '0';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_XQ(uint64_t value) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Write digits backwards.
    do_put_backward_U<16>(bp, value);
    // Prepend the hexadecimal prefix.
    *(--bp) = 'x';
    *(--bp) = '0';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_BI(int32_t value) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint32_t sign = static_cast<uint32_t>(value >> 31);
    // Write digits backwards using its absolute value without causing overflows.
    do_put_backward_U<2>(bp, (static_cast<uint32_t>(value) ^ sign) - sign);
    // Prepend the binary prefix.
    *(--bp) = 'b';
    *(--bp) = '0';
    // If the number is negative, prepend a minus sign.
    if(sign)
      *(--bp) = '-';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_BL(int64_t value) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint64_t sign = static_cast<uint64_t>(value >> 63);
    // Write digits backwards using its absolute value without causing overflows.
    do_put_backward_U<2>(bp, (static_cast<uint64_t>(value) ^ sign) - sign);
    // Prepend the binary prefix.
    *(--bp) = 'b';
    *(--bp) = '0';
    // If the number is negative, prepend a minus sign.
    if(sign)
      *(--bp) = '-';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_DI(int32_t value) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint32_t sign = static_cast<uint32_t>(value >> 31);
    // Write digits backwards using its absolute value without causing overflows.
    do_put_backward_U<10>(bp, (static_cast<uint32_t>(value) ^ sign) - sign);
    // If the number is negative, prepend a minus sign.
    if(sign)
      *(--bp) = '-';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_DL(int64_t value) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint64_t sign = static_cast<uint64_t>(value >> 63);
    // Write digits backwards using its absolute value without causing overflows.
    do_put_backward_U<10>(bp, (static_cast<uint64_t>(value) ^ sign) - sign);
    // If the number is negative, prepend a minus sign.
    if(sign)
      *(--bp) = '-';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_XI(int32_t value) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint32_t sign = static_cast<uint32_t>(value >> 31);
    // Write digits backwards using its absolute value without causing overflows.
    do_put_backward_U<16>(bp, (static_cast<uint32_t>(value) ^ sign) - sign);
    // Prepend the hexadecimal prefix.
    *(--bp) = 'x';
    *(--bp) = '0';
    // If the number is negative, prepend a minus sign.
    if(sign)
      *(--bp) = '-';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_XL(int64_t value) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint64_t sign = static_cast<uint64_t>(value >> 63);
    // Write digits backwards using its absolute value without causing overflows.
    do_put_backward_U<16>(bp, (static_cast<uint64_t>(value) ^ sign) - sign);
    // Prepend the hexadecimal prefix.
    *(--bp) = 'x';
    *(--bp) = '0';
    // If the number is negative, prepend a minus sign.
    if(sign)
      *(--bp) = '-';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }
/*
tinynumput& tinynumput::put_BF(double value) noexcept
  {
  }

tinynumput& tinynumput::put_DF(double value) noexcept
  {
  }

tinynumput& tinynumput::put_XF(double value) noexcept
  {
  }

tinynumput& tinynumput::put_BE(double value) noexcept
  {
  }

tinynumput& tinynumput::put_DE(double value) noexcept
  {
  }

tinynumput& tinynumput::put_XE(double value) noexcept
  {
  }
*/
}  // namespace rocket
