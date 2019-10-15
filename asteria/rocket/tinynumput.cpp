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
      reg <<= 4;
      // Write this digit.
      *(ep++) = "0123456789ABCDEF"[dval];
    }
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

    namespace {

    template<typename valueT>
        constexpr typename make_unsigned<valueT>::type do_cast_U(const valueT& value) noexcept
      {
        return static_cast<typename make_unsigned<valueT>::type>(value);
      }

    template<uint8_t radixT, typename valueT, ROCKET_ENABLE_IF(is_unsigned<valueT>::value)>
        void do_xput_U_bkwd(char*& bp, const valueT& value, size_t precision = 1)
      {
        char* stop = bp - precision;
        // Write digits backwards.
        valueT reg = value;
        while(reg != 0) {
          // Shift a digit out.
          size_t dval = reg % radixT;
          reg /= radixT;
          // Write this digit.
          *--bp = "0123456789ABCDEF"[dval];
        }
        // Pad the string to at least the precision requested.
        while(bp > stop)
          *--bp = '0';
      }

    }  // namespace

tinynumput& tinynumput::put_BU(uint32_t value, size_t precision) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Write digits backwards.
    do_xput_U_bkwd<2>(bp, value, precision);
    // Prepend the binary prefix.
    *--bp = 'b';
    *--bp = '0';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_BQ(uint64_t value, size_t precision) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Write digits backwards.
    do_xput_U_bkwd<2>(bp, value, precision);
    // Prepend the binary prefix.
    *--bp = 'b';
    *--bp = '0';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_DU(uint32_t value, size_t precision) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Write digits backwards.
    do_xput_U_bkwd<10>(bp, value, precision);
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_DQ(uint64_t value, size_t precision) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Write digits backwards.
    do_xput_U_bkwd<10>(bp, value, precision);
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_XU(uint32_t value, size_t precision) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Write digits backwards.
    do_xput_U_bkwd<16>(bp, value, precision);
    // Prepend the hexadecimal prefix.
    *--bp = 'x';
    *--bp = '0';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_XQ(uint64_t value, size_t precision) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Write digits backwards.
    do_xput_U_bkwd<16>(bp, value, precision);
    // Prepend the hexadecimal prefix.
    *--bp = 'x';
    *--bp = '0';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_BI(int32_t value, size_t precision) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint32_t sign = do_cast_U(value >> 31);
    // Write digits backwards using its absolute value without causing overflows.
    do_xput_U_bkwd<2>(bp, (do_cast_U(value) ^ sign) - sign, precision);
    // Prepend the binary prefix.
    *--bp = 'b';
    *--bp = '0';
    // If the number is negative, prepend a minus sign.
    if(sign)
      *--bp = '-';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_BL(int64_t value, size_t precision) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint64_t sign = do_cast_U(value >> 63);
    // Write digits backwards using its absolute value without causing overflows.
    do_xput_U_bkwd<2>(bp, (do_cast_U(value) ^ sign) - sign, precision);
    // Prepend the binary prefix.
    *--bp = 'b';
    *--bp = '0';
    // If the number is negative, prepend a minus sign.
    if(sign)
      *--bp = '-';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_DI(int32_t value, size_t precision) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint32_t sign = do_cast_U(value >> 31);
    // Write digits backwards using its absolute value without causing overflows.
    do_xput_U_bkwd<10>(bp, (do_cast_U(value) ^ sign) - sign, precision);
    // If the number is negative, prepend a minus sign.
    if(sign)
      *--bp = '-';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_DL(int64_t value, size_t precision) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint64_t sign = do_cast_U(value >> 63);
    // Write digits backwards using its absolute value without causing overflows.
    do_xput_U_bkwd<10>(bp, (do_cast_U(value) ^ sign) - sign, precision);
    // If the number is negative, prepend a minus sign.
    if(sign)
      *--bp = '-';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_XI(int32_t value, size_t precision) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint32_t sign = do_cast_U(value >> 31);
    // Write digits backwards using its absolute value without causing overflows.
    do_xput_U_bkwd<16>(bp, (do_cast_U(value) ^ sign) - sign, precision);
    // Prepend the hexadecimal prefix.
    *--bp = 'x';
    *--bp = '0';
    // If the number is negative, prepend a minus sign.
    if(sign)
      *--bp = '-';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_XL(int64_t value, size_t precision) noexcept
  {
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint64_t sign = do_cast_U(value >> 63);
    // Write digits backwards using its absolute value without causing overflows.
    do_xput_U_bkwd<16>(bp, (do_cast_U(value) ^ sign) - sign, precision);
    // Prepend the hexadecimal prefix.
    *--bp = 'x';
    *--bp = '0';
    // If the number is negative, prepend a minus sign.
    if(sign)
      *--bp = '-';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

    namespace {

    template<typename valueT>
        char* do_check_special_opt(char*& bp, char*& ep, const valueT& value)
      {
        // Note that this function returns a pointer to immutable strings.
        int cls = ::std::fpclassify(value);
        if(cls == FP_INFINITE) {
          bp = const_cast<char*>("-infinity");
          ep = bp + 9;
          return bp;
        }
        if(cls == FP_NAN) {
          bp = const_cast<char*>("-nan");
          ep = bp + 4;
          return bp;
        }
        if(cls == FP_ZERO) {
          bp = const_cast<char*>("-0");
          ep = bp + 2;
          return bp;
        }
        // Return a null pointer to indicate that the value is finite.
        return nullptr;
      }

    void do_xfrexp_F_bin(uint64_t& mant, int& exp, const double& value) noexcept
      {
        // Note if `value` is not finite then the behavior is undefined.
        int bexp;
        double frac = ::std::frexp(::std::fabs(value), &bexp);
        exp = bexp - 1;
        mant = static_cast<uint64_t>(static_cast<int64_t>(::std::ldexp(frac, 53))) << 11;
      }

    void do_xput_M_bin(char*& ep, const uint64_t& mant, const char* dp_opt)
      {
        // Write digits in normal order.
        uint64_t reg = mant;
        while(reg != 0) {
          // Shift a digit out.
          size_t dval = static_cast<size_t>(reg >> 63);
          reg <<= 1;
          // Insert a decimal point before `dp_opt`.
          if(ep == dp_opt)
            *(ep++) = '.';
          // Write this digit.
          *(ep++) = static_cast<char>('0' + dval);
        }
        // If `dp_opt` is set, fill zeroes until it is reached,
        // if no decimal point has been added so far.
        if(dp_opt)
          while(ep < dp_opt)
            *(ep++) = '0';
      }

    void do_xput_M_hex(char*& ep, const uint64_t& mant, const char* dp_opt)
      {
        // Write digits in normal order.
        uint64_t reg = mant;
        while(reg != 0) {
          // Shift a digit out.
          size_t dval = static_cast<size_t>(reg >> 60);
          reg <<= 4;
          // Insert a decimal point before `dp_opt`.
          if(ep == dp_opt)
            *(ep++) = '.';
          // Write this digit.
          *(ep++) = "0123456789ABCDEF"[dval];
        }
        // If `dp_opt` is set, fill zeroes until it is reached,
        // if no decimal point has been added so far.
        if(dp_opt)
          while(ep < dp_opt)
            *(ep++) = '0';
      }

    char* do_xput_I_exp(char*& ep, const int& exp)
      {
        // Append the sign symbol, always.
        if(exp < 0)
          *(ep++) = '-';
        else
          *(ep++) = '+';
        // Write the exponent in this temporary storage backwards.
        // The exponent shall contain at least two digits.
        char temps[8];
        char* tbp = ::std::end(temps);
        do_xput_U_bkwd<10>(tbp, do_cast_U(::std::abs(exp)), 2);
        // Append the exponent.
        noadl::ranged_for(tbp, ::std::end(temps), [&](const char* p) { *(ep++) = *p;  });
        return ep;
      }

    }  // namespace

tinynumput& tinynumput::put_BF(double value) noexcept
  {
    char* bp;
    char* ep;
    // Extract the sign bit and extend it to a word.
    int sign = ::std::signbit(value) ? -1 : 0;
    // Treat non-finite values and zeroes specially.
    if(do_check_special_opt(bp, ep, value)) {
      // Use the template string literal, which is immutable.
      // Skip the minus sign if the sign bit is clear.
      bp += do_cast_U(sign + 1);
    }
    else {
      // Seek to the beginning of the internal buffer.
      bp = this->m_stor;
      ep = bp;
      // Prepend a minus sign if the number is negative.
      if(sign)
        *(ep++) = '-';
      // Prepend the binary prefix.
      *(ep++) = '0';
      *(ep++) = 'b';
      // Break the number down into fractional and exponential parts. This result is exact.
      uint64_t mant;
      int exp;
      do_xfrexp_F_bin(mant, exp, value);
      // Write the broken-down number...
      if((exp < -4) || (53 <= exp)) {
        // ... in scientific notation.
        do_xput_M_bin(ep, mant, ep + 1);
        *(ep++) = 'p';
        do_xput_I_exp(ep, exp);
      }
      else if(exp < 0) {
        // ... in plain format; the number starts with "0."; zeroes are prepended as necessary.
        *(ep++) = '0';
        *(ep++) = '.';
        noadl::ranged_for(exp, -1, [&](int) { *(ep++) = '0';  });
        do_xput_M_bin(ep, mant, nullptr);
      }
      else
        // ... in plain format; the decimal point is inserted in the middle.
        do_xput_M_bin(ep, mant, ep + 1 + do_cast_U(exp));
    }
    // Set the string. The internal storage is used for finite values only.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_BE(double value) noexcept
  {
    char* bp;
    char* ep;
    // Extract the sign bit and extend it to a word.
    int sign = ::std::signbit(value) ? -1 : 0;
    // Treat non-finite values and zeroes specially.
    if(do_check_special_opt(bp, ep, value)) {
      // Use the template string literal, which is immutable.
      // Skip the minus sign if the sign bit is clear.
      bp += do_cast_U(sign + 1);
    }
    else {
      // Seek to the beginning of the internal buffer.
      bp = this->m_stor;
      ep = bp;
      // Prepend a minus sign if the number is negative.
      if(sign)
        *(ep++) = '-';
      // Prepend the binary prefix.
      *(ep++) = '0';
      *(ep++) = 'b';
      // Break the number down into fractional and exponential parts. This result is exact.
      uint64_t mant;
      int exp;
      do_xfrexp_F_bin(mant, exp, value);
      // Write the broken-down number in scientific notation.
      do_xput_M_bin(ep, mant, ep + 1);
      *(ep++) = 'p';
      do_xput_I_exp(ep, exp);
    }
    // Set the string. The internal storage is used for finite values only.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_XF(double value) noexcept
  {
    char* bp;
    char* ep;
    // Extract the sign bit and extend it to a word.
    int sign = ::std::signbit(value) ? -1 : 0;
    // Treat non-finite values and zeroes specially.
    if(do_check_special_opt(bp, ep, value)) {
      // Use the template string literal, which is immutable.
      // Skip the minus sign if the sign bit is clear.
      bp += do_cast_U(sign + 1);
    }
    else {
      // Seek to the beginning of the internal buffer.
      bp = this->m_stor;
      ep = bp;
      // Prepend a minus sign if the number is negative.
      if(sign)
        *(ep++) = '-';
      // Prepend the hexadecimal prefix.
      *(ep++) = '0';
      *(ep++) = 'x';
      // Break the number down into fractional and exponential parts. This result is exact.
      uint64_t mant;
      int exp;
      do_xfrexp_F_bin(mant, exp, value);
      // Normalize the integral part so it is the maximum value in the range [0,16).
      mant >>= ~exp & 3;
      exp >>= 2;
      // Write the broken-down number...
      if((exp < -4) || (14 <= exp)) {
        // ... in scientific notation.
        do_xput_M_hex(ep, mant, ep + 1);
        *(ep++) = 'p';
        do_xput_I_exp(ep, exp * 4);
      }
      else if(exp < 0) {
        // ... in plain format; the number starts with "0."; zeroes are prepended as necessary.
        *(ep++) = '0';
        *(ep++) = '.';
        noadl::ranged_for(exp, -1, [&](int) { *(ep++) = '0';  });
        do_xput_M_hex(ep, mant, nullptr);
      }
      else
        // ... in plain format; the decimal point is inserted in the middle.
        do_xput_M_hex(ep, mant, ep + 1 + do_cast_U(exp));
    }
    // Set the string. The internal storage is used for finite values only.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_XE(double value) noexcept
  {
    char* bp;
    char* ep;
    // Extract the sign bit and extend it to a word.
    int sign = ::std::signbit(value) ? -1 : 0;
    // Treat non-finite values and zeroes specially.
    if(do_check_special_opt(bp, ep, value)) {
      // Use the template string literal, which is immutable.
      // Skip the minus sign if the sign bit is clear.
      bp += do_cast_U(sign + 1);
    }
    else {
      // Seek to the beginning of the internal buffer.
      bp = this->m_stor;
      ep = bp;
      // Prepend a minus sign if the number is negative.
      if(sign)
        *(ep++) = '-';
      // Prepend the hexadecimal prefix.
      *(ep++) = '0';
      *(ep++) = 'x';
      // Break the number down into fractional and exponential parts. This result is exact.
      uint64_t mant;
      int exp;
      do_xfrexp_F_bin(mant, exp, value);
      // Normalize the integral part so it is the maximum value in the range [0,16).
      mant >>= ~exp & 3;
      exp >>= 2;
      // Write the broken-down number in scientific notation.
      do_xput_M_hex(ep, mant, ep + 1);
      *(ep++) = 'p';
      do_xput_I_exp(ep, exp * 4);
    }
    // Set the string. The internal storage is used for finite values only.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

    namespace {

    /* You may generate this table using the following program:
     *
     * #include <stdio.h>
     *
     * int main(void)
     *   {
     *     int e, m, len;
     *
     *     // initialize the first line.
     *     putchar('\t');
     *     len = 0;
     *
     *     for(e = -324; e <= +308; ++e) {
     *       for(m = 1; m <= 9; ++m) {
     *         if(e == -324 && m < 3)
     *           continue;  // underflows to zero
     *         if(e == +308 && m > 1)
     *           break;  // overflows to infinity
     *
     *         len += printf("%de%+d,", m, e);
     *         if(len < 90)
     *           continue;
     *
     *         // break overlong lines
     *         putchar('\n');
     *         putchar('\t');
     *         len = 0;
     *       }
     *     }
     *   }
     */
    constexpr double s_decbounds_F[] =
      {
	3e-324,4e-324,5e-324,6e-324,7e-324,8e-324,9e-324,1e-323,2e-323,3e-323,4e-323,5e-323,6e-323,
	7e-323,8e-323,9e-323,1e-322,2e-322,3e-322,4e-322,5e-322,6e-322,7e-322,8e-322,9e-322,1e-321,
	2e-321,3e-321,4e-321,5e-321,6e-321,7e-321,8e-321,9e-321,1e-320,2e-320,3e-320,4e-320,5e-320,
	6e-320,7e-320,8e-320,9e-320,1e-319,2e-319,3e-319,4e-319,5e-319,6e-319,7e-319,8e-319,9e-319,
	1e-318,2e-318,3e-318,4e-318,5e-318,6e-318,7e-318,8e-318,9e-318,1e-317,2e-317,3e-317,4e-317,
	5e-317,6e-317,7e-317,8e-317,9e-317,1e-316,2e-316,3e-316,4e-316,5e-316,6e-316,7e-316,8e-316,
	9e-316,1e-315,2e-315,3e-315,4e-315,5e-315,6e-315,7e-315,8e-315,9e-315,1e-314,2e-314,3e-314,
	4e-314,5e-314,6e-314,7e-314,8e-314,9e-314,1e-313,2e-313,3e-313,4e-313,5e-313,6e-313,7e-313,
	8e-313,9e-313,1e-312,2e-312,3e-312,4e-312,5e-312,6e-312,7e-312,8e-312,9e-312,1e-311,2e-311,
	3e-311,4e-311,5e-311,6e-311,7e-311,8e-311,9e-311,1e-310,2e-310,3e-310,4e-310,5e-310,6e-310,
	7e-310,8e-310,9e-310,1e-309,2e-309,3e-309,4e-309,5e-309,6e-309,7e-309,8e-309,9e-309,1e-308,
	2e-308,3e-308,4e-308,5e-308,6e-308,7e-308,8e-308,9e-308,1e-307,2e-307,3e-307,4e-307,5e-307,
	6e-307,7e-307,8e-307,9e-307,1e-306,2e-306,3e-306,4e-306,5e-306,6e-306,7e-306,8e-306,9e-306,
	1e-305,2e-305,3e-305,4e-305,5e-305,6e-305,7e-305,8e-305,9e-305,1e-304,2e-304,3e-304,4e-304,
	5e-304,6e-304,7e-304,8e-304,9e-304,1e-303,2e-303,3e-303,4e-303,5e-303,6e-303,7e-303,8e-303,
	9e-303,1e-302,2e-302,3e-302,4e-302,5e-302,6e-302,7e-302,8e-302,9e-302,1e-301,2e-301,3e-301,
	4e-301,5e-301,6e-301,7e-301,8e-301,9e-301,1e-300,2e-300,3e-300,4e-300,5e-300,6e-300,7e-300,
	8e-300,9e-300,1e-299,2e-299,3e-299,4e-299,5e-299,6e-299,7e-299,8e-299,9e-299,1e-298,2e-298,
	3e-298,4e-298,5e-298,6e-298,7e-298,8e-298,9e-298,1e-297,2e-297,3e-297,4e-297,5e-297,6e-297,
	7e-297,8e-297,9e-297,1e-296,2e-296,3e-296,4e-296,5e-296,6e-296,7e-296,8e-296,9e-296,1e-295,
	2e-295,3e-295,4e-295,5e-295,6e-295,7e-295,8e-295,9e-295,1e-294,2e-294,3e-294,4e-294,5e-294,
	6e-294,7e-294,8e-294,9e-294,1e-293,2e-293,3e-293,4e-293,5e-293,6e-293,7e-293,8e-293,9e-293,
	1e-292,2e-292,3e-292,4e-292,5e-292,6e-292,7e-292,8e-292,9e-292,1e-291,2e-291,3e-291,4e-291,
	5e-291,6e-291,7e-291,8e-291,9e-291,1e-290,2e-290,3e-290,4e-290,5e-290,6e-290,7e-290,8e-290,
	9e-290,1e-289,2e-289,3e-289,4e-289,5e-289,6e-289,7e-289,8e-289,9e-289,1e-288,2e-288,3e-288,
	4e-288,5e-288,6e-288,7e-288,8e-288,9e-288,1e-287,2e-287,3e-287,4e-287,5e-287,6e-287,7e-287,
	8e-287,9e-287,1e-286,2e-286,3e-286,4e-286,5e-286,6e-286,7e-286,8e-286,9e-286,1e-285,2e-285,
	3e-285,4e-285,5e-285,6e-285,7e-285,8e-285,9e-285,1e-284,2e-284,3e-284,4e-284,5e-284,6e-284,
	7e-284,8e-284,9e-284,1e-283,2e-283,3e-283,4e-283,5e-283,6e-283,7e-283,8e-283,9e-283,1e-282,
	2e-282,3e-282,4e-282,5e-282,6e-282,7e-282,8e-282,9e-282,1e-281,2e-281,3e-281,4e-281,5e-281,
	6e-281,7e-281,8e-281,9e-281,1e-280,2e-280,3e-280,4e-280,5e-280,6e-280,7e-280,8e-280,9e-280,
	1e-279,2e-279,3e-279,4e-279,5e-279,6e-279,7e-279,8e-279,9e-279,1e-278,2e-278,3e-278,4e-278,
	5e-278,6e-278,7e-278,8e-278,9e-278,1e-277,2e-277,3e-277,4e-277,5e-277,6e-277,7e-277,8e-277,
	9e-277,1e-276,2e-276,3e-276,4e-276,5e-276,6e-276,7e-276,8e-276,9e-276,1e-275,2e-275,3e-275,
	4e-275,5e-275,6e-275,7e-275,8e-275,9e-275,1e-274,2e-274,3e-274,4e-274,5e-274,6e-274,7e-274,
	8e-274,9e-274,1e-273,2e-273,3e-273,4e-273,5e-273,6e-273,7e-273,8e-273,9e-273,1e-272,2e-272,
	3e-272,4e-272,5e-272,6e-272,7e-272,8e-272,9e-272,1e-271,2e-271,3e-271,4e-271,5e-271,6e-271,
	7e-271,8e-271,9e-271,1e-270,2e-270,3e-270,4e-270,5e-270,6e-270,7e-270,8e-270,9e-270,1e-269,
	2e-269,3e-269,4e-269,5e-269,6e-269,7e-269,8e-269,9e-269,1e-268,2e-268,3e-268,4e-268,5e-268,
	6e-268,7e-268,8e-268,9e-268,1e-267,2e-267,3e-267,4e-267,5e-267,6e-267,7e-267,8e-267,9e-267,
	1e-266,2e-266,3e-266,4e-266,5e-266,6e-266,7e-266,8e-266,9e-266,1e-265,2e-265,3e-265,4e-265,
	5e-265,6e-265,7e-265,8e-265,9e-265,1e-264,2e-264,3e-264,4e-264,5e-264,6e-264,7e-264,8e-264,
	9e-264,1e-263,2e-263,3e-263,4e-263,5e-263,6e-263,7e-263,8e-263,9e-263,1e-262,2e-262,3e-262,
	4e-262,5e-262,6e-262,7e-262,8e-262,9e-262,1e-261,2e-261,3e-261,4e-261,5e-261,6e-261,7e-261,
	8e-261,9e-261,1e-260,2e-260,3e-260,4e-260,5e-260,6e-260,7e-260,8e-260,9e-260,1e-259,2e-259,
	3e-259,4e-259,5e-259,6e-259,7e-259,8e-259,9e-259,1e-258,2e-258,3e-258,4e-258,5e-258,6e-258,
	7e-258,8e-258,9e-258,1e-257,2e-257,3e-257,4e-257,5e-257,6e-257,7e-257,8e-257,9e-257,1e-256,
	2e-256,3e-256,4e-256,5e-256,6e-256,7e-256,8e-256,9e-256,1e-255,2e-255,3e-255,4e-255,5e-255,
	6e-255,7e-255,8e-255,9e-255,1e-254,2e-254,3e-254,4e-254,5e-254,6e-254,7e-254,8e-254,9e-254,
	1e-253,2e-253,3e-253,4e-253,5e-253,6e-253,7e-253,8e-253,9e-253,1e-252,2e-252,3e-252,4e-252,
	5e-252,6e-252,7e-252,8e-252,9e-252,1e-251,2e-251,3e-251,4e-251,5e-251,6e-251,7e-251,8e-251,
	9e-251,1e-250,2e-250,3e-250,4e-250,5e-250,6e-250,7e-250,8e-250,9e-250,1e-249,2e-249,3e-249,
	4e-249,5e-249,6e-249,7e-249,8e-249,9e-249,1e-248,2e-248,3e-248,4e-248,5e-248,6e-248,7e-248,
	8e-248,9e-248,1e-247,2e-247,3e-247,4e-247,5e-247,6e-247,7e-247,8e-247,9e-247,1e-246,2e-246,
	3e-246,4e-246,5e-246,6e-246,7e-246,8e-246,9e-246,1e-245,2e-245,3e-245,4e-245,5e-245,6e-245,
	7e-245,8e-245,9e-245,1e-244,2e-244,3e-244,4e-244,5e-244,6e-244,7e-244,8e-244,9e-244,1e-243,
	2e-243,3e-243,4e-243,5e-243,6e-243,7e-243,8e-243,9e-243,1e-242,2e-242,3e-242,4e-242,5e-242,
	6e-242,7e-242,8e-242,9e-242,1e-241,2e-241,3e-241,4e-241,5e-241,6e-241,7e-241,8e-241,9e-241,
	1e-240,2e-240,3e-240,4e-240,5e-240,6e-240,7e-240,8e-240,9e-240,1e-239,2e-239,3e-239,4e-239,
	5e-239,6e-239,7e-239,8e-239,9e-239,1e-238,2e-238,3e-238,4e-238,5e-238,6e-238,7e-238,8e-238,
	9e-238,1e-237,2e-237,3e-237,4e-237,5e-237,6e-237,7e-237,8e-237,9e-237,1e-236,2e-236,3e-236,
	4e-236,5e-236,6e-236,7e-236,8e-236,9e-236,1e-235,2e-235,3e-235,4e-235,5e-235,6e-235,7e-235,
	8e-235,9e-235,1e-234,2e-234,3e-234,4e-234,5e-234,6e-234,7e-234,8e-234,9e-234,1e-233,2e-233,
	3e-233,4e-233,5e-233,6e-233,7e-233,8e-233,9e-233,1e-232,2e-232,3e-232,4e-232,5e-232,6e-232,
	7e-232,8e-232,9e-232,1e-231,2e-231,3e-231,4e-231,5e-231,6e-231,7e-231,8e-231,9e-231,1e-230,
	2e-230,3e-230,4e-230,5e-230,6e-230,7e-230,8e-230,9e-230,1e-229,2e-229,3e-229,4e-229,5e-229,
	6e-229,7e-229,8e-229,9e-229,1e-228,2e-228,3e-228,4e-228,5e-228,6e-228,7e-228,8e-228,9e-228,
	1e-227,2e-227,3e-227,4e-227,5e-227,6e-227,7e-227,8e-227,9e-227,1e-226,2e-226,3e-226,4e-226,
	5e-226,6e-226,7e-226,8e-226,9e-226,1e-225,2e-225,3e-225,4e-225,5e-225,6e-225,7e-225,8e-225,
	9e-225,1e-224,2e-224,3e-224,4e-224,5e-224,6e-224,7e-224,8e-224,9e-224,1e-223,2e-223,3e-223,
	4e-223,5e-223,6e-223,7e-223,8e-223,9e-223,1e-222,2e-222,3e-222,4e-222,5e-222,6e-222,7e-222,
	8e-222,9e-222,1e-221,2e-221,3e-221,4e-221,5e-221,6e-221,7e-221,8e-221,9e-221,1e-220,2e-220,
	3e-220,4e-220,5e-220,6e-220,7e-220,8e-220,9e-220,1e-219,2e-219,3e-219,4e-219,5e-219,6e-219,
	7e-219,8e-219,9e-219,1e-218,2e-218,3e-218,4e-218,5e-218,6e-218,7e-218,8e-218,9e-218,1e-217,
	2e-217,3e-217,4e-217,5e-217,6e-217,7e-217,8e-217,9e-217,1e-216,2e-216,3e-216,4e-216,5e-216,
	6e-216,7e-216,8e-216,9e-216,1e-215,2e-215,3e-215,4e-215,5e-215,6e-215,7e-215,8e-215,9e-215,
	1e-214,2e-214,3e-214,4e-214,5e-214,6e-214,7e-214,8e-214,9e-214,1e-213,2e-213,3e-213,4e-213,
	5e-213,6e-213,7e-213,8e-213,9e-213,1e-212,2e-212,3e-212,4e-212,5e-212,6e-212,7e-212,8e-212,
	9e-212,1e-211,2e-211,3e-211,4e-211,5e-211,6e-211,7e-211,8e-211,9e-211,1e-210,2e-210,3e-210,
	4e-210,5e-210,6e-210,7e-210,8e-210,9e-210,1e-209,2e-209,3e-209,4e-209,5e-209,6e-209,7e-209,
	8e-209,9e-209,1e-208,2e-208,3e-208,4e-208,5e-208,6e-208,7e-208,8e-208,9e-208,1e-207,2e-207,
	3e-207,4e-207,5e-207,6e-207,7e-207,8e-207,9e-207,1e-206,2e-206,3e-206,4e-206,5e-206,6e-206,
	7e-206,8e-206,9e-206,1e-205,2e-205,3e-205,4e-205,5e-205,6e-205,7e-205,8e-205,9e-205,1e-204,
	2e-204,3e-204,4e-204,5e-204,6e-204,7e-204,8e-204,9e-204,1e-203,2e-203,3e-203,4e-203,5e-203,
	6e-203,7e-203,8e-203,9e-203,1e-202,2e-202,3e-202,4e-202,5e-202,6e-202,7e-202,8e-202,9e-202,
	1e-201,2e-201,3e-201,4e-201,5e-201,6e-201,7e-201,8e-201,9e-201,1e-200,2e-200,3e-200,4e-200,
	5e-200,6e-200,7e-200,8e-200,9e-200,1e-199,2e-199,3e-199,4e-199,5e-199,6e-199,7e-199,8e-199,
	9e-199,1e-198,2e-198,3e-198,4e-198,5e-198,6e-198,7e-198,8e-198,9e-198,1e-197,2e-197,3e-197,
	4e-197,5e-197,6e-197,7e-197,8e-197,9e-197,1e-196,2e-196,3e-196,4e-196,5e-196,6e-196,7e-196,
	8e-196,9e-196,1e-195,2e-195,3e-195,4e-195,5e-195,6e-195,7e-195,8e-195,9e-195,1e-194,2e-194,
	3e-194,4e-194,5e-194,6e-194,7e-194,8e-194,9e-194,1e-193,2e-193,3e-193,4e-193,5e-193,6e-193,
	7e-193,8e-193,9e-193,1e-192,2e-192,3e-192,4e-192,5e-192,6e-192,7e-192,8e-192,9e-192,1e-191,
	2e-191,3e-191,4e-191,5e-191,6e-191,7e-191,8e-191,9e-191,1e-190,2e-190,3e-190,4e-190,5e-190,
	6e-190,7e-190,8e-190,9e-190,1e-189,2e-189,3e-189,4e-189,5e-189,6e-189,7e-189,8e-189,9e-189,
	1e-188,2e-188,3e-188,4e-188,5e-188,6e-188,7e-188,8e-188,9e-188,1e-187,2e-187,3e-187,4e-187,
	5e-187,6e-187,7e-187,8e-187,9e-187,1e-186,2e-186,3e-186,4e-186,5e-186,6e-186,7e-186,8e-186,
	9e-186,1e-185,2e-185,3e-185,4e-185,5e-185,6e-185,7e-185,8e-185,9e-185,1e-184,2e-184,3e-184,
	4e-184,5e-184,6e-184,7e-184,8e-184,9e-184,1e-183,2e-183,3e-183,4e-183,5e-183,6e-183,7e-183,
	8e-183,9e-183,1e-182,2e-182,3e-182,4e-182,5e-182,6e-182,7e-182,8e-182,9e-182,1e-181,2e-181,
	3e-181,4e-181,5e-181,6e-181,7e-181,8e-181,9e-181,1e-180,2e-180,3e-180,4e-180,5e-180,6e-180,
	7e-180,8e-180,9e-180,1e-179,2e-179,3e-179,4e-179,5e-179,6e-179,7e-179,8e-179,9e-179,1e-178,
	2e-178,3e-178,4e-178,5e-178,6e-178,7e-178,8e-178,9e-178,1e-177,2e-177,3e-177,4e-177,5e-177,
	6e-177,7e-177,8e-177,9e-177,1e-176,2e-176,3e-176,4e-176,5e-176,6e-176,7e-176,8e-176,9e-176,
	1e-175,2e-175,3e-175,4e-175,5e-175,6e-175,7e-175,8e-175,9e-175,1e-174,2e-174,3e-174,4e-174,
	5e-174,6e-174,7e-174,8e-174,9e-174,1e-173,2e-173,3e-173,4e-173,5e-173,6e-173,7e-173,8e-173,
	9e-173,1e-172,2e-172,3e-172,4e-172,5e-172,6e-172,7e-172,8e-172,9e-172,1e-171,2e-171,3e-171,
	4e-171,5e-171,6e-171,7e-171,8e-171,9e-171,1e-170,2e-170,3e-170,4e-170,5e-170,6e-170,7e-170,
	8e-170,9e-170,1e-169,2e-169,3e-169,4e-169,5e-169,6e-169,7e-169,8e-169,9e-169,1e-168,2e-168,
	3e-168,4e-168,5e-168,6e-168,7e-168,8e-168,9e-168,1e-167,2e-167,3e-167,4e-167,5e-167,6e-167,
	7e-167,8e-167,9e-167,1e-166,2e-166,3e-166,4e-166,5e-166,6e-166,7e-166,8e-166,9e-166,1e-165,
	2e-165,3e-165,4e-165,5e-165,6e-165,7e-165,8e-165,9e-165,1e-164,2e-164,3e-164,4e-164,5e-164,
	6e-164,7e-164,8e-164,9e-164,1e-163,2e-163,3e-163,4e-163,5e-163,6e-163,7e-163,8e-163,9e-163,
	1e-162,2e-162,3e-162,4e-162,5e-162,6e-162,7e-162,8e-162,9e-162,1e-161,2e-161,3e-161,4e-161,
	5e-161,6e-161,7e-161,8e-161,9e-161,1e-160,2e-160,3e-160,4e-160,5e-160,6e-160,7e-160,8e-160,
	9e-160,1e-159,2e-159,3e-159,4e-159,5e-159,6e-159,7e-159,8e-159,9e-159,1e-158,2e-158,3e-158,
	4e-158,5e-158,6e-158,7e-158,8e-158,9e-158,1e-157,2e-157,3e-157,4e-157,5e-157,6e-157,7e-157,
	8e-157,9e-157,1e-156,2e-156,3e-156,4e-156,5e-156,6e-156,7e-156,8e-156,9e-156,1e-155,2e-155,
	3e-155,4e-155,5e-155,6e-155,7e-155,8e-155,9e-155,1e-154,2e-154,3e-154,4e-154,5e-154,6e-154,
	7e-154,8e-154,9e-154,1e-153,2e-153,3e-153,4e-153,5e-153,6e-153,7e-153,8e-153,9e-153,1e-152,
	2e-152,3e-152,4e-152,5e-152,6e-152,7e-152,8e-152,9e-152,1e-151,2e-151,3e-151,4e-151,5e-151,
	6e-151,7e-151,8e-151,9e-151,1e-150,2e-150,3e-150,4e-150,5e-150,6e-150,7e-150,8e-150,9e-150,
	1e-149,2e-149,3e-149,4e-149,5e-149,6e-149,7e-149,8e-149,9e-149,1e-148,2e-148,3e-148,4e-148,
	5e-148,6e-148,7e-148,8e-148,9e-148,1e-147,2e-147,3e-147,4e-147,5e-147,6e-147,7e-147,8e-147,
	9e-147,1e-146,2e-146,3e-146,4e-146,5e-146,6e-146,7e-146,8e-146,9e-146,1e-145,2e-145,3e-145,
	4e-145,5e-145,6e-145,7e-145,8e-145,9e-145,1e-144,2e-144,3e-144,4e-144,5e-144,6e-144,7e-144,
	8e-144,9e-144,1e-143,2e-143,3e-143,4e-143,5e-143,6e-143,7e-143,8e-143,9e-143,1e-142,2e-142,
	3e-142,4e-142,5e-142,6e-142,7e-142,8e-142,9e-142,1e-141,2e-141,3e-141,4e-141,5e-141,6e-141,
	7e-141,8e-141,9e-141,1e-140,2e-140,3e-140,4e-140,5e-140,6e-140,7e-140,8e-140,9e-140,1e-139,
	2e-139,3e-139,4e-139,5e-139,6e-139,7e-139,8e-139,9e-139,1e-138,2e-138,3e-138,4e-138,5e-138,
	6e-138,7e-138,8e-138,9e-138,1e-137,2e-137,3e-137,4e-137,5e-137,6e-137,7e-137,8e-137,9e-137,
	1e-136,2e-136,3e-136,4e-136,5e-136,6e-136,7e-136,8e-136,9e-136,1e-135,2e-135,3e-135,4e-135,
	5e-135,6e-135,7e-135,8e-135,9e-135,1e-134,2e-134,3e-134,4e-134,5e-134,6e-134,7e-134,8e-134,
	9e-134,1e-133,2e-133,3e-133,4e-133,5e-133,6e-133,7e-133,8e-133,9e-133,1e-132,2e-132,3e-132,
	4e-132,5e-132,6e-132,7e-132,8e-132,9e-132,1e-131,2e-131,3e-131,4e-131,5e-131,6e-131,7e-131,
	8e-131,9e-131,1e-130,2e-130,3e-130,4e-130,5e-130,6e-130,7e-130,8e-130,9e-130,1e-129,2e-129,
	3e-129,4e-129,5e-129,6e-129,7e-129,8e-129,9e-129,1e-128,2e-128,3e-128,4e-128,5e-128,6e-128,
	7e-128,8e-128,9e-128,1e-127,2e-127,3e-127,4e-127,5e-127,6e-127,7e-127,8e-127,9e-127,1e-126,
	2e-126,3e-126,4e-126,5e-126,6e-126,7e-126,8e-126,9e-126,1e-125,2e-125,3e-125,4e-125,5e-125,
	6e-125,7e-125,8e-125,9e-125,1e-124,2e-124,3e-124,4e-124,5e-124,6e-124,7e-124,8e-124,9e-124,
	1e-123,2e-123,3e-123,4e-123,5e-123,6e-123,7e-123,8e-123,9e-123,1e-122,2e-122,3e-122,4e-122,
	5e-122,6e-122,7e-122,8e-122,9e-122,1e-121,2e-121,3e-121,4e-121,5e-121,6e-121,7e-121,8e-121,
	9e-121,1e-120,2e-120,3e-120,4e-120,5e-120,6e-120,7e-120,8e-120,9e-120,1e-119,2e-119,3e-119,
	4e-119,5e-119,6e-119,7e-119,8e-119,9e-119,1e-118,2e-118,3e-118,4e-118,5e-118,6e-118,7e-118,
	8e-118,9e-118,1e-117,2e-117,3e-117,4e-117,5e-117,6e-117,7e-117,8e-117,9e-117,1e-116,2e-116,
	3e-116,4e-116,5e-116,6e-116,7e-116,8e-116,9e-116,1e-115,2e-115,3e-115,4e-115,5e-115,6e-115,
	7e-115,8e-115,9e-115,1e-114,2e-114,3e-114,4e-114,5e-114,6e-114,7e-114,8e-114,9e-114,1e-113,
	2e-113,3e-113,4e-113,5e-113,6e-113,7e-113,8e-113,9e-113,1e-112,2e-112,3e-112,4e-112,5e-112,
	6e-112,7e-112,8e-112,9e-112,1e-111,2e-111,3e-111,4e-111,5e-111,6e-111,7e-111,8e-111,9e-111,
	1e-110,2e-110,3e-110,4e-110,5e-110,6e-110,7e-110,8e-110,9e-110,1e-109,2e-109,3e-109,4e-109,
	5e-109,6e-109,7e-109,8e-109,9e-109,1e-108,2e-108,3e-108,4e-108,5e-108,6e-108,7e-108,8e-108,
	9e-108,1e-107,2e-107,3e-107,4e-107,5e-107,6e-107,7e-107,8e-107,9e-107,1e-106,2e-106,3e-106,
	4e-106,5e-106,6e-106,7e-106,8e-106,9e-106,1e-105,2e-105,3e-105,4e-105,5e-105,6e-105,7e-105,
	8e-105,9e-105,1e-104,2e-104,3e-104,4e-104,5e-104,6e-104,7e-104,8e-104,9e-104,1e-103,2e-103,
	3e-103,4e-103,5e-103,6e-103,7e-103,8e-103,9e-103,1e-102,2e-102,3e-102,4e-102,5e-102,6e-102,
	7e-102,8e-102,9e-102,1e-101,2e-101,3e-101,4e-101,5e-101,6e-101,7e-101,8e-101,9e-101,1e-100,
	2e-100,3e-100,4e-100,5e-100,6e-100,7e-100,8e-100,9e-100,1e-99,2e-99,3e-99,4e-99,5e-99,6e-99,
	7e-99,8e-99,9e-99,1e-98,2e-98,3e-98,4e-98,5e-98,6e-98,7e-98,8e-98,9e-98,1e-97,2e-97,3e-97,
	4e-97,5e-97,6e-97,7e-97,8e-97,9e-97,1e-96,2e-96,3e-96,4e-96,5e-96,6e-96,7e-96,8e-96,9e-96,
	1e-95,2e-95,3e-95,4e-95,5e-95,6e-95,7e-95,8e-95,9e-95,1e-94,2e-94,3e-94,4e-94,5e-94,6e-94,
	7e-94,8e-94,9e-94,1e-93,2e-93,3e-93,4e-93,5e-93,6e-93,7e-93,8e-93,9e-93,1e-92,2e-92,3e-92,
	4e-92,5e-92,6e-92,7e-92,8e-92,9e-92,1e-91,2e-91,3e-91,4e-91,5e-91,6e-91,7e-91,8e-91,9e-91,
	1e-90,2e-90,3e-90,4e-90,5e-90,6e-90,7e-90,8e-90,9e-90,1e-89,2e-89,3e-89,4e-89,5e-89,6e-89,
	7e-89,8e-89,9e-89,1e-88,2e-88,3e-88,4e-88,5e-88,6e-88,7e-88,8e-88,9e-88,1e-87,2e-87,3e-87,
	4e-87,5e-87,6e-87,7e-87,8e-87,9e-87,1e-86,2e-86,3e-86,4e-86,5e-86,6e-86,7e-86,8e-86,9e-86,
	1e-85,2e-85,3e-85,4e-85,5e-85,6e-85,7e-85,8e-85,9e-85,1e-84,2e-84,3e-84,4e-84,5e-84,6e-84,
	7e-84,8e-84,9e-84,1e-83,2e-83,3e-83,4e-83,5e-83,6e-83,7e-83,8e-83,9e-83,1e-82,2e-82,3e-82,
	4e-82,5e-82,6e-82,7e-82,8e-82,9e-82,1e-81,2e-81,3e-81,4e-81,5e-81,6e-81,7e-81,8e-81,9e-81,
	1e-80,2e-80,3e-80,4e-80,5e-80,6e-80,7e-80,8e-80,9e-80,1e-79,2e-79,3e-79,4e-79,5e-79,6e-79,
	7e-79,8e-79,9e-79,1e-78,2e-78,3e-78,4e-78,5e-78,6e-78,7e-78,8e-78,9e-78,1e-77,2e-77,3e-77,
	4e-77,5e-77,6e-77,7e-77,8e-77,9e-77,1e-76,2e-76,3e-76,4e-76,5e-76,6e-76,7e-76,8e-76,9e-76,
	1e-75,2e-75,3e-75,4e-75,5e-75,6e-75,7e-75,8e-75,9e-75,1e-74,2e-74,3e-74,4e-74,5e-74,6e-74,
	7e-74,8e-74,9e-74,1e-73,2e-73,3e-73,4e-73,5e-73,6e-73,7e-73,8e-73,9e-73,1e-72,2e-72,3e-72,
	4e-72,5e-72,6e-72,7e-72,8e-72,9e-72,1e-71,2e-71,3e-71,4e-71,5e-71,6e-71,7e-71,8e-71,9e-71,
	1e-70,2e-70,3e-70,4e-70,5e-70,6e-70,7e-70,8e-70,9e-70,1e-69,2e-69,3e-69,4e-69,5e-69,6e-69,
	7e-69,8e-69,9e-69,1e-68,2e-68,3e-68,4e-68,5e-68,6e-68,7e-68,8e-68,9e-68,1e-67,2e-67,3e-67,
	4e-67,5e-67,6e-67,7e-67,8e-67,9e-67,1e-66,2e-66,3e-66,4e-66,5e-66,6e-66,7e-66,8e-66,9e-66,
	1e-65,2e-65,3e-65,4e-65,5e-65,6e-65,7e-65,8e-65,9e-65,1e-64,2e-64,3e-64,4e-64,5e-64,6e-64,
	7e-64,8e-64,9e-64,1e-63,2e-63,3e-63,4e-63,5e-63,6e-63,7e-63,8e-63,9e-63,1e-62,2e-62,3e-62,
	4e-62,5e-62,6e-62,7e-62,8e-62,9e-62,1e-61,2e-61,3e-61,4e-61,5e-61,6e-61,7e-61,8e-61,9e-61,
	1e-60,2e-60,3e-60,4e-60,5e-60,6e-60,7e-60,8e-60,9e-60,1e-59,2e-59,3e-59,4e-59,5e-59,6e-59,
	7e-59,8e-59,9e-59,1e-58,2e-58,3e-58,4e-58,5e-58,6e-58,7e-58,8e-58,9e-58,1e-57,2e-57,3e-57,
	4e-57,5e-57,6e-57,7e-57,8e-57,9e-57,1e-56,2e-56,3e-56,4e-56,5e-56,6e-56,7e-56,8e-56,9e-56,
	1e-55,2e-55,3e-55,4e-55,5e-55,6e-55,7e-55,8e-55,9e-55,1e-54,2e-54,3e-54,4e-54,5e-54,6e-54,
	7e-54,8e-54,9e-54,1e-53,2e-53,3e-53,4e-53,5e-53,6e-53,7e-53,8e-53,9e-53,1e-52,2e-52,3e-52,
	4e-52,5e-52,6e-52,7e-52,8e-52,9e-52,1e-51,2e-51,3e-51,4e-51,5e-51,6e-51,7e-51,8e-51,9e-51,
	1e-50,2e-50,3e-50,4e-50,5e-50,6e-50,7e-50,8e-50,9e-50,1e-49,2e-49,3e-49,4e-49,5e-49,6e-49,
	7e-49,8e-49,9e-49,1e-48,2e-48,3e-48,4e-48,5e-48,6e-48,7e-48,8e-48,9e-48,1e-47,2e-47,3e-47,
	4e-47,5e-47,6e-47,7e-47,8e-47,9e-47,1e-46,2e-46,3e-46,4e-46,5e-46,6e-46,7e-46,8e-46,9e-46,
	1e-45,2e-45,3e-45,4e-45,5e-45,6e-45,7e-45,8e-45,9e-45,1e-44,2e-44,3e-44,4e-44,5e-44,6e-44,
	7e-44,8e-44,9e-44,1e-43,2e-43,3e-43,4e-43,5e-43,6e-43,7e-43,8e-43,9e-43,1e-42,2e-42,3e-42,
	4e-42,5e-42,6e-42,7e-42,8e-42,9e-42,1e-41,2e-41,3e-41,4e-41,5e-41,6e-41,7e-41,8e-41,9e-41,
	1e-40,2e-40,3e-40,4e-40,5e-40,6e-40,7e-40,8e-40,9e-40,1e-39,2e-39,3e-39,4e-39,5e-39,6e-39,
	7e-39,8e-39,9e-39,1e-38,2e-38,3e-38,4e-38,5e-38,6e-38,7e-38,8e-38,9e-38,1e-37,2e-37,3e-37,
	4e-37,5e-37,6e-37,7e-37,8e-37,9e-37,1e-36,2e-36,3e-36,4e-36,5e-36,6e-36,7e-36,8e-36,9e-36,
	1e-35,2e-35,3e-35,4e-35,5e-35,6e-35,7e-35,8e-35,9e-35,1e-34,2e-34,3e-34,4e-34,5e-34,6e-34,
	7e-34,8e-34,9e-34,1e-33,2e-33,3e-33,4e-33,5e-33,6e-33,7e-33,8e-33,9e-33,1e-32,2e-32,3e-32,
	4e-32,5e-32,6e-32,7e-32,8e-32,9e-32,1e-31,2e-31,3e-31,4e-31,5e-31,6e-31,7e-31,8e-31,9e-31,
	1e-30,2e-30,3e-30,4e-30,5e-30,6e-30,7e-30,8e-30,9e-30,1e-29,2e-29,3e-29,4e-29,5e-29,6e-29,
	7e-29,8e-29,9e-29,1e-28,2e-28,3e-28,4e-28,5e-28,6e-28,7e-28,8e-28,9e-28,1e-27,2e-27,3e-27,
	4e-27,5e-27,6e-27,7e-27,8e-27,9e-27,1e-26,2e-26,3e-26,4e-26,5e-26,6e-26,7e-26,8e-26,9e-26,
	1e-25,2e-25,3e-25,4e-25,5e-25,6e-25,7e-25,8e-25,9e-25,1e-24,2e-24,3e-24,4e-24,5e-24,6e-24,
	7e-24,8e-24,9e-24,1e-23,2e-23,3e-23,4e-23,5e-23,6e-23,7e-23,8e-23,9e-23,1e-22,2e-22,3e-22,
	4e-22,5e-22,6e-22,7e-22,8e-22,9e-22,1e-21,2e-21,3e-21,4e-21,5e-21,6e-21,7e-21,8e-21,9e-21,
	1e-20,2e-20,3e-20,4e-20,5e-20,6e-20,7e-20,8e-20,9e-20,1e-19,2e-19,3e-19,4e-19,5e-19,6e-19,
	7e-19,8e-19,9e-19,1e-18,2e-18,3e-18,4e-18,5e-18,6e-18,7e-18,8e-18,9e-18,1e-17,2e-17,3e-17,
	4e-17,5e-17,6e-17,7e-17,8e-17,9e-17,1e-16,2e-16,3e-16,4e-16,5e-16,6e-16,7e-16,8e-16,9e-16,
	1e-15,2e-15,3e-15,4e-15,5e-15,6e-15,7e-15,8e-15,9e-15,1e-14,2e-14,3e-14,4e-14,5e-14,6e-14,
	7e-14,8e-14,9e-14,1e-13,2e-13,3e-13,4e-13,5e-13,6e-13,7e-13,8e-13,9e-13,1e-12,2e-12,3e-12,
	4e-12,5e-12,6e-12,7e-12,8e-12,9e-12,1e-11,2e-11,3e-11,4e-11,5e-11,6e-11,7e-11,8e-11,9e-11,
	1e-10,2e-10,3e-10,4e-10,5e-10,6e-10,7e-10,8e-10,9e-10,1e-9,2e-9,3e-9,4e-9,5e-9,6e-9,7e-9,8e-9,
	9e-9,1e-8,2e-8,3e-8,4e-8,5e-8,6e-8,7e-8,8e-8,9e-8,1e-7,2e-7,3e-7,4e-7,5e-7,6e-7,7e-7,8e-7,
	9e-7,1e-6,2e-6,3e-6,4e-6,5e-6,6e-6,7e-6,8e-6,9e-6,1e-5,2e-5,3e-5,4e-5,5e-5,6e-5,7e-5,8e-5,
	9e-5,1e-4,2e-4,3e-4,4e-4,5e-4,6e-4,7e-4,8e-4,9e-4,1e-3,2e-3,3e-3,4e-3,5e-3,6e-3,7e-3,8e-3,
	9e-3,1e-2,2e-2,3e-2,4e-2,5e-2,6e-2,7e-2,8e-2,9e-2,1e-1,2e-1,3e-1,4e-1,5e-1,6e-1,7e-1,8e-1,
	9e-1,1e+0,2e+0,3e+0,4e+0,5e+0,6e+0,7e+0,8e+0,9e+0,1e+1,2e+1,3e+1,4e+1,5e+1,6e+1,7e+1,8e+1,
	9e+1,1e+2,2e+2,3e+2,4e+2,5e+2,6e+2,7e+2,8e+2,9e+2,1e+3,2e+3,3e+3,4e+3,5e+3,6e+3,7e+3,8e+3,
	9e+3,1e+4,2e+4,3e+4,4e+4,5e+4,6e+4,7e+4,8e+4,9e+4,1e+5,2e+5,3e+5,4e+5,5e+5,6e+5,7e+5,8e+5,
	9e+5,1e+6,2e+6,3e+6,4e+6,5e+6,6e+6,7e+6,8e+6,9e+6,1e+7,2e+7,3e+7,4e+7,5e+7,6e+7,7e+7,8e+7,
	9e+7,1e+8,2e+8,3e+8,4e+8,5e+8,6e+8,7e+8,8e+8,9e+8,1e+9,2e+9,3e+9,4e+9,5e+9,6e+9,7e+9,8e+9,
	9e+9,1e+10,2e+10,3e+10,4e+10,5e+10,6e+10,7e+10,8e+10,9e+10,1e+11,2e+11,3e+11,4e+11,5e+11,6e+11,
	7e+11,8e+11,9e+11,1e+12,2e+12,3e+12,4e+12,5e+12,6e+12,7e+12,8e+12,9e+12,1e+13,2e+13,3e+13,
	4e+13,5e+13,6e+13,7e+13,8e+13,9e+13,1e+14,2e+14,3e+14,4e+14,5e+14,6e+14,7e+14,8e+14,9e+14,
	1e+15,2e+15,3e+15,4e+15,5e+15,6e+15,7e+15,8e+15,9e+15,1e+16,2e+16,3e+16,4e+16,5e+16,6e+16,
	7e+16,8e+16,9e+16,1e+17,2e+17,3e+17,4e+17,5e+17,6e+17,7e+17,8e+17,9e+17,1e+18,2e+18,3e+18,
	4e+18,5e+18,6e+18,7e+18,8e+18,9e+18,1e+19,2e+19,3e+19,4e+19,5e+19,6e+19,7e+19,8e+19,9e+19,
	1e+20,2e+20,3e+20,4e+20,5e+20,6e+20,7e+20,8e+20,9e+20,1e+21,2e+21,3e+21,4e+21,5e+21,6e+21,
	7e+21,8e+21,9e+21,1e+22,2e+22,3e+22,4e+22,5e+22,6e+22,7e+22,8e+22,9e+22,1e+23,2e+23,3e+23,
	4e+23,5e+23,6e+23,7e+23,8e+23,9e+23,1e+24,2e+24,3e+24,4e+24,5e+24,6e+24,7e+24,8e+24,9e+24,
	1e+25,2e+25,3e+25,4e+25,5e+25,6e+25,7e+25,8e+25,9e+25,1e+26,2e+26,3e+26,4e+26,5e+26,6e+26,
	7e+26,8e+26,9e+26,1e+27,2e+27,3e+27,4e+27,5e+27,6e+27,7e+27,8e+27,9e+27,1e+28,2e+28,3e+28,
	4e+28,5e+28,6e+28,7e+28,8e+28,9e+28,1e+29,2e+29,3e+29,4e+29,5e+29,6e+29,7e+29,8e+29,9e+29,
	1e+30,2e+30,3e+30,4e+30,5e+30,6e+30,7e+30,8e+30,9e+30,1e+31,2e+31,3e+31,4e+31,5e+31,6e+31,
	7e+31,8e+31,9e+31,1e+32,2e+32,3e+32,4e+32,5e+32,6e+32,7e+32,8e+32,9e+32,1e+33,2e+33,3e+33,
	4e+33,5e+33,6e+33,7e+33,8e+33,9e+33,1e+34,2e+34,3e+34,4e+34,5e+34,6e+34,7e+34,8e+34,9e+34,
	1e+35,2e+35,3e+35,4e+35,5e+35,6e+35,7e+35,8e+35,9e+35,1e+36,2e+36,3e+36,4e+36,5e+36,6e+36,
	7e+36,8e+36,9e+36,1e+37,2e+37,3e+37,4e+37,5e+37,6e+37,7e+37,8e+37,9e+37,1e+38,2e+38,3e+38,
	4e+38,5e+38,6e+38,7e+38,8e+38,9e+38,1e+39,2e+39,3e+39,4e+39,5e+39,6e+39,7e+39,8e+39,9e+39,
	1e+40,2e+40,3e+40,4e+40,5e+40,6e+40,7e+40,8e+40,9e+40,1e+41,2e+41,3e+41,4e+41,5e+41,6e+41,
	7e+41,8e+41,9e+41,1e+42,2e+42,3e+42,4e+42,5e+42,6e+42,7e+42,8e+42,9e+42,1e+43,2e+43,3e+43,
	4e+43,5e+43,6e+43,7e+43,8e+43,9e+43,1e+44,2e+44,3e+44,4e+44,5e+44,6e+44,7e+44,8e+44,9e+44,
	1e+45,2e+45,3e+45,4e+45,5e+45,6e+45,7e+45,8e+45,9e+45,1e+46,2e+46,3e+46,4e+46,5e+46,6e+46,
	7e+46,8e+46,9e+46,1e+47,2e+47,3e+47,4e+47,5e+47,6e+47,7e+47,8e+47,9e+47,1e+48,2e+48,3e+48,
	4e+48,5e+48,6e+48,7e+48,8e+48,9e+48,1e+49,2e+49,3e+49,4e+49,5e+49,6e+49,7e+49,8e+49,9e+49,
	1e+50,2e+50,3e+50,4e+50,5e+50,6e+50,7e+50,8e+50,9e+50,1e+51,2e+51,3e+51,4e+51,5e+51,6e+51,
	7e+51,8e+51,9e+51,1e+52,2e+52,3e+52,4e+52,5e+52,6e+52,7e+52,8e+52,9e+52,1e+53,2e+53,3e+53,
	4e+53,5e+53,6e+53,7e+53,8e+53,9e+53,1e+54,2e+54,3e+54,4e+54,5e+54,6e+54,7e+54,8e+54,9e+54,
	1e+55,2e+55,3e+55,4e+55,5e+55,6e+55,7e+55,8e+55,9e+55,1e+56,2e+56,3e+56,4e+56,5e+56,6e+56,
	7e+56,8e+56,9e+56,1e+57,2e+57,3e+57,4e+57,5e+57,6e+57,7e+57,8e+57,9e+57,1e+58,2e+58,3e+58,
	4e+58,5e+58,6e+58,7e+58,8e+58,9e+58,1e+59,2e+59,3e+59,4e+59,5e+59,6e+59,7e+59,8e+59,9e+59,
	1e+60,2e+60,3e+60,4e+60,5e+60,6e+60,7e+60,8e+60,9e+60,1e+61,2e+61,3e+61,4e+61,5e+61,6e+61,
	7e+61,8e+61,9e+61,1e+62,2e+62,3e+62,4e+62,5e+62,6e+62,7e+62,8e+62,9e+62,1e+63,2e+63,3e+63,
	4e+63,5e+63,6e+63,7e+63,8e+63,9e+63,1e+64,2e+64,3e+64,4e+64,5e+64,6e+64,7e+64,8e+64,9e+64,
	1e+65,2e+65,3e+65,4e+65,5e+65,6e+65,7e+65,8e+65,9e+65,1e+66,2e+66,3e+66,4e+66,5e+66,6e+66,
	7e+66,8e+66,9e+66,1e+67,2e+67,3e+67,4e+67,5e+67,6e+67,7e+67,8e+67,9e+67,1e+68,2e+68,3e+68,
	4e+68,5e+68,6e+68,7e+68,8e+68,9e+68,1e+69,2e+69,3e+69,4e+69,5e+69,6e+69,7e+69,8e+69,9e+69,
	1e+70,2e+70,3e+70,4e+70,5e+70,6e+70,7e+70,8e+70,9e+70,1e+71,2e+71,3e+71,4e+71,5e+71,6e+71,
	7e+71,8e+71,9e+71,1e+72,2e+72,3e+72,4e+72,5e+72,6e+72,7e+72,8e+72,9e+72,1e+73,2e+73,3e+73,
	4e+73,5e+73,6e+73,7e+73,8e+73,9e+73,1e+74,2e+74,3e+74,4e+74,5e+74,6e+74,7e+74,8e+74,9e+74,
	1e+75,2e+75,3e+75,4e+75,5e+75,6e+75,7e+75,8e+75,9e+75,1e+76,2e+76,3e+76,4e+76,5e+76,6e+76,
	7e+76,8e+76,9e+76,1e+77,2e+77,3e+77,4e+77,5e+77,6e+77,7e+77,8e+77,9e+77,1e+78,2e+78,3e+78,
	4e+78,5e+78,6e+78,7e+78,8e+78,9e+78,1e+79,2e+79,3e+79,4e+79,5e+79,6e+79,7e+79,8e+79,9e+79,
	1e+80,2e+80,3e+80,4e+80,5e+80,6e+80,7e+80,8e+80,9e+80,1e+81,2e+81,3e+81,4e+81,5e+81,6e+81,
	7e+81,8e+81,9e+81,1e+82,2e+82,3e+82,4e+82,5e+82,6e+82,7e+82,8e+82,9e+82,1e+83,2e+83,3e+83,
	4e+83,5e+83,6e+83,7e+83,8e+83,9e+83,1e+84,2e+84,3e+84,4e+84,5e+84,6e+84,7e+84,8e+84,9e+84,
	1e+85,2e+85,3e+85,4e+85,5e+85,6e+85,7e+85,8e+85,9e+85,1e+86,2e+86,3e+86,4e+86,5e+86,6e+86,
	7e+86,8e+86,9e+86,1e+87,2e+87,3e+87,4e+87,5e+87,6e+87,7e+87,8e+87,9e+87,1e+88,2e+88,3e+88,
	4e+88,5e+88,6e+88,7e+88,8e+88,9e+88,1e+89,2e+89,3e+89,4e+89,5e+89,6e+89,7e+89,8e+89,9e+89,
	1e+90,2e+90,3e+90,4e+90,5e+90,6e+90,7e+90,8e+90,9e+90,1e+91,2e+91,3e+91,4e+91,5e+91,6e+91,
	7e+91,8e+91,9e+91,1e+92,2e+92,3e+92,4e+92,5e+92,6e+92,7e+92,8e+92,9e+92,1e+93,2e+93,3e+93,
	4e+93,5e+93,6e+93,7e+93,8e+93,9e+93,1e+94,2e+94,3e+94,4e+94,5e+94,6e+94,7e+94,8e+94,9e+94,
	1e+95,2e+95,3e+95,4e+95,5e+95,6e+95,7e+95,8e+95,9e+95,1e+96,2e+96,3e+96,4e+96,5e+96,6e+96,
	7e+96,8e+96,9e+96,1e+97,2e+97,3e+97,4e+97,5e+97,6e+97,7e+97,8e+97,9e+97,1e+98,2e+98,3e+98,
	4e+98,5e+98,6e+98,7e+98,8e+98,9e+98,1e+99,2e+99,3e+99,4e+99,5e+99,6e+99,7e+99,8e+99,9e+99,
	1e+100,2e+100,3e+100,4e+100,5e+100,6e+100,7e+100,8e+100,9e+100,1e+101,2e+101,3e+101,4e+101,
	5e+101,6e+101,7e+101,8e+101,9e+101,1e+102,2e+102,3e+102,4e+102,5e+102,6e+102,7e+102,8e+102,
	9e+102,1e+103,2e+103,3e+103,4e+103,5e+103,6e+103,7e+103,8e+103,9e+103,1e+104,2e+104,3e+104,
	4e+104,5e+104,6e+104,7e+104,8e+104,9e+104,1e+105,2e+105,3e+105,4e+105,5e+105,6e+105,7e+105,
	8e+105,9e+105,1e+106,2e+106,3e+106,4e+106,5e+106,6e+106,7e+106,8e+106,9e+106,1e+107,2e+107,
	3e+107,4e+107,5e+107,6e+107,7e+107,8e+107,9e+107,1e+108,2e+108,3e+108,4e+108,5e+108,6e+108,
	7e+108,8e+108,9e+108,1e+109,2e+109,3e+109,4e+109,5e+109,6e+109,7e+109,8e+109,9e+109,1e+110,
	2e+110,3e+110,4e+110,5e+110,6e+110,7e+110,8e+110,9e+110,1e+111,2e+111,3e+111,4e+111,5e+111,
	6e+111,7e+111,8e+111,9e+111,1e+112,2e+112,3e+112,4e+112,5e+112,6e+112,7e+112,8e+112,9e+112,
	1e+113,2e+113,3e+113,4e+113,5e+113,6e+113,7e+113,8e+113,9e+113,1e+114,2e+114,3e+114,4e+114,
	5e+114,6e+114,7e+114,8e+114,9e+114,1e+115,2e+115,3e+115,4e+115,5e+115,6e+115,7e+115,8e+115,
	9e+115,1e+116,2e+116,3e+116,4e+116,5e+116,6e+116,7e+116,8e+116,9e+116,1e+117,2e+117,3e+117,
	4e+117,5e+117,6e+117,7e+117,8e+117,9e+117,1e+118,2e+118,3e+118,4e+118,5e+118,6e+118,7e+118,
	8e+118,9e+118,1e+119,2e+119,3e+119,4e+119,5e+119,6e+119,7e+119,8e+119,9e+119,1e+120,2e+120,
	3e+120,4e+120,5e+120,6e+120,7e+120,8e+120,9e+120,1e+121,2e+121,3e+121,4e+121,5e+121,6e+121,
	7e+121,8e+121,9e+121,1e+122,2e+122,3e+122,4e+122,5e+122,6e+122,7e+122,8e+122,9e+122,1e+123,
	2e+123,3e+123,4e+123,5e+123,6e+123,7e+123,8e+123,9e+123,1e+124,2e+124,3e+124,4e+124,5e+124,
	6e+124,7e+124,8e+124,9e+124,1e+125,2e+125,3e+125,4e+125,5e+125,6e+125,7e+125,8e+125,9e+125,
	1e+126,2e+126,3e+126,4e+126,5e+126,6e+126,7e+126,8e+126,9e+126,1e+127,2e+127,3e+127,4e+127,
	5e+127,6e+127,7e+127,8e+127,9e+127,1e+128,2e+128,3e+128,4e+128,5e+128,6e+128,7e+128,8e+128,
	9e+128,1e+129,2e+129,3e+129,4e+129,5e+129,6e+129,7e+129,8e+129,9e+129,1e+130,2e+130,3e+130,
	4e+130,5e+130,6e+130,7e+130,8e+130,9e+130,1e+131,2e+131,3e+131,4e+131,5e+131,6e+131,7e+131,
	8e+131,9e+131,1e+132,2e+132,3e+132,4e+132,5e+132,6e+132,7e+132,8e+132,9e+132,1e+133,2e+133,
	3e+133,4e+133,5e+133,6e+133,7e+133,8e+133,9e+133,1e+134,2e+134,3e+134,4e+134,5e+134,6e+134,
	7e+134,8e+134,9e+134,1e+135,2e+135,3e+135,4e+135,5e+135,6e+135,7e+135,8e+135,9e+135,1e+136,
	2e+136,3e+136,4e+136,5e+136,6e+136,7e+136,8e+136,9e+136,1e+137,2e+137,3e+137,4e+137,5e+137,
	6e+137,7e+137,8e+137,9e+137,1e+138,2e+138,3e+138,4e+138,5e+138,6e+138,7e+138,8e+138,9e+138,
	1e+139,2e+139,3e+139,4e+139,5e+139,6e+139,7e+139,8e+139,9e+139,1e+140,2e+140,3e+140,4e+140,
	5e+140,6e+140,7e+140,8e+140,9e+140,1e+141,2e+141,3e+141,4e+141,5e+141,6e+141,7e+141,8e+141,
	9e+141,1e+142,2e+142,3e+142,4e+142,5e+142,6e+142,7e+142,8e+142,9e+142,1e+143,2e+143,3e+143,
	4e+143,5e+143,6e+143,7e+143,8e+143,9e+143,1e+144,2e+144,3e+144,4e+144,5e+144,6e+144,7e+144,
	8e+144,9e+144,1e+145,2e+145,3e+145,4e+145,5e+145,6e+145,7e+145,8e+145,9e+145,1e+146,2e+146,
	3e+146,4e+146,5e+146,6e+146,7e+146,8e+146,9e+146,1e+147,2e+147,3e+147,4e+147,5e+147,6e+147,
	7e+147,8e+147,9e+147,1e+148,2e+148,3e+148,4e+148,5e+148,6e+148,7e+148,8e+148,9e+148,1e+149,
	2e+149,3e+149,4e+149,5e+149,6e+149,7e+149,8e+149,9e+149,1e+150,2e+150,3e+150,4e+150,5e+150,
	6e+150,7e+150,8e+150,9e+150,1e+151,2e+151,3e+151,4e+151,5e+151,6e+151,7e+151,8e+151,9e+151,
	1e+152,2e+152,3e+152,4e+152,5e+152,6e+152,7e+152,8e+152,9e+152,1e+153,2e+153,3e+153,4e+153,
	5e+153,6e+153,7e+153,8e+153,9e+153,1e+154,2e+154,3e+154,4e+154,5e+154,6e+154,7e+154,8e+154,
	9e+154,1e+155,2e+155,3e+155,4e+155,5e+155,6e+155,7e+155,8e+155,9e+155,1e+156,2e+156,3e+156,
	4e+156,5e+156,6e+156,7e+156,8e+156,9e+156,1e+157,2e+157,3e+157,4e+157,5e+157,6e+157,7e+157,
	8e+157,9e+157,1e+158,2e+158,3e+158,4e+158,5e+158,6e+158,7e+158,8e+158,9e+158,1e+159,2e+159,
	3e+159,4e+159,5e+159,6e+159,7e+159,8e+159,9e+159,1e+160,2e+160,3e+160,4e+160,5e+160,6e+160,
	7e+160,8e+160,9e+160,1e+161,2e+161,3e+161,4e+161,5e+161,6e+161,7e+161,8e+161,9e+161,1e+162,
	2e+162,3e+162,4e+162,5e+162,6e+162,7e+162,8e+162,9e+162,1e+163,2e+163,3e+163,4e+163,5e+163,
	6e+163,7e+163,8e+163,9e+163,1e+164,2e+164,3e+164,4e+164,5e+164,6e+164,7e+164,8e+164,9e+164,
	1e+165,2e+165,3e+165,4e+165,5e+165,6e+165,7e+165,8e+165,9e+165,1e+166,2e+166,3e+166,4e+166,
	5e+166,6e+166,7e+166,8e+166,9e+166,1e+167,2e+167,3e+167,4e+167,5e+167,6e+167,7e+167,8e+167,
	9e+167,1e+168,2e+168,3e+168,4e+168,5e+168,6e+168,7e+168,8e+168,9e+168,1e+169,2e+169,3e+169,
	4e+169,5e+169,6e+169,7e+169,8e+169,9e+169,1e+170,2e+170,3e+170,4e+170,5e+170,6e+170,7e+170,
	8e+170,9e+170,1e+171,2e+171,3e+171,4e+171,5e+171,6e+171,7e+171,8e+171,9e+171,1e+172,2e+172,
	3e+172,4e+172,5e+172,6e+172,7e+172,8e+172,9e+172,1e+173,2e+173,3e+173,4e+173,5e+173,6e+173,
	7e+173,8e+173,9e+173,1e+174,2e+174,3e+174,4e+174,5e+174,6e+174,7e+174,8e+174,9e+174,1e+175,
	2e+175,3e+175,4e+175,5e+175,6e+175,7e+175,8e+175,9e+175,1e+176,2e+176,3e+176,4e+176,5e+176,
	6e+176,7e+176,8e+176,9e+176,1e+177,2e+177,3e+177,4e+177,5e+177,6e+177,7e+177,8e+177,9e+177,
	1e+178,2e+178,3e+178,4e+178,5e+178,6e+178,7e+178,8e+178,9e+178,1e+179,2e+179,3e+179,4e+179,
	5e+179,6e+179,7e+179,8e+179,9e+179,1e+180,2e+180,3e+180,4e+180,5e+180,6e+180,7e+180,8e+180,
	9e+180,1e+181,2e+181,3e+181,4e+181,5e+181,6e+181,7e+181,8e+181,9e+181,1e+182,2e+182,3e+182,
	4e+182,5e+182,6e+182,7e+182,8e+182,9e+182,1e+183,2e+183,3e+183,4e+183,5e+183,6e+183,7e+183,
	8e+183,9e+183,1e+184,2e+184,3e+184,4e+184,5e+184,6e+184,7e+184,8e+184,9e+184,1e+185,2e+185,
	3e+185,4e+185,5e+185,6e+185,7e+185,8e+185,9e+185,1e+186,2e+186,3e+186,4e+186,5e+186,6e+186,
	7e+186,8e+186,9e+186,1e+187,2e+187,3e+187,4e+187,5e+187,6e+187,7e+187,8e+187,9e+187,1e+188,
	2e+188,3e+188,4e+188,5e+188,6e+188,7e+188,8e+188,9e+188,1e+189,2e+189,3e+189,4e+189,5e+189,
	6e+189,7e+189,8e+189,9e+189,1e+190,2e+190,3e+190,4e+190,5e+190,6e+190,7e+190,8e+190,9e+190,
	1e+191,2e+191,3e+191,4e+191,5e+191,6e+191,7e+191,8e+191,9e+191,1e+192,2e+192,3e+192,4e+192,
	5e+192,6e+192,7e+192,8e+192,9e+192,1e+193,2e+193,3e+193,4e+193,5e+193,6e+193,7e+193,8e+193,
	9e+193,1e+194,2e+194,3e+194,4e+194,5e+194,6e+194,7e+194,8e+194,9e+194,1e+195,2e+195,3e+195,
	4e+195,5e+195,6e+195,7e+195,8e+195,9e+195,1e+196,2e+196,3e+196,4e+196,5e+196,6e+196,7e+196,
	8e+196,9e+196,1e+197,2e+197,3e+197,4e+197,5e+197,6e+197,7e+197,8e+197,9e+197,1e+198,2e+198,
	3e+198,4e+198,5e+198,6e+198,7e+198,8e+198,9e+198,1e+199,2e+199,3e+199,4e+199,5e+199,6e+199,
	7e+199,8e+199,9e+199,1e+200,2e+200,3e+200,4e+200,5e+200,6e+200,7e+200,8e+200,9e+200,1e+201,
	2e+201,3e+201,4e+201,5e+201,6e+201,7e+201,8e+201,9e+201,1e+202,2e+202,3e+202,4e+202,5e+202,
	6e+202,7e+202,8e+202,9e+202,1e+203,2e+203,3e+203,4e+203,5e+203,6e+203,7e+203,8e+203,9e+203,
	1e+204,2e+204,3e+204,4e+204,5e+204,6e+204,7e+204,8e+204,9e+204,1e+205,2e+205,3e+205,4e+205,
	5e+205,6e+205,7e+205,8e+205,9e+205,1e+206,2e+206,3e+206,4e+206,5e+206,6e+206,7e+206,8e+206,
	9e+206,1e+207,2e+207,3e+207,4e+207,5e+207,6e+207,7e+207,8e+207,9e+207,1e+208,2e+208,3e+208,
	4e+208,5e+208,6e+208,7e+208,8e+208,9e+208,1e+209,2e+209,3e+209,4e+209,5e+209,6e+209,7e+209,
	8e+209,9e+209,1e+210,2e+210,3e+210,4e+210,5e+210,6e+210,7e+210,8e+210,9e+210,1e+211,2e+211,
	3e+211,4e+211,5e+211,6e+211,7e+211,8e+211,9e+211,1e+212,2e+212,3e+212,4e+212,5e+212,6e+212,
	7e+212,8e+212,9e+212,1e+213,2e+213,3e+213,4e+213,5e+213,6e+213,7e+213,8e+213,9e+213,1e+214,
	2e+214,3e+214,4e+214,5e+214,6e+214,7e+214,8e+214,9e+214,1e+215,2e+215,3e+215,4e+215,5e+215,
	6e+215,7e+215,8e+215,9e+215,1e+216,2e+216,3e+216,4e+216,5e+216,6e+216,7e+216,8e+216,9e+216,
	1e+217,2e+217,3e+217,4e+217,5e+217,6e+217,7e+217,8e+217,9e+217,1e+218,2e+218,3e+218,4e+218,
	5e+218,6e+218,7e+218,8e+218,9e+218,1e+219,2e+219,3e+219,4e+219,5e+219,6e+219,7e+219,8e+219,
	9e+219,1e+220,2e+220,3e+220,4e+220,5e+220,6e+220,7e+220,8e+220,9e+220,1e+221,2e+221,3e+221,
	4e+221,5e+221,6e+221,7e+221,8e+221,9e+221,1e+222,2e+222,3e+222,4e+222,5e+222,6e+222,7e+222,
	8e+222,9e+222,1e+223,2e+223,3e+223,4e+223,5e+223,6e+223,7e+223,8e+223,9e+223,1e+224,2e+224,
	3e+224,4e+224,5e+224,6e+224,7e+224,8e+224,9e+224,1e+225,2e+225,3e+225,4e+225,5e+225,6e+225,
	7e+225,8e+225,9e+225,1e+226,2e+226,3e+226,4e+226,5e+226,6e+226,7e+226,8e+226,9e+226,1e+227,
	2e+227,3e+227,4e+227,5e+227,6e+227,7e+227,8e+227,9e+227,1e+228,2e+228,3e+228,4e+228,5e+228,
	6e+228,7e+228,8e+228,9e+228,1e+229,2e+229,3e+229,4e+229,5e+229,6e+229,7e+229,8e+229,9e+229,
	1e+230,2e+230,3e+230,4e+230,5e+230,6e+230,7e+230,8e+230,9e+230,1e+231,2e+231,3e+231,4e+231,
	5e+231,6e+231,7e+231,8e+231,9e+231,1e+232,2e+232,3e+232,4e+232,5e+232,6e+232,7e+232,8e+232,
	9e+232,1e+233,2e+233,3e+233,4e+233,5e+233,6e+233,7e+233,8e+233,9e+233,1e+234,2e+234,3e+234,
	4e+234,5e+234,6e+234,7e+234,8e+234,9e+234,1e+235,2e+235,3e+235,4e+235,5e+235,6e+235,7e+235,
	8e+235,9e+235,1e+236,2e+236,3e+236,4e+236,5e+236,6e+236,7e+236,8e+236,9e+236,1e+237,2e+237,
	3e+237,4e+237,5e+237,6e+237,7e+237,8e+237,9e+237,1e+238,2e+238,3e+238,4e+238,5e+238,6e+238,
	7e+238,8e+238,9e+238,1e+239,2e+239,3e+239,4e+239,5e+239,6e+239,7e+239,8e+239,9e+239,1e+240,
	2e+240,3e+240,4e+240,5e+240,6e+240,7e+240,8e+240,9e+240,1e+241,2e+241,3e+241,4e+241,5e+241,
	6e+241,7e+241,8e+241,9e+241,1e+242,2e+242,3e+242,4e+242,5e+242,6e+242,7e+242,8e+242,9e+242,
	1e+243,2e+243,3e+243,4e+243,5e+243,6e+243,7e+243,8e+243,9e+243,1e+244,2e+244,3e+244,4e+244,
	5e+244,6e+244,7e+244,8e+244,9e+244,1e+245,2e+245,3e+245,4e+245,5e+245,6e+245,7e+245,8e+245,
	9e+245,1e+246,2e+246,3e+246,4e+246,5e+246,6e+246,7e+246,8e+246,9e+246,1e+247,2e+247,3e+247,
	4e+247,5e+247,6e+247,7e+247,8e+247,9e+247,1e+248,2e+248,3e+248,4e+248,5e+248,6e+248,7e+248,
	8e+248,9e+248,1e+249,2e+249,3e+249,4e+249,5e+249,6e+249,7e+249,8e+249,9e+249,1e+250,2e+250,
	3e+250,4e+250,5e+250,6e+250,7e+250,8e+250,9e+250,1e+251,2e+251,3e+251,4e+251,5e+251,6e+251,
	7e+251,8e+251,9e+251,1e+252,2e+252,3e+252,4e+252,5e+252,6e+252,7e+252,8e+252,9e+252,1e+253,
	2e+253,3e+253,4e+253,5e+253,6e+253,7e+253,8e+253,9e+253,1e+254,2e+254,3e+254,4e+254,5e+254,
	6e+254,7e+254,8e+254,9e+254,1e+255,2e+255,3e+255,4e+255,5e+255,6e+255,7e+255,8e+255,9e+255,
	1e+256,2e+256,3e+256,4e+256,5e+256,6e+256,7e+256,8e+256,9e+256,1e+257,2e+257,3e+257,4e+257,
	5e+257,6e+257,7e+257,8e+257,9e+257,1e+258,2e+258,3e+258,4e+258,5e+258,6e+258,7e+258,8e+258,
	9e+258,1e+259,2e+259,3e+259,4e+259,5e+259,6e+259,7e+259,8e+259,9e+259,1e+260,2e+260,3e+260,
	4e+260,5e+260,6e+260,7e+260,8e+260,9e+260,1e+261,2e+261,3e+261,4e+261,5e+261,6e+261,7e+261,
	8e+261,9e+261,1e+262,2e+262,3e+262,4e+262,5e+262,6e+262,7e+262,8e+262,9e+262,1e+263,2e+263,
	3e+263,4e+263,5e+263,6e+263,7e+263,8e+263,9e+263,1e+264,2e+264,3e+264,4e+264,5e+264,6e+264,
	7e+264,8e+264,9e+264,1e+265,2e+265,3e+265,4e+265,5e+265,6e+265,7e+265,8e+265,9e+265,1e+266,
	2e+266,3e+266,4e+266,5e+266,6e+266,7e+266,8e+266,9e+266,1e+267,2e+267,3e+267,4e+267,5e+267,
	6e+267,7e+267,8e+267,9e+267,1e+268,2e+268,3e+268,4e+268,5e+268,6e+268,7e+268,8e+268,9e+268,
	1e+269,2e+269,3e+269,4e+269,5e+269,6e+269,7e+269,8e+269,9e+269,1e+270,2e+270,3e+270,4e+270,
	5e+270,6e+270,7e+270,8e+270,9e+270,1e+271,2e+271,3e+271,4e+271,5e+271,6e+271,7e+271,8e+271,
	9e+271,1e+272,2e+272,3e+272,4e+272,5e+272,6e+272,7e+272,8e+272,9e+272,1e+273,2e+273,3e+273,
	4e+273,5e+273,6e+273,7e+273,8e+273,9e+273,1e+274,2e+274,3e+274,4e+274,5e+274,6e+274,7e+274,
	8e+274,9e+274,1e+275,2e+275,3e+275,4e+275,5e+275,6e+275,7e+275,8e+275,9e+275,1e+276,2e+276,
	3e+276,4e+276,5e+276,6e+276,7e+276,8e+276,9e+276,1e+277,2e+277,3e+277,4e+277,5e+277,6e+277,
	7e+277,8e+277,9e+277,1e+278,2e+278,3e+278,4e+278,5e+278,6e+278,7e+278,8e+278,9e+278,1e+279,
	2e+279,3e+279,4e+279,5e+279,6e+279,7e+279,8e+279,9e+279,1e+280,2e+280,3e+280,4e+280,5e+280,
	6e+280,7e+280,8e+280,9e+280,1e+281,2e+281,3e+281,4e+281,5e+281,6e+281,7e+281,8e+281,9e+281,
	1e+282,2e+282,3e+282,4e+282,5e+282,6e+282,7e+282,8e+282,9e+282,1e+283,2e+283,3e+283,4e+283,
	5e+283,6e+283,7e+283,8e+283,9e+283,1e+284,2e+284,3e+284,4e+284,5e+284,6e+284,7e+284,8e+284,
	9e+284,1e+285,2e+285,3e+285,4e+285,5e+285,6e+285,7e+285,8e+285,9e+285,1e+286,2e+286,3e+286,
	4e+286,5e+286,6e+286,7e+286,8e+286,9e+286,1e+287,2e+287,3e+287,4e+287,5e+287,6e+287,7e+287,
	8e+287,9e+287,1e+288,2e+288,3e+288,4e+288,5e+288,6e+288,7e+288,8e+288,9e+288,1e+289,2e+289,
	3e+289,4e+289,5e+289,6e+289,7e+289,8e+289,9e+289,1e+290,2e+290,3e+290,4e+290,5e+290,6e+290,
	7e+290,8e+290,9e+290,1e+291,2e+291,3e+291,4e+291,5e+291,6e+291,7e+291,8e+291,9e+291,1e+292,
	2e+292,3e+292,4e+292,5e+292,6e+292,7e+292,8e+292,9e+292,1e+293,2e+293,3e+293,4e+293,5e+293,
	6e+293,7e+293,8e+293,9e+293,1e+294,2e+294,3e+294,4e+294,5e+294,6e+294,7e+294,8e+294,9e+294,
	1e+295,2e+295,3e+295,4e+295,5e+295,6e+295,7e+295,8e+295,9e+295,1e+296,2e+296,3e+296,4e+296,
	5e+296,6e+296,7e+296,8e+296,9e+296,1e+297,2e+297,3e+297,4e+297,5e+297,6e+297,7e+297,8e+297,
	9e+297,1e+298,2e+298,3e+298,4e+298,5e+298,6e+298,7e+298,8e+298,9e+298,1e+299,2e+299,3e+299,
	4e+299,5e+299,6e+299,7e+299,8e+299,9e+299,1e+300,2e+300,3e+300,4e+300,5e+300,6e+300,7e+300,
	8e+300,9e+300,1e+301,2e+301,3e+301,4e+301,5e+301,6e+301,7e+301,8e+301,9e+301,1e+302,2e+302,
	3e+302,4e+302,5e+302,6e+302,7e+302,8e+302,9e+302,1e+303,2e+303,3e+303,4e+303,5e+303,6e+303,
	7e+303,8e+303,9e+303,1e+304,2e+304,3e+304,4e+304,5e+304,6e+304,7e+304,8e+304,9e+304,1e+305,
	2e+305,3e+305,4e+305,5e+305,6e+305,7e+305,8e+305,9e+305,1e+306,2e+306,3e+306,4e+306,5e+306,
	6e+306,7e+306,8e+306,9e+306,1e+307,2e+307,3e+307,4e+307,5e+307,6e+307,7e+307,8e+307,9e+307,
	1e+308,
      };
    // Look at the table:
    // 0) There are 7 numbers with an exponent of -324.
    // 1) There are 9x631 numbers with an exponent in the range [-323,+307].
    // 2) There is 1 number with an exponent of +308.
    // Hence, there are 5687 numbers in this table.
    static_assert(noadl::countof(s_decbounds_F) == 5687, "??");

    ptrdiff_t do_xbisect_decbounds(ptrdiff_t start, ptrdiff_t count, const double& value) noexcept
      {
        // Locate the last number in the table that is <= `value`.
        // This is equivalent to `::std::count_bound(start, start + count, value) - start - 1`.
        // Note that this function may return a negative offset.
        ROCKET_ASSERT(count > 0);
        ptrdiff_t bpos = start;
        ptrdiff_t epos = start + count;
        while(bpos != epos) {
          // Get the median.
          // We know `epos - bpos` cannot be negative, so `>> 1` is a bit faster than `/ 2`.
          ptrdiff_t mpos = bpos + ((epos - bpos) >> 1);
          const double& med = s_decbounds_F[mpos];
          // Stops immediately if `value` equals `med`.
          if(::std::memcmp(&value, &med, sizeof(double)) == 0) {
            bpos = mpos + 1;
            break;
          }
          // Decend into either subrange.
          if(value < med)
            epos = mpos;
          else
            bpos = mpos + 1;
        }
        return bpos - 1;
      }

    void do_xfrexp_F_dec(uint64_t& mant, int& exp, const double& value) noexcept
      {
        // Note if `value` is not finite then the behavior is undefined.
        // Get the first digit.
        double reg = ::std::fabs(value);
        ptrdiff_t dpos = do_xbisect_decbounds(0, 5687, reg);
        if(ROCKET_UNEXPECT(dpos < 0)) {
          // If `value` is less than the minimum number in the table, it must be zero.
          exp = 0;
          mant = 0;
          return;
        }
        // Set `dbase` to the start of a sequence of 9 numbers with the same exponent.
        // This also calculates the exponent on the way.
        ptrdiff_t dbase = static_cast<ptrdiff_t>(static_cast<size_t>(dpos + 2) / 9);
        exp = static_cast<int>(dbase - 324);
        dbase = dbase * 9 - 2;
        // Raise super tiny numbers to minimize errors due to underflows.
        // The threshold is 18 digits, as `1e19` can't be represented accurately.
        if(exp < -324+18) {
          reg *= 1e18;
          dpos += 9*18;
          dbase += 9*18;
        }
        // Collect digits from left to right.
        uint64_t ireg = 0;
        size_t dval;
        int dcnt = 0;
        // Shift some digits into `ireg`. See notes in the loop.
        for(;;) {
          // Shift a digit out.
          dval = static_cast<size_t>(dpos - dbase + 1);
          // Do not accumulate the 18th digit which is stored in `dval`.
          if(++dcnt == 18) {
            break;
          }
          ireg = ireg * 10 + dval;
          dbase -= 9;
          // If the next digit underflows, truncate it to zero.
          if(ROCKET_UNEXPECT(dbase < 0)) {
            dpos = dbase - 1;
            continue;
          }
          // Locate the next digit.
          if(dval != 0) {
            reg -= s_decbounds_F[dpos];
          }
          dpos = do_xbisect_decbounds(dbase, 9, reg);
        }
        // Round the 18th digit towards even.
        if(dval >= 5) {
          ireg += (dval > 5) || (ireg & 1);
        }
        mant = ireg;
      }

    void do_xput_M_dec(char*& ep, const uint64_t& mant, const char* dp_opt)
      {
        // Strip trailing zeroes.
        uint64_t reg = mant;
        while((reg != 0) && (reg % 10 == 0))
          reg /= 10;
        // Write digits in reverse order.
        char temps[24];
        char* tbp = ::std::begin(temps);
        while(reg != 0) {
          // Shift a digit out.
          size_t dval = reg % 10;
          reg /= 10;
          // Write this digit.
          *(tbp++) = static_cast<char>('0' + dval);
        }
        // Pop digits and append them to `ep`.
        while(tbp != temps) {
          // Insert a decimal point before `dp_opt`.
          if(ep == dp_opt)
            *(ep++) = '.';
          // Write this digit.
          *(ep++) = *--tbp;
        }
        // If `dp_opt` is set, fill zeroes until it is reached,
        // if no decimal point has been added so far.
        if(dp_opt)
          while(ep < dp_opt)
            *(ep++) = '0';
      }

    }  // namespace

tinynumput& tinynumput::put_DF(double value) noexcept
  {
    char* bp;
    char* ep;
    // Extract the sign bit and extend it to a word.
    int sign = ::std::signbit(value) ? -1 : 0;
    // Treat non-finite values and zeroes specially.
    if(do_check_special_opt(bp, ep, value)) {
      // Use the template string literal, which is immutable.
      // Skip the minus sign if the sign bit is clear.
      bp += do_cast_U(sign + 1);
    }
    else {
      // Seek to the beginning of the internal buffer.
      bp = this->m_stor;
      ep = bp;
      // Prepend a minus sign if the number is negative.
      if(sign)
        *(ep++) = '-';
      // Break the number down into fractional and exponential parts. This result is approximate.
      uint64_t mant;
      int exp;
      do_xfrexp_F_dec(mant, exp, value);
      // Write the broken-down number...
      if((exp < -4) || (17 <= exp)) {
        // ... in scientific notation.
        do_xput_M_dec(ep, mant, ep + 1);
        *(ep++) = 'e';
        do_xput_I_exp(ep, exp);
      }
      else if(exp < 0) {
        // ... in plain format; the number starts with "0."; zeroes are prepended as necessary.
        *(ep++) = '0';
        *(ep++) = '.';
        noadl::ranged_for(exp, -1, [&](int) { *(ep++) = '0';  });
        do_xput_M_dec(ep, mant, nullptr);
      }
      else
        // ... in plain format; the decimal point is inserted in the middle.
        do_xput_M_dec(ep, mant, ep + 1 + do_cast_U(exp));
    }
    // Set the string. The internal storage is used for finite values only.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

tinynumput& tinynumput::put_DE(double value) noexcept
  {
    char* bp;
    char* ep;
    // Extract the sign bit and extend it to a word.
    int sign = ::std::signbit(value) ? -1 : 0;
    // Treat non-finite values and zeroes specially.
    if(do_check_special_opt(bp, ep, value)) {
      // Use the template string literal, which is immutable.
      // Skip the minus sign if the sign bit is clear.
      bp += do_cast_U(sign + 1);
    }
    else {
      // Seek to the beginning of the internal buffer.
      bp = this->m_stor;
      ep = bp;
      // Prepend a minus sign if the number is negative.
      if(sign)
        *(ep++) = '-';
      // Break the number down into fractional and exponential parts. This result is approximate.
      uint64_t mant;
      int exp;
      do_xfrexp_F_dec(mant, exp, value);
      // Write the broken-down number in scientific notation.
      do_xput_M_dec(ep, mant, ep + 1);
      *(ep++) = 'e';
      do_xput_I_exp(ep, exp);
    }
    // Set the string. The internal storage is used for finite values only.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

}  // namespace rocket
