// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "ascii_numput.hpp"
#include "assert.hpp"
#include <cmath>

namespace rocket {

ascii_numput& ascii_numput::put_TB(bool value) noexcept
  {
    this->clear();
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

ascii_numput& ascii_numput::put_XP(const void* value) noexcept
  {
    this->clear();
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
    // Append a null terminator.
    *ep = 0;
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

    template<typename valueT, ROCKET_ENABLE_IF(is_unsigned<valueT>::value)>
        void do_xput_U_bkwd(char*& bp, const valueT& value, uint8_t radix, size_t precision)
      {
        char* stop = bp - precision;
        // Write digits backwards.
        valueT reg = value;
        while(reg != 0) {
          // Shift a digit out.
          size_t dval = static_cast<size_t>(reg % radix);
          reg /= radix;
          // Write this digit.
          *(--bp) = "0123456789ABCDEF"[dval];
        }
        // Pad the string to at least the precision requested.
        while(bp > stop)
          *(--bp) = '0';
      }

    }  // namespace

ascii_numput& ascii_numput::put_BU(uint64_t value, size_t precision) noexcept
  {
    this->clear();
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Append a null terminator.
    *bp = 0;
    // Write digits backwards.
    do_xput_U_bkwd(bp, value, 2, precision);
    // Prepend the binary prefix.
    *(--bp) = 'b';
    *(--bp) = '0';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

ascii_numput& ascii_numput::put_XU(uint64_t value, size_t precision) noexcept
  {
    this->clear();
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Append a null terminator.
    *bp = 0;
    // Write digits backwards.
    do_xput_U_bkwd(bp, value, 16, precision);
    // Prepend the hexadecimal prefix.
    *(--bp) = 'x';
    *(--bp) = '0';
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

ascii_numput& ascii_numput::put_DU(uint64_t value, size_t precision) noexcept
  {
    this->clear();
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Append a null terminator.
    *bp = 0;
    // Write digits backwards.
    do_xput_U_bkwd(bp, value, 10, precision);
    // Set the string, which resides in the internal storage.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

ascii_numput& ascii_numput::put_BI(int64_t value, size_t precision) noexcept
  {
    this->clear();
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Append a null terminator.
    *bp = 0;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint64_t sign = do_cast_U(value >> 63);
    // Write digits backwards using its absolute value without causing overflows.
    do_xput_U_bkwd(bp, (do_cast_U(value) ^ sign) - sign, 2, precision);
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

ascii_numput& ascii_numput::put_XI(int64_t value, size_t precision) noexcept
  {
    this->clear();
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Append a null terminator.
    *bp = 0;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint64_t sign = do_cast_U(value >> 63);
    // Write digits backwards using its absolute value without causing overflows.
    do_xput_U_bkwd(bp, (do_cast_U(value) ^ sign) - sign, 16, precision);
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

ascii_numput& ascii_numput::put_DI(int64_t value, size_t precision) noexcept
  {
    this->clear();
    char* bp = this->m_stor + M;
    char* ep = bp;
    // Append a null terminator.
    *bp = 0;
    // Extend the sign bit to a word, assuming arithmetic shift.
    uint64_t sign = do_cast_U(value >> 63);
    // Write digits backwards using its absolute value without causing overflows.
    do_xput_U_bkwd(bp, (do_cast_U(value) ^ sign) - sign, 10, precision);
    // If the number is negative, prepend a minus sign.
    if(sign)
      *(--bp) = '-';
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
        char* tbp = end(temps);
        do_xput_U_bkwd(tbp, do_cast_U(::std::abs(exp)), 10, 2);
        // Append the exponent.
        noadl::ranged_for(tbp, end(temps), [&](const char* p) { *(ep++) = *p;  });
        return ep;
      }

    }  // namespace

ascii_numput& ascii_numput::put_BF(double value) noexcept
  {
    this->clear();
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
      else {
        // ... in plain format; the decimal point is inserted in the middle.
        do_xput_M_bin(ep, mant, ep + 1 + do_cast_U(exp));
      }
      // Append a null terminator.
      *ep = 0;
    }
    // Set the string. The internal storage is used for finite values only.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

ascii_numput& ascii_numput::put_BE(double value) noexcept
  {
    this->clear();
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
      // Append a null terminator.
      *ep = 0;
    }
    // Set the string. The internal storage is used for finite values only.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

ascii_numput& ascii_numput::put_XF(double value) noexcept
  {
    this->clear();
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
      else {
        // ... in plain format; the decimal point is inserted in the middle.
        do_xput_M_hex(ep, mant, ep + 1 + do_cast_U(exp));
      }
      // Append a null terminator.
      *ep = 0;
    }
    // Set the string. The internal storage is used for finite values only.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

ascii_numput& ascii_numput::put_XE(double value) noexcept
  {
    this->clear();
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
      // Append a null terminator.
      *ep = 0;
    }
    // Set the string. The internal storage is used for finite values only.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

    namespace {

#if 0
/* This program is used to generate the bound table for decimal
 * numbers. Each bound value from `3e-324` to `1e+308` is split
 * into two parts for accuracy.
 *
 * Compile with:
 *   gcc -std=c99 -W{all,extra,pedantic,{sign-,}conversion}  \
 *       table.c -lquadmath -lm
 */

#include <quadmath.h>
#include <stdio.h>

void do_print_one(int m, int e)
  {
    __float128 value, frac;
    int bexp, uoff;
    long long mant;

    if(e <= -324 && m < 3) {
      // The value is too small for `double`.
      printf("\t{ %1$23s,  %1$24s }, ", "0");
    }
    else if(e >= +308 && m > 1) {
      // The value is too large for `double`.
      printf("\t{ %1$23s,  %1$24s }, ", "HUGE_VAL");
    }
    else {
      // Raise the value `m` to the power `e` of ten.
      value = m * powq(10, e);
      // Break it down into the fraction and exponent.
      frac = frexpq(value, &bexp);
      // Truncate the fraction to 53 bits. Do not round it.
      frac = ldexpq(frac, 53);
      mant = (long long)frac;
      bexp = bexp - 1;
      // Print the higher part in fixed-point format.
      printf("\t{ 0x1.%.13llXp%+.4d, ",
             mant & 0xFFFFFFFFFFFFF, bexp);
      // Subtract the higher part to get the lower part.
      frac = ldexpq(frac - mant, 53);
      mant = (long long)frac;
      bexp = bexp - 53;
      // Truncate it to zero in case of underflows.
      // Note: The purpose of this operation is solely to
      //       silence compilers that warn about truncation.
      uoff = -1023 - bexp;
      if(uoff >= 0 && (uoff > 63 || mant >> uoff == 0))
        mant = 0;
      // Print the lower part in fixed-point format.
      printf(" 0x0.%.14llXp%+.4d }, ",
             (mant << 3) & 0xFFFFFFFFFFFFF8, bexp + 1);
    }
    // Print some comments.
    printf("  // %d.0e%+.4d\n", m, e);
  }

int main(void)
  {
    int m, e;

    for(e = -324; e <= +308; ++e) {
      for(m = 1; m <= 9; ++m)
        do_print_one(m, e);
    }
    return 0;
  }
#endif  // 0

    // These are generated data. Do not edit by hand!
    constexpr double s_decbounds_F[][2] =
      {
        {                       0,                         0 },   // 1.0e-0324
        {                       0,                         0 },   // 2.0e-0324
        { 0x1.36E3CDEF8CB55p-1075,  0x0.00000000000000p-1127 },   // 3.0e-0324
        { 0x1.9E851294BB9C6p-1075,  0x0.00000000000000p-1127 },   // 4.0e-0324
        { 0x1.03132B9CF541Cp-1074,  0x0.00000000000000p-1126 },   // 5.0e-0324
        { 0x1.36E3CDEF8CB55p-1074,  0x0.00000000000000p-1126 },   // 6.0e-0324
        { 0x1.6AB470422428Dp-1074,  0x0.00000000000000p-1126 },   // 7.0e-0324
        { 0x1.9E851294BB9C6p-1074,  0x0.00000000000000p-1126 },   // 8.0e-0324
        { 0x1.D255B4E7530FFp-1074,  0x0.00000000000000p-1126 },   // 9.0e-0324
        { 0x1.03132B9CF541Cp-1073,  0x0.00000000000000p-1125 },   // 1.0e-0323
        { 0x1.03132B9CF541Cp-1072,  0x0.00000000000000p-1124 },   // 2.0e-0323
        { 0x1.849CC16B6FE2Ap-1072,  0x0.00000000000000p-1124 },   // 3.0e-0323
        { 0x1.03132B9CF541Cp-1071,  0x0.00000000000000p-1123 },   // 4.0e-0323
        { 0x1.43D7F68432923p-1071,  0x0.00000000000000p-1123 },   // 5.0e-0323
        { 0x1.849CC16B6FE2Ap-1071,  0x0.00000000000000p-1123 },   // 6.0e-0323
        { 0x1.C5618C52AD331p-1071,  0x0.00000000000000p-1123 },   // 7.0e-0323
        { 0x1.03132B9CF541Cp-1070,  0x0.00000000000000p-1122 },   // 8.0e-0323
        { 0x1.2375911093E9Fp-1070,  0x0.00000000000000p-1122 },   // 9.0e-0323
        { 0x1.43D7F68432923p-1070,  0x0.00000000000000p-1122 },   // 1.0e-0322
        { 0x1.43D7F68432923p-1069,  0x0.00000000000000p-1121 },   // 2.0e-0322
        { 0x1.E5C3F1C64BDB4p-1069,  0x0.00000000000000p-1121 },   // 3.0e-0322
        { 0x1.43D7F68432923p-1068,  0x0.00000000000000p-1120 },   // 4.0e-0322
        { 0x1.94CDF4253F36Cp-1068,  0x0.00000000000000p-1120 },   // 5.0e-0322
        { 0x1.E5C3F1C64BDB4p-1068,  0x0.00000000000000p-1120 },   // 6.0e-0322
        { 0x1.1B5CF7B3AC3FEp-1067,  0x0.00000000000000p-1119 },   // 7.0e-0322
        { 0x1.43D7F68432923p-1067,  0x0.00000000000000p-1119 },   // 8.0e-0322
        { 0x1.6C52F554B8E47p-1067,  0x0.00000000000000p-1119 },   // 9.0e-0322
        { 0x1.94CDF4253F36Cp-1067,  0x0.00000000000000p-1119 },   // 1.0e-0321
        { 0x1.94CDF4253F36Cp-1066,  0x0.00000000000000p-1118 },   // 2.0e-0321
        { 0x1.2F9A771BEF691p-1065,  0x0.00000000000000p-1117 },   // 3.0e-0321
        { 0x1.94CDF4253F36Cp-1065,  0x0.00000000000000p-1117 },   // 4.0e-0321
        { 0x1.FA01712E8F047p-1065,  0x0.00000000000000p-1117 },   // 5.0e-0321
        { 0x1.2F9A771BEF691p-1064,  0x0.00000000000000p-1116 },   // 6.0e-0321
        { 0x1.623435A0974FEp-1064,  0x0.00000000000000p-1116 },   // 7.0e-0321
        { 0x1.94CDF4253F36Cp-1064,  0x0.00000000000000p-1116 },   // 8.0e-0321
        { 0x1.C767B2A9E71D9p-1064,  0x0.00000000000000p-1116 },   // 9.0e-0321
        { 0x1.FA01712E8F047p-1064,  0x0.00000000000000p-1116 },   // 1.0e-0320
        { 0x1.FA01712E8F047p-1063,  0x0.00000000000000p-1115 },   // 2.0e-0320
        { 0x1.7B8114E2EB435p-1062,  0x0.00000000000000p-1114 },   // 3.0e-0320
        { 0x1.FA01712E8F047p-1062,  0x0.00000000000000p-1114 },   // 4.0e-0320
        { 0x1.3C40E6BD1962Cp-1061,  0x0.00000000000000p-1113 },   // 5.0e-0320
        { 0x1.7B8114E2EB435p-1061,  0x0.00000000000000p-1113 },   // 6.0e-0320
        { 0x1.BAC14308BD23Ep-1061,  0x0.00000000000000p-1113 },   // 7.0e-0320
        { 0x1.FA01712E8F047p-1061,  0x0.00000000000000p-1113 },   // 8.0e-0320
        { 0x1.1CA0CFAA30727p-1060,  0x0.00000000000000p-1112 },   // 9.0e-0320
        { 0x1.3C40E6BD1962Cp-1060,  0x0.00000000000000p-1112 },   // 1.0e-0319
        { 0x1.3C40E6BD1962Cp-1059,  0x0.00000000000000p-1111 },   // 2.0e-0319
        { 0x1.DA615A1BA6142p-1059,  0x0.00000000000000p-1111 },   // 3.0e-0319
        { 0x1.3C40E6BD1962Cp-1058,  0x0.00000000000000p-1110 },   // 4.0e-0319
        { 0x1.8B51206C5FBB7p-1058,  0x0.00000000000000p-1110 },   // 5.0e-0319
        { 0x1.DA615A1BA6142p-1058,  0x0.00000000000000p-1110 },   // 6.0e-0319
        { 0x1.14B8C9E576366p-1057,  0x0.00000000000000p-1109 },   // 7.0e-0319
        { 0x1.3C40E6BD1962Cp-1057,  0x0.00000000000000p-1109 },   // 8.0e-0319
        { 0x1.63C90394BC8F1p-1057,  0x0.00000000000000p-1109 },   // 9.0e-0319
        { 0x1.8B51206C5FBB7p-1057,  0x0.00000000000000p-1109 },   // 1.0e-0318
        { 0x1.8B51206C5FBB7p-1056,  0x0.00000000000000p-1108 },   // 2.0e-0318
        { 0x1.287CD85147CC9p-1055,  0x0.00000000000000p-1107 },   // 3.0e-0318
        { 0x1.8B51206C5FBB7p-1055,  0x0.00000000000000p-1107 },   // 4.0e-0318
        { 0x1.EE25688777AA5p-1055,  0x0.00000000000000p-1107 },   // 5.0e-0318
        { 0x1.287CD85147CC9p-1054,  0x0.00000000000000p-1106 },   // 6.0e-0318
        { 0x1.59E6FC5ED3C40p-1054,  0x0.00000000000000p-1106 },   // 7.0e-0318
        { 0x1.8B51206C5FBB7p-1054,  0x0.00000000000000p-1106 },   // 8.0e-0318
        { 0x1.BCBB4479EBB2Ep-1054,  0x0.00000000000000p-1106 },   // 9.0e-0318
        { 0x1.EE25688777AA5p-1054,  0x0.00000000000000p-1106 },   // 1.0e-0317
        { 0x1.EE25688777AA5p-1053,  0x0.00000000000000p-1105 },   // 2.0e-0317
        { 0x1.729C0E6599BFCp-1052,  0x0.00000000000000p-1104 },   // 3.0e-0317
        { 0x1.EE25688777AA5p-1052,  0x0.00000000000000p-1104 },   // 4.0e-0317
        { 0x1.34D76154AACA7p-1051,  0x0.00000000000000p-1103 },   // 5.0e-0317
        { 0x1.729C0E6599BFCp-1051,  0x0.00000000000000p-1103 },   // 6.0e-0317
        { 0x1.B060BB7688B50p-1051,  0x0.00000000000000p-1103 },   // 7.0e-0317
        { 0x1.EE25688777AA5p-1051,  0x0.00000000000000p-1103 },   // 8.0e-0317
        { 0x1.15F50ACC334FDp-1050,  0x0.00000000000000p-1102 },   // 9.0e-0317
        { 0x1.34D76154AACA7p-1050,  0x0.00000000000000p-1102 },   // 1.0e-0316
        { 0x1.34D76154AACA7p-1049,  0x0.00000000000000p-1101 },   // 2.0e-0316
        { 0x1.CF4311FF002FBp-1049,  0x0.00000000000000p-1101 },   // 3.0e-0316
        { 0x1.34D76154AACA7p-1048,  0x0.00000000000000p-1100 },   // 4.0e-0316
        { 0x1.820D39A9D57D1p-1048,  0x0.00000000000000p-1100 },   // 5.0e-0316
        { 0x1.CF4311FF002FBp-1048,  0x0.00000000000000p-1100 },   // 6.0e-0316
        { 0x1.0E3C752A15712p-1047,  0x0.00000000000000p-1099 },   // 7.0e-0316
        { 0x1.34D76154AACA7p-1047,  0x0.00000000000000p-1099 },   // 8.0e-0316
        { 0x1.5B724D7F4023Cp-1047,  0x0.00000000000000p-1099 },   // 9.0e-0316
        { 0x1.820D39A9D57D1p-1047,  0x0.00000000000000p-1099 },   // 1.0e-0315
        { 0x1.820D39A9D57D1p-1046,  0x0.00000000000000p-1098 },   // 2.0e-0315
        { 0x1.2189EB3F601DCp-1045,  0x0.00000000000000p-1097 },   // 3.0e-0315
        { 0x1.820D39A9D57D1p-1045,  0x0.00000000000000p-1097 },   // 4.0e-0315
        { 0x1.E29088144ADC5p-1045,  0x0.00000000000000p-1097 },   // 5.0e-0315
        { 0x1.2189EB3F601DCp-1044,  0x0.00000000000000p-1096 },   // 6.0e-0315
        { 0x1.51CB92749ACD7p-1044,  0x0.00000000000000p-1096 },   // 7.0e-0315
        { 0x1.820D39A9D57D1p-1044,  0x0.00000000000000p-1096 },   // 8.0e-0315
        { 0x1.B24EE0DF102CBp-1044,  0x0.00000000000000p-1096 },   // 9.0e-0315
        { 0x1.E29088144ADC5p-1044,  0x0.00000000000000p-1096 },   // 1.0e-0314
        { 0x1.E29088144ADC5p-1043,  0x0.00000000000000p-1095 },   // 2.0e-0314
        { 0x1.69EC660F38254p-1042,  0x0.00000000000000p-1094 },   // 3.0e-0314
        { 0x1.E29088144ADC5p-1042,  0x0.00000000000000p-1094 },   // 4.0e-0314
        { 0x1.2D9A550CAEC9Bp-1041,  0x0.00000000000000p-1093 },   // 5.0e-0314
        { 0x1.69EC660F38254p-1041,  0x0.00000000000000p-1093 },   // 6.0e-0314
        { 0x1.A63E7711C180Cp-1041,  0x0.00000000000000p-1093 },   // 7.0e-0314
        { 0x1.E29088144ADC5p-1041,  0x0.00000000000000p-1093 },   // 8.0e-0314
        { 0x1.0F714C8B6A1BFp-1040,  0x0.00000000000000p-1092 },   // 9.0e-0314
        { 0x1.2D9A550CAEC9Bp-1040,  0x0.00000000000000p-1092 },   // 1.0e-0313
        { 0x1.2D9A550CAEC9Bp-1039,  0x0.00000000000000p-1091 },   // 2.0e-0313
        { 0x1.C4677F93062E9p-1039,  0x0.00000000000000p-1091 },   // 3.0e-0313
        { 0x1.2D9A550CAEC9Bp-1038,  0x0.00000000000000p-1090 },   // 4.0e-0313
        { 0x1.7900EA4FDA7C2p-1038,  0x0.00000000000000p-1090 },   // 5.0e-0313
        { 0x1.C4677F93062E9p-1038,  0x0.00000000000000p-1090 },   // 6.0e-0313
        { 0x1.07E70A6B18F08p-1037,  0x0.00000000000000p-1089 },   // 7.0e-0313
        { 0x1.2D9A550CAEC9Bp-1037,  0x0.00000000000000p-1089 },   // 8.0e-0313
        { 0x1.534D9FAE44A2Ep-1037,  0x0.00000000000000p-1089 },   // 9.0e-0313
        { 0x1.7900EA4FDA7C2p-1037,  0x0.00000000000000p-1089 },   // 1.0e-0312
        { 0x1.7900EA4FDA7C2p-1036,  0x0.00000000000000p-1088 },   // 2.0e-0312
        { 0x1.1AC0AFBBE3DD1p-1035,  0x0.00000000000000p-1087 },   // 3.0e-0312
        { 0x1.7900EA4FDA7C2p-1035,  0x0.00000000000000p-1087 },   // 4.0e-0312
        { 0x1.D74124E3D11B2p-1035,  0x0.00000000000000p-1087 },   // 5.0e-0312
        { 0x1.1AC0AFBBE3DD1p-1034,  0x0.00000000000000p-1086 },   // 6.0e-0312
        { 0x1.49E0CD05DF2CAp-1034,  0x0.00000000000000p-1086 },   // 7.0e-0312
        { 0x1.7900EA4FDA7C2p-1034,  0x0.00000000000000p-1086 },   // 8.0e-0312
        { 0x1.A8210799D5CBAp-1034,  0x0.00000000000000p-1086 },   // 9.0e-0312
        { 0x1.D74124E3D11B2p-1034,  0x0.00000000000000p-1086 },   // 1.0e-0311
        { 0x1.D74124E3D11B2p-1033,  0x0.00000000000000p-1085 },   // 2.0e-0311
        { 0x1.6170DBAADCD46p-1032,  0x0.00000000000000p-1084 },   // 3.0e-0311
        { 0x1.D74124E3D11B2p-1032,  0x0.00000000000000p-1084 },   // 4.0e-0311
        { 0x1.2688B70E62B0Fp-1031,  0x0.00000000000000p-1083 },   // 5.0e-0311
        { 0x1.6170DBAADCD46p-1031,  0x0.00000000000000p-1083 },   // 6.0e-0311
        { 0x1.9C59004756F7Cp-1031,  0x0.00000000000000p-1083 },   // 7.0e-0311
        { 0x1.D74124E3D11B2p-1031,  0x0.00000000000000p-1083 },   // 8.0e-0311
        { 0x1.0914A4C0259F4p-1030,  0x0.00000000000000p-1082 },   // 9.0e-0311
        { 0x1.2688B70E62B0Fp-1030,  0x0.00000000000000p-1082 },   // 1.0e-0310
        { 0x1.2688B70E62B0Fp-1029,  0x0.00000000000000p-1081 },   // 2.0e-0310
        { 0x1.B9CD129594097p-1029,  0x0.00000000000000p-1081 },   // 3.0e-0310
        { 0x1.2688B70E62B0Fp-1028,  0x0.00000000000000p-1080 },   // 4.0e-0310
        { 0x1.702AE4D1FB5D3p-1028,  0x0.00000000000000p-1080 },   // 5.0e-0310
        { 0x1.B9CD129594097p-1028,  0x0.00000000000000p-1080 },   // 6.0e-0310
        { 0x1.01B7A02C965ADp-1027,  0x0.00000000000000p-1079 },   // 7.0e-0310
        { 0x1.2688B70E62B0Fp-1027,  0x0.00000000000000p-1079 },   // 8.0e-0310
        { 0x1.4B59CDF02F071p-1027,  0x0.00000000000000p-1079 },   // 9.0e-0310
        { 0x1.702AE4D1FB5D3p-1027,  0x0.00000000000000p-1079 },   // 1.0e-0309
        { 0x1.702AE4D1FB5D3p-1026,  0x0.00000000000000p-1078 },   // 2.0e-0309
        { 0x1.14202B9D7C85Ep-1025,  0x0.00000000000000p-1077 },   // 3.0e-0309
        { 0x1.702AE4D1FB5D3p-1025,  0x0.00000000000000p-1077 },   // 4.0e-0309
        { 0x1.CC359E067A348p-1025,  0x0.00000000000000p-1077 },   // 5.0e-0309
        { 0x1.14202B9D7C85Ep-1024,  0x0.00000000000000p-1076 },   // 6.0e-0309
        { 0x1.42258837BBF19p-1024,  0x0.00000000000000p-1076 },   // 7.0e-0309
        { 0x1.702AE4D1FB5D3p-1024,  0x0.00000000000000p-1076 },   // 8.0e-0309
        { 0x1.9E30416C3AC8Ep-1024,  0x0.00000000000000p-1076 },   // 9.0e-0309
        { 0x1.CC359E067A348p-1024,  0x0.00000000000000p-1076 },   // 1.0e-0308
        { 0x1.CC359E067A348p-1023,  0x0.00000000000000p-1075 },   // 2.0e-0308
        { 0x1.59283684DBA76p-1022,  0x0.8CF2795B6CABE0p-1074 },   // 3.0e-0308
        { 0x1.CC359E067A348p-1022,  0x0.BBEDF72490E530p-1074 },   // 4.0e-0308
        { 0x1.1FA182C40C60Dp-1021,  0x0.7574BA76DA8F38p-1073 },   // 5.0e-0308
        { 0x1.59283684DBA76p-1021,  0x0.8CF2795B6CABE0p-1073 },   // 6.0e-0308
        { 0x1.92AEEA45AAEDFp-1021,  0x0.A470383FFEC888p-1073 },   // 7.0e-0308
        { 0x1.CC359E067A348p-1021,  0x0.BBEDF72490E530p-1073 },   // 8.0e-0308
        { 0x1.02DE28E3A4BD8p-1020,  0x0.E9B5DB049180E8p-1072 },   // 9.0e-0308
        { 0x1.1FA182C40C60Dp-1020,  0x0.7574BA76DA8F38p-1072 },   // 1.0e-0307
        { 0x1.1FA182C40C60Dp-1019,  0x0.7574BA76DA8F38p-1071 },   // 2.0e-0307
        { 0x1.AF72442612914p-1019,  0x0.302F17B247D6D8p-1071 },   // 3.0e-0307
        { 0x1.1FA182C40C60Dp-1018,  0x0.7574BA76DA8F38p-1070 },   // 4.0e-0307
        { 0x1.6789E3750F790p-1018,  0x0.D2D1E914913308p-1070 },   // 5.0e-0307
        { 0x1.AF72442612914p-1018,  0x0.302F17B247D6D8p-1070 },   // 6.0e-0307
        { 0x1.F75AA4D715A97p-1018,  0x0.8D8C464FFE7AA8p-1070 },   // 7.0e-0307
        { 0x1.1FA182C40C60Dp-1017,  0x0.7574BA76DA8F38p-1069 },   // 8.0e-0307
        { 0x1.4395B31C8DECFp-1017,  0x0.242351C5B5E120p-1069 },   // 9.0e-0307
        { 0x1.6789E3750F790p-1017,  0x0.D2D1E914913308p-1069 },   // 1.0e-0306
        { 0x1.6789E3750F790p-1016,  0x0.D2D1E914913308p-1068 },   // 2.0e-0306
        { 0x1.0DA76A97CB9ACp-1015,  0x0.9E1D6ECF6CE648p-1067 },   // 3.0e-0306
        { 0x1.6789E3750F790p-1015,  0x0.D2D1E914913308p-1067 },   // 4.0e-0306
        { 0x1.C16C5C5253575p-1015,  0x0.07866359B57FD0p-1067 },   // 5.0e-0306
        { 0x1.0DA76A97CB9ACp-1014,  0x0.9E1D6ECF6CE648p-1066 },   // 6.0e-0306
        { 0x1.3A98A7066D89Ep-1014,  0x0.B877ABF1FF0CA8p-1066 },   // 7.0e-0306
        { 0x1.6789E3750F790p-1014,  0x0.D2D1E914913308p-1066 },   // 8.0e-0306
        { 0x1.947B1FE3B1682p-1014,  0x0.ED2C2637235970p-1066 },   // 9.0e-0306
        { 0x1.C16C5C5253575p-1014,  0x0.07866359B57FD0p-1066 },   // 1.0e-0305
        { 0x1.C16C5C5253575p-1013,  0x0.07866359B57FD0p-1065 },   // 2.0e-0305
        { 0x1.5111453DBE817p-1012,  0x0.C5A4CA83481FD8p-1064 },   // 3.0e-0305
        { 0x1.C16C5C5253575p-1012,  0x0.07866359B57FD0p-1064 },   // 4.0e-0305
        { 0x1.18E3B9B374169p-1011,  0x0.24B3FE18116FE0p-1063 },   // 5.0e-0305
        { 0x1.5111453DBE817p-1011,  0x0.C5A4CA83481FD8p-1063 },   // 6.0e-0305
        { 0x1.893ED0C808EC6p-1011,  0x0.669596EE7ECFD8p-1063 },   // 7.0e-0305
        { 0x1.C16C5C5253575p-1011,  0x0.07866359B57FD0p-1063 },   // 8.0e-0305
        { 0x1.F999E7DC9DC23p-1011,  0x0.A8772FC4EC2FC8p-1063 },   // 9.0e-0305
        { 0x1.18E3B9B374169p-1010,  0x0.24B3FE18116FE0p-1062 },   // 1.0e-0304
        { 0x1.18E3B9B374169p-1009,  0x0.24B3FE18116FE0p-1061 },   // 2.0e-0304
        { 0x1.A555968D2E21Dp-1009,  0x0.B70DFD241A27D0p-1061 },   // 3.0e-0304
        { 0x1.18E3B9B374169p-1008,  0x0.24B3FE18116FE0p-1060 },   // 4.0e-0304
        { 0x1.5F1CA820511C3p-1008,  0x0.6DE0FD9E15CBD8p-1060 },   // 5.0e-0304
        { 0x1.A555968D2E21Dp-1008,  0x0.B70DFD241A27D0p-1060 },   // 6.0e-0304
        { 0x1.EB8E84FA0B278p-1008,  0x0.003AFCAA1E83C8p-1060 },   // 7.0e-0304
        { 0x1.18E3B9B374169p-1007,  0x0.24B3FE18116FE0p-1059 },   // 8.0e-0304
        { 0x1.3C0030E9E2996p-1007,  0x0.494A7DDB139DE0p-1059 },   // 9.0e-0304
        { 0x1.5F1CA820511C3p-1007,  0x0.6DE0FD9E15CBD8p-1059 },   // 1.0e-0303
        { 0x1.5F1CA820511C3p-1006,  0x0.6DE0FD9E15CBD8p-1058 },   // 2.0e-0303
        { 0x1.07557E183CD52p-1005,  0x0.9268BE369058E0p-1057 },   // 3.0e-0303
        { 0x1.5F1CA820511C3p-1005,  0x0.6DE0FD9E15CBD8p-1057 },   // 4.0e-0303
        { 0x1.B6E3D22865634p-1005,  0x0.49593D059B3ED0p-1057 },   // 5.0e-0303
        { 0x1.07557E183CD52p-1004,  0x0.9268BE369058E0p-1056 },   // 6.0e-0303
        { 0x1.3339131C46F8Bp-1004,  0x0.0024DDEA531260p-1056 },   // 7.0e-0303
        { 0x1.5F1CA820511C3p-1004,  0x0.6DE0FD9E15CBD8p-1056 },   // 8.0e-0303
        { 0x1.8B003D245B3FBp-1004,  0x0.DB9D1D51D88558p-1056 },   // 9.0e-0303
        { 0x1.B6E3D22865634p-1004,  0x0.49593D059B3ED0p-1056 },   // 1.0e-0302
        { 0x1.B6E3D22865634p-1003,  0x0.49593D059B3ED0p-1055 },   // 2.0e-0302
        { 0x1.492ADD9E4C0A7p-1002,  0x0.3702EDC4346F18p-1054 },   // 3.0e-0302
        { 0x1.B6E3D22865634p-1002,  0x0.49593D059B3ED0p-1054 },   // 4.0e-0302
        { 0x1.124E63593F5E0p-1001,  0x0.ADD7C623810740p-1053 },   // 5.0e-0302
        { 0x1.492ADD9E4C0A7p-1001,  0x0.3702EDC4346F18p-1053 },   // 6.0e-0302
        { 0x1.800757E358B6Dp-1001,  0x0.C02E1564E7D6F8p-1053 },   // 7.0e-0302
        { 0x1.B6E3D22865634p-1001,  0x0.49593D059B3ED0p-1053 },   // 8.0e-0302
        { 0x1.EDC04C6D720FAp-1001,  0x0.D28464A64EA6A8p-1053 },   // 9.0e-0302
        { 0x1.124E63593F5E0p-1000,  0x0.ADD7C623810740p-1052 },   // 1.0e-0301
        { 0x1.124E63593F5E0p-0999,  0x0.ADD7C623810740p-1051 },   // 2.0e-0301
        { 0x1.9B759505DF0D1p-0999,  0x0.04C3A935418AE0p-1051 },   // 3.0e-0301
        { 0x1.124E63593F5E0p-0998,  0x0.ADD7C623810740p-1050 },   // 4.0e-0301
        { 0x1.56E1FC2F8F358p-0998,  0x0.D94DB7AC614910p-1050 },   // 5.0e-0301
        { 0x1.9B759505DF0D1p-0998,  0x0.04C3A935418AE0p-1050 },   // 6.0e-0301
        { 0x1.E0092DDC2EE49p-0998,  0x0.30399ABE21CCB0p-1050 },   // 7.0e-0301
        { 0x1.124E63593F5E0p-0997,  0x0.ADD7C623810740p-1049 },   // 8.0e-0301
        { 0x1.34982FC46749Cp-0997,  0x0.C392BEE7F12828p-1049 },   // 9.0e-0301
        { 0x1.56E1FC2F8F358p-0997,  0x0.D94DB7AC614910p-1049 },   // 1.0e-0300
        { 0x1.56E1FC2F8F358p-0996,  0x0.D94DB7AC614910p-1048 },   // 2.0e-0300
        { 0x1.01297D23AB682p-0995,  0x0.A2FA49C148F6D0p-1047 },   // 3.0e-0300
        { 0x1.56E1FC2F8F358p-0995,  0x0.D94DB7AC614910p-1047 },   // 4.0e-0300
        { 0x1.AC9A7B3B7302Fp-0995,  0x0.0FA12597799B58p-1047 },   // 5.0e-0300
        { 0x1.01297D23AB682p-0994,  0x0.A2FA49C148F6D0p-1046 },   // 6.0e-0300
        { 0x1.2C05BCA99D4EDp-0994,  0x0.BE2400B6D51FF0p-1046 },   // 7.0e-0300
        { 0x1.56E1FC2F8F358p-0994,  0x0.D94DB7AC614910p-1046 },   // 8.0e-0300
        { 0x1.81BE3BB5811C3p-0994,  0x0.F4776EA1ED7238p-1046 },   // 9.0e-0300
        { 0x1.AC9A7B3B7302Fp-0994,  0x0.0FA12597799B58p-1046 },   // 1.0e-0299
        { 0x1.AC9A7B3B7302Fp-0993,  0x0.0FA12597799B58p-1045 },   // 2.0e-0299
        { 0x1.4173DC6C96423p-0992,  0x0.4BB8DC319B3480p-1044 },   // 3.0e-0299
        { 0x1.AC9A7B3B7302Fp-0992,  0x0.0FA12597799B58p-1044 },   // 4.0e-0299
        { 0x1.0BE08D0527E1Dp-0991,  0x0.69C4B77EAC0118p-1043 },   // 5.0e-0299
        { 0x1.4173DC6C96423p-0991,  0x0.4BB8DC319B3480p-1043 },   // 6.0e-0299
        { 0x1.77072BD404A29p-0991,  0x0.2DAD00E48A67E8p-1043 },   // 7.0e-0299
        { 0x1.AC9A7B3B7302Fp-0991,  0x0.0FA12597799B58p-1043 },   // 8.0e-0299
        { 0x1.E22DCAA2E1634p-0991,  0x0.F1954A4A68CEC0p-1043 },   // 9.0e-0299
        { 0x1.0BE08D0527E1Dp-0990,  0x0.69C4B77EAC0118p-1042 },   // 1.0e-0298
        { 0x1.0BE08D0527E1Dp-0989,  0x0.69C4B77EAC0118p-1041 },   // 2.0e-0298
        { 0x1.91D0D387BBD2Cp-0989,  0x0.1EA7133E0201A0p-1041 },   // 3.0e-0298
        { 0x1.0BE08D0527E1Dp-0988,  0x0.69C4B77EAC0118p-1040 },   // 4.0e-0298
        { 0x1.4ED8B04671DA4p-0988,  0x0.C435E55E570158p-1040 },   // 5.0e-0298
        { 0x1.91D0D387BBD2Cp-0988,  0x0.1EA7133E0201A0p-1040 },   // 6.0e-0298
        { 0x1.D4C8F6C905CB3p-0988,  0x0.7918411DAD01E8p-1040 },   // 7.0e-0298
        { 0x1.0BE08D0527E1Dp-0987,  0x0.69C4B77EAC0118p-1039 },   // 8.0e-0298
        { 0x1.2D5C9EA5CCDE1p-0987,  0x0.16FD4E6E818138p-1039 },   // 9.0e-0298
        { 0x1.4ED8B04671DA4p-0987,  0x0.C435E55E570158p-1039 },   // 1.0e-0297
        { 0x1.4ED8B04671DA4p-0986,  0x0.C435E55E570158p-1038 },   // 2.0e-0297
        { 0x1.F6450869AAC77p-0986,  0x0.2650D80D828208p-1038 },   // 3.0e-0297
        { 0x1.4ED8B04671DA4p-0985,  0x0.C435E55E570158p-1037 },   // 4.0e-0297
        { 0x1.A28EDC580E50Dp-0985,  0x0.F5435EB5ECC1B0p-1037 },   // 5.0e-0297
        { 0x1.F6450869AAC77p-0985,  0x0.2650D80D828208p-1037 },   // 6.0e-0297
        { 0x1.24FD9A3DA39F0p-0984,  0x0.2BAF28B28C2130p-1036 },   // 7.0e-0297
        { 0x1.4ED8B04671DA4p-0984,  0x0.C435E55E570158p-1036 },   // 8.0e-0297
        { 0x1.78B3C64F40159p-0984,  0x0.5CBCA20A21E188p-1036 },   // 9.0e-0297
        { 0x1.A28EDC580E50Dp-0984,  0x0.F5435EB5ECC1B0p-1036 },   // 1.0e-0296
        { 0x1.A28EDC580E50Dp-0983,  0x0.F5435EB5ECC1B0p-1035 },   // 2.0e-0296
        { 0x1.39EB25420ABCAp-0982,  0x0.77F28708719148p-1034 },   // 3.0e-0296
        { 0x1.A28EDC580E50Dp-0982,  0x0.F5435EB5ECC1B0p-1034 },   // 4.0e-0296
        { 0x1.059949B708F28p-0981,  0x0.B94A1B31B3F910p-1033 },   // 5.0e-0296
        { 0x1.39EB25420ABCAp-0981,  0x0.77F28708719148p-1033 },   // 6.0e-0296
        { 0x1.6E3D00CD0C86Cp-0981,  0x0.369AF2DF2F2978p-1033 },   // 7.0e-0296
        { 0x1.A28EDC580E50Dp-0981,  0x0.F5435EB5ECC1B0p-1033 },   // 8.0e-0296
        { 0x1.D6E0B7E3101AFp-0981,  0x0.B3EBCA8CAA59E8p-1033 },   // 9.0e-0296
        { 0x1.059949B708F28p-0980,  0x0.B94A1B31B3F910p-1032 },   // 1.0e-0295
        { 0x1.059949B708F28p-0979,  0x0.B94A1B31B3F910p-1031 },   // 2.0e-0295
        { 0x1.8865EE928D6BDp-0979,  0x0.15EF28CA8DF598p-1031 },   // 3.0e-0295
        { 0x1.059949B708F28p-0978,  0x0.B94A1B31B3F910p-1030 },   // 4.0e-0295
        { 0x1.46FF9C24CB2F2p-0978,  0x0.E79CA1FE20F750p-1030 },   // 5.0e-0295
        { 0x1.8865EE928D6BDp-0978,  0x0.15EF28CA8DF598p-1030 },   // 6.0e-0295
        { 0x1.C9CC41004FA87p-0978,  0x0.4441AF96FAF3D8p-1030 },   // 7.0e-0295
        { 0x1.059949B708F28p-0977,  0x0.B94A1B31B3F910p-1029 },   // 8.0e-0295
        { 0x1.264C72EDEA10Dp-0977,  0x0.D0735E97EA7830p-1029 },   // 9.0e-0295
        { 0x1.46FF9C24CB2F2p-0977,  0x0.E79CA1FE20F750p-1029 },   // 1.0e-0294
        { 0x1.46FF9C24CB2F2p-0976,  0x0.E79CA1FE20F750p-1028 },   // 2.0e-0294
        { 0x1.EA7F6A3730C6Cp-0976,  0x0.5B6AF2FD317300p-1028 },   // 3.0e-0294
        { 0x1.46FF9C24CB2F2p-0975,  0x0.E79CA1FE20F750p-1027 },   // 4.0e-0294
        { 0x1.98BF832DFDFAFp-0975,  0x0.A183CA7DA93528p-1027 },   // 5.0e-0294
        { 0x1.EA7F6A3730C6Cp-0975,  0x0.5B6AF2FD317300p-1027 },   // 6.0e-0294
        { 0x1.1E1FA8A031C94p-0974,  0x0.8AA90DBE5CD868p-1026 },   // 7.0e-0294
        { 0x1.46FF9C24CB2F2p-0974,  0x0.E79CA1FE20F750p-1026 },   // 8.0e-0294
        { 0x1.6FDF8FA964951p-0974,  0x0.4490363DE51640p-1026 },   // 9.0e-0294
        { 0x1.98BF832DFDFAFp-0974,  0x0.A183CA7DA93528p-1026 },   // 1.0e-0293
        { 0x1.98BF832DFDFAFp-0973,  0x0.A183CA7DA93528p-1025 },   // 2.0e-0293
        { 0x1.328FA2627E7C3p-0972,  0x0.B922D7DE3EE7E0p-1024 },   // 3.0e-0293
        { 0x1.98BF832DFDFAFp-0972,  0x0.A183CA7DA93528p-1024 },   // 4.0e-0293
        { 0x1.FEEF63F97D79Bp-0972,  0x0.89E4BD1D138270p-1024 },   // 5.0e-0293
        { 0x1.328FA2627E7C3p-0971,  0x0.B922D7DE3EE7E0p-1023 },   // 6.0e-0293
        { 0x1.65A792C83E3B9p-0971,  0x0.AD53512DF40E80p-1023 },   // 7.0e-0293
        { 0x1.98BF832DFDFAFp-0971,  0x0.A183CA7DA93528p-1023 },   // 8.0e-0293
        { 0x1.CBD77393BDBA5p-0971,  0x0.95B443CD5E5BD0p-1023 },   // 9.0e-0293
        { 0x1.FEEF63F97D79Bp-0971,  0x0.89E4BD1D138270p-1023 },   // 1.0e-0292
        { 0x1.FEEF63F97D79Bp-0970,  0x0.89E4BD1D138270p-1022 },   // 2.0e-0292
        { 0x1.7F338AFB1E1B4p-0969,  0x0.A76B8DD5CEA1D8p-1021 },   // 3.0e-0292
        { 0x1.FEEF63F97D79Bp-0969,  0x0.89E4BD1D138270p-1021 },   // 4.0e-0292
        { 0x1.3F559E7BEE6C1p-0968,  0x0.362EF6322C3188p-1020 },   // 5.0e-0292
        { 0x1.7F338AFB1E1B4p-0968,  0x0.A76B8DD5CEA1D8p-1020 },   // 6.0e-0292
        { 0x1.BF11777A4DCA8p-0968,  0x0.18A82579711228p-1020 },   // 7.0e-0292
        { 0x1.FEEF63F97D79Bp-0968,  0x0.89E4BD1D138270p-1020 },   // 8.0e-0292
        { 0x1.1F66A83C56947p-0967,  0x0.7D90AA605AF960p-1019 },   // 9.0e-0292
        { 0x1.3F559E7BEE6C1p-0967,  0x0.362EF6322C3188p-1019 },   // 1.0e-0291
        { 0x1.3F559E7BEE6C1p-0966,  0x0.362EF6322C3188p-1018 },   // 2.0e-0291
        { 0x1.DF006DB9E5A21p-0966,  0x0.D146714B424A48p-1018 },   // 3.0e-0291
        { 0x1.3F559E7BEE6C1p-0965,  0x0.362EF6322C3188p-1017 },   // 4.0e-0291
        { 0x1.8F2B061AEA071p-0965,  0x0.83BAB3BEB73DE8p-1017 },   // 5.0e-0291
        { 0x1.DF006DB9E5A21p-0965,  0x0.D146714B424A48p-1017 },   // 6.0e-0291
        { 0x1.176AEAAC709E9p-0964,  0x0.0F69176BE6AB58p-1016 },   // 7.0e-0291
        { 0x1.3F559E7BEE6C1p-0964,  0x0.362EF6322C3188p-1016 },   // 8.0e-0291
        { 0x1.6740524B6C399p-0964,  0x0.5CF4D4F871B7B8p-1016 },   // 9.0e-0291
        { 0x1.8F2B061AEA071p-0964,  0x0.83BAB3BEB73DE8p-1016 },   // 1.0e-0290
        { 0x1.8F2B061AEA071p-0963,  0x0.83BAB3BEB73DE8p-1015 },   // 2.0e-0290
        { 0x1.2B6044942F855p-0962,  0x0.22CC06CF096E70p-1014 },   // 3.0e-0290
        { 0x1.8F2B061AEA071p-0962,  0x0.83BAB3BEB73DE8p-1014 },   // 4.0e-0290
        { 0x1.F2F5C7A1A488Dp-0962,  0x0.E4A960AE650D68p-1014 },   // 5.0e-0290
        { 0x1.2B6044942F855p-0961,  0x0.22CC06CF096E70p-1013 },   // 6.0e-0290
        { 0x1.5D45A5578CC63p-0961,  0x0.53435D46E05628p-1013 },   // 7.0e-0290
        { 0x1.8F2B061AEA071p-0961,  0x0.83BAB3BEB73DE8p-1013 },   // 8.0e-0290
        { 0x1.C11066DE4747Fp-0961,  0x0.B4320A368E25A8p-1013 },   // 9.0e-0290
        { 0x1.F2F5C7A1A488Dp-0961,  0x0.E4A960AE650D68p-1013 },   // 1.0e-0289
        { 0x1.F2F5C7A1A488Dp-0960,  0x0.E4A960AE650D68p-1012 },   // 2.0e-0289
        { 0x1.763855B93B66Ap-0959,  0x0.6B7F0882CBCA08p-1011 },   // 3.0e-0289
        { 0x1.F2F5C7A1A488Dp-0959,  0x0.E4A960AE650D68p-1011 },   // 4.0e-0289
        { 0x1.37D99CC506D58p-0958,  0x0.AEE9DC6CFF2860p-1010 },   // 5.0e-0289
        { 0x1.763855B93B66Ap-0958,  0x0.6B7F0882CBCA08p-1010 },   // 6.0e-0289
        { 0x1.B4970EAD6FF7Cp-0958,  0x0.28143498986BB8p-1010 },   // 7.0e-0289
        { 0x1.F2F5C7A1A488Dp-0958,  0x0.E4A960AE650D68p-1010 },   // 8.0e-0289
        { 0x1.18AA404AEC8CFp-0957,  0x0.D09F466218D788p-1009 },   // 9.0e-0289
        { 0x1.37D99CC506D58p-0957,  0x0.AEE9DC6CFF2860p-1009 },   // 1.0e-0288
        { 0x1.37D99CC506D58p-0956,  0x0.AEE9DC6CFF2860p-1008 },   // 2.0e-0288
        { 0x1.D3C66B278A405p-0956,  0x0.065ECAA37EBC90p-1008 },   // 3.0e-0288
        { 0x1.37D99CC506D58p-0955,  0x0.AEE9DC6CFF2860p-1007 },   // 4.0e-0288
        { 0x1.85D003F6488AEp-0955,  0x0.DAA453883EF278p-1007 },   // 5.0e-0288
        { 0x1.D3C66B278A405p-0955,  0x0.065ECAA37EBC90p-1007 },   // 6.0e-0288
        { 0x1.10DE692C65FADp-0954,  0x0.990CA0DF5F4350p-1006 },   // 7.0e-0288
        { 0x1.37D99CC506D58p-0954,  0x0.AEE9DC6CFF2860p-1006 },   // 8.0e-0288
        { 0x1.5ED4D05DA7B03p-0954,  0x0.C4C717FA9F0D68p-1006 },   // 9.0e-0288
        { 0x1.85D003F6488AEp-0954,  0x0.DAA453883EF278p-1006 },   // 1.0e-0287
        { 0x1.85D003F6488AEp-0953,  0x0.DAA453883EF278p-1005 },   // 2.0e-0287
        { 0x1.245C02F8B6683p-0952,  0x0.23FB3EA62F35D8p-1004 },   // 3.0e-0287
        { 0x1.85D003F6488AEp-0952,  0x0.DAA453883EF278p-1004 },   // 4.0e-0287
        { 0x1.E74404F3DAADAp-0952,  0x0.914D686A4EAF18p-1004 },   // 5.0e-0287
        { 0x1.245C02F8B6683p-0951,  0x0.23FB3EA62F35D8p-1003 },   // 6.0e-0287
        { 0x1.551603777F798p-0951,  0x0.FF4FC917371428p-1003 },   // 7.0e-0287
        { 0x1.85D003F6488AEp-0951,  0x0.DAA453883EF278p-1003 },   // 8.0e-0287
        { 0x1.B68A0475119C4p-0951,  0x0.B5F8DDF946D0C8p-1003 },   // 9.0e-0287
        { 0x1.E74404F3DAADAp-0951,  0x0.914D686A4EAF18p-1003 },   // 1.0e-0286
        { 0x1.E74404F3DAADAp-0950,  0x0.914D686A4EAF18p-1002 },   // 2.0e-0286
        { 0x1.6D7303B6E4023p-0949,  0x0.ECFA0E4FBB0350p-1001 },   // 3.0e-0286
        { 0x1.E74404F3DAADAp-0949,  0x0.914D686A4EAF18p-1001 },   // 4.0e-0286
        { 0x1.308A831868AC8p-0948,  0x0.9AD06142712D68p-1000 },   // 5.0e-0286
        { 0x1.6D7303B6E4023p-0948,  0x0.ECFA0E4FBB0350p-1000 },   // 6.0e-0286
        { 0x1.AA5B84555F57Fp-0948,  0x0.3F23BB5D04D930p-1000 },   // 7.0e-0286
        { 0x1.E74404F3DAADAp-0948,  0x0.914D686A4EAF18p-1000 },   // 8.0e-0286
        { 0x1.121642C92B01Ap-0947,  0x0.F1BB8ABBCC4278p-0999 },   // 9.0e-0286
        { 0x1.308A831868AC8p-0947,  0x0.9AD06142712D68p-0999 },   // 1.0e-0285
        { 0x1.308A831868AC8p-0946,  0x0.9AD06142712D68p-0998 },   // 2.0e-0285
        { 0x1.C8CFC4A49D02Cp-0946,  0x0.E83891E3A9C420p-0998 },   // 3.0e-0285
        { 0x1.308A831868AC8p-0945,  0x0.9AD06142712D68p-0997 },   // 4.0e-0285
        { 0x1.7CAD23DE82D7Ap-0945,  0x0.C18479930D78C8p-0997 },   // 5.0e-0285
        { 0x1.C8CFC4A49D02Cp-0945,  0x0.E83891E3A9C420p-0997 },   // 6.0e-0285
        { 0x1.0A7932B55B96Fp-0944,  0x0.8776551A2307C0p-0996 },   // 7.0e-0285
        { 0x1.308A831868AC8p-0944,  0x0.9AD06142712D68p-0996 },   // 8.0e-0285
        { 0x1.569BD37B75C21p-0944,  0x0.AE2A6D6ABF5318p-0996 },   // 9.0e-0285
        { 0x1.7CAD23DE82D7Ap-0944,  0x0.C18479930D78C8p-0996 },   // 1.0e-0284
        { 0x1.7CAD23DE82D7Ap-0943,  0x0.C18479930D78C8p-0995 },   // 2.0e-0284
        { 0x1.1D81DAE6E221Cp-0942,  0x0.11235B2E4A1A98p-0994 },   // 3.0e-0284
        { 0x1.7CAD23DE82D7Ap-0942,  0x0.C18479930D78C8p-0994 },   // 4.0e-0284
        { 0x1.DBD86CD6238D9p-0942,  0x0.71E597F7D0D6F8p-0994 },   // 5.0e-0284
        { 0x1.1D81DAE6E221Cp-0941,  0x0.11235B2E4A1A98p-0993 },   // 6.0e-0284
        { 0x1.4D177F62B27CBp-0941,  0x0.6953EA60ABC9B0p-0993 },   // 7.0e-0284
        { 0x1.7CAD23DE82D7Ap-0941,  0x0.C18479930D78C8p-0993 },   // 8.0e-0284
        { 0x1.AC42C85A5332Ap-0941,  0x0.19B508C56F27E0p-0993 },   // 9.0e-0284
        { 0x1.DBD86CD6238D9p-0941,  0x0.71E597F7D0D6F8p-0993 },   // 1.0e-0283
        { 0x1.DBD86CD6238D9p-0940,  0x0.71E597F7D0D6F8p-0992 },   // 2.0e-0283
        { 0x1.64E251A09AAA3p-0939,  0x0.156C31F9DCA138p-0991 },   // 3.0e-0283
        { 0x1.DBD86CD6238D9p-0939,  0x0.71E597F7D0D6F8p-0991 },   // 4.0e-0283
        { 0x1.29674405D6387p-0938,  0x0.E72F7EFAE28658p-0990 },   // 5.0e-0283
        { 0x1.64E251A09AAA3p-0938,  0x0.156C31F9DCA138p-0990 },   // 6.0e-0283
        { 0x1.A05D5F3B5F1BEp-0938,  0x0.43A8E4F8D6BC18p-0990 },   // 7.0e-0283
        { 0x1.DBD86CD6238D9p-0938,  0x0.71E597F7D0D6F8p-0990 },   // 8.0e-0283
        { 0x1.0BA9BD3873FFAp-0937,  0x0.5011257B6578E8p-0989 },   // 9.0e-0283
        { 0x1.29674405D6387p-0937,  0x0.E72F7EFAE28658p-0989 },   // 1.0e-0282
        { 0x1.29674405D6387p-0936,  0x0.E72F7EFAE28658p-0988 },   // 2.0e-0282
        { 0x1.BE1AE608C154Bp-0936,  0x0.DAC73E7853C988p-0988 },   // 3.0e-0282
        { 0x1.29674405D6387p-0935,  0x0.E72F7EFAE28658p-0987 },   // 4.0e-0282
        { 0x1.73C115074BC69p-0935,  0x0.E0FB5EB99B27F0p-0987 },   // 5.0e-0282
        { 0x1.BE1AE608C154Bp-0935,  0x0.DAC73E7853C988p-0987 },   // 6.0e-0282
        { 0x1.043A5B851B716p-0934,  0x0.EA498F1B863590p-0986 },   // 7.0e-0282
        { 0x1.29674405D6387p-0934,  0x0.E72F7EFAE28658p-0986 },   // 8.0e-0282
        { 0x1.4E942C8690FF8p-0934,  0x0.E4156EDA3ED728p-0986 },   // 9.0e-0282
        { 0x1.73C115074BC69p-0934,  0x0.E0FB5EB99B27F0p-0986 },   // 1.0e-0281
        { 0x1.73C115074BC69p-0933,  0x0.E0FB5EB99B27F0p-0985 },   // 2.0e-0281
        { 0x1.16D0CFC578D4Fp-0932,  0x0.68BC870B345DF8p-0984 },   // 3.0e-0281
        { 0x1.73C115074BC69p-0932,  0x0.E0FB5EB99B27F0p-0984 },   // 4.0e-0281
        { 0x1.D0B15A491EB84p-0932,  0x0.593A366801F1F0p-0984 },   // 5.0e-0281
        { 0x1.16D0CFC578D4Fp-0931,  0x0.68BC870B345DF8p-0983 },   // 6.0e-0281
        { 0x1.4548F266624DCp-0931,  0x0.A4DBF2E267C2F0p-0983 },   // 7.0e-0281
        { 0x1.73C115074BC69p-0931,  0x0.E0FB5EB99B27F0p-0983 },   // 8.0e-0281
        { 0x1.A23937A8353F7p-0931,  0x0.1D1ACA90CE8CF0p-0983 },   // 9.0e-0281
        { 0x1.D0B15A491EB84p-0931,  0x0.593A366801F1F0p-0983 },   // 1.0e-0280
        { 0x1.D0B15A491EB84p-0930,  0x0.593A366801F1F0p-0982 },   // 2.0e-0280
        { 0x1.5C8503B6D70A3p-0929,  0x0.42EBA8CE017570p-0981 },   // 3.0e-0280
        { 0x1.D0B15A491EB84p-0929,  0x0.593A366801F1F0p-0981 },   // 4.0e-0280
        { 0x1.226ED86DB3332p-0928,  0x0.B7C46201013738p-0980 },   // 5.0e-0280
        { 0x1.5C8503B6D70A3p-0928,  0x0.42EBA8CE017570p-0980 },   // 6.0e-0280
        { 0x1.969B2EFFFAE13p-0928,  0x0.CE12EF9B01B3B0p-0980 },   // 7.0e-0280
        { 0x1.D0B15A491EB84p-0928,  0x0.593A366801F1F0p-0980 },   // 8.0e-0280
        { 0x1.0563C2C92147Ap-0927,  0x0.7230BE9A811818p-0979 },   // 9.0e-0280
        { 0x1.226ED86DB3332p-0927,  0x0.B7C46201013738p-0979 },   // 1.0e-0279
        { 0x1.226ED86DB3332p-0926,  0x0.B7C46201013738p-0978 },   // 2.0e-0279
        { 0x1.B3A644A48CCCCp-0926,  0x0.13A6930181D2D0p-0978 },   // 3.0e-0279
        { 0x1.226ED86DB3332p-0925,  0x0.B7C46201013738p-0977 },   // 4.0e-0279
        { 0x1.6B0A8E891FFFFp-0925,  0x0.65B57A81418500p-0977 },   // 5.0e-0279
        { 0x1.B3A644A48CCCCp-0925,  0x0.13A6930181D2D0p-0977 },   // 6.0e-0279
        { 0x1.FC41FABFF9998p-0925,  0x0.C197AB81C220A0p-0977 },   // 7.0e-0279
        { 0x1.226ED86DB3332p-0924,  0x0.B7C46201013738p-0976 },   // 8.0e-0279
        { 0x1.46BCB37B69999p-0924,  0x0.0EBCEE41215E18p-0976 },   // 9.0e-0279
        { 0x1.6B0A8E891FFFFp-0924,  0x0.65B57A81418500p-0976 },   // 1.0e-0278
        { 0x1.6B0A8E891FFFFp-0923,  0x0.65B57A81418500p-0975 },   // 2.0e-0278
        { 0x1.1047EAE6D7FFFp-0922,  0x0.8C481BE0F123C0p-0974 },   // 3.0e-0278
        { 0x1.6B0A8E891FFFFp-0922,  0x0.65B57A81418500p-0974 },   // 4.0e-0278
        { 0x1.C5CD322B67FFFp-0922,  0x0.3F22D92191E648p-0974 },   // 5.0e-0278
        { 0x1.1047EAE6D7FFFp-0921,  0x0.8C481BE0F123C0p-0973 },   // 6.0e-0278
        { 0x1.3DA93CB7FBFFFp-0921,  0x0.78FECB31195460p-0973 },   // 7.0e-0278
        { 0x1.6B0A8E891FFFFp-0921,  0x0.65B57A81418500p-0973 },   // 8.0e-0278
        { 0x1.986BE05A43FFFp-0921,  0x0.526C29D169B5A0p-0973 },   // 9.0e-0278
        { 0x1.C5CD322B67FFFp-0921,  0x0.3F22D92191E640p-0973 },   // 1.0e-0277
        { 0x1.C5CD322B67FFFp-0920,  0x0.3F22D92191E640p-0972 },   // 2.0e-0277
        { 0x1.5459E5A08DFFFp-0919,  0x0.6F5A22D92D6CB0p-0971 },   // 3.0e-0277
        { 0x1.C5CD322B67FFFp-0919,  0x0.3F22D92191E640p-0971 },   // 4.0e-0277
        { 0x1.1BA03F5B20FFFp-0918,  0x0.8775C7B4FB2FE8p-0970 },   // 5.0e-0277
        { 0x1.5459E5A08DFFFp-0918,  0x0.6F5A22D92D6CB0p-0970 },   // 6.0e-0277
        { 0x1.8D138BE5FAFFFp-0918,  0x0.573E7DFD5FA978p-0970 },   // 7.0e-0277
        { 0x1.C5CD322B67FFFp-0918,  0x0.3F22D92191E640p-0970 },   // 8.0e-0277
        { 0x1.FE86D870D4FFFp-0918,  0x0.27073445C42310p-0970 },   // 9.0e-0277
        { 0x1.1BA03F5B20FFFp-0917,  0x0.8775C7B4FB2FE8p-0969 },   // 1.0e-0276
        { 0x1.1BA03F5B20FFFp-0916,  0x0.8775C7B4FB2FE8p-0968 },   // 2.0e-0276
        { 0x1.A9705F08B17FFp-0916,  0x0.4B30AB8F78C7E0p-0968 },   // 3.0e-0276
        { 0x1.1BA03F5B20FFFp-0915,  0x0.8775C7B4FB2FE8p-0967 },   // 4.0e-0276
        { 0x1.62884F31E93FFp-0915,  0x0.695339A239FBE8p-0967 },   // 5.0e-0276
        { 0x1.A9705F08B17FFp-0915,  0x0.4B30AB8F78C7E0p-0967 },   // 6.0e-0276
        { 0x1.F0586EDF79BFFp-0915,  0x0.2D0E1D7CB793D8p-0967 },   // 7.0e-0276
        { 0x1.1BA03F5B20FFFp-0914,  0x0.8775C7B4FB2FE8p-0966 },   // 8.0e-0276
        { 0x1.3F144746851FFp-0914,  0x0.786480AB9A95E8p-0966 },   // 9.0e-0276
        { 0x1.62884F31E93FFp-0914,  0x0.695339A239FBE8p-0966 },   // 1.0e-0275
        { 0x1.62884F31E93FFp-0913,  0x0.695339A239FBE8p-0965 },   // 2.0e-0275
        { 0x1.09E63B656EEFFp-0912,  0x0.8EFE6B39AB7CE8p-0964 },   // 3.0e-0275
        { 0x1.62884F31E93FFp-0912,  0x0.695339A239FBE8p-0964 },   // 4.0e-0275
        { 0x1.BB2A62FE638FFp-0912,  0x0.43A8080AC87AE0p-0964 },   // 5.0e-0275
        { 0x1.09E63B656EEFFp-0911,  0x0.8EFE6B39AB7CE8p-0963 },   // 6.0e-0275
        { 0x1.3637454BAC17Fp-0911,  0x0.7C28D26DF2BC68p-0963 },   // 7.0e-0275
        { 0x1.62884F31E93FFp-0911,  0x0.695339A239FBE8p-0963 },   // 8.0e-0275
        { 0x1.8ED959182667Fp-0911,  0x0.567DA0D6813B60p-0963 },   // 9.0e-0275
        { 0x1.BB2A62FE638FFp-0911,  0x0.43A8080AC87AE0p-0963 },   // 1.0e-0274
        { 0x1.BB2A62FE638FFp-0910,  0x0.43A8080AC87AE0p-0962 },   // 2.0e-0274
        { 0x1.4C5FCA3ECAABFp-0909,  0x0.72BE0608165C28p-0961 },   // 3.0e-0274
        { 0x1.BB2A62FE638FFp-0909,  0x0.43A8080AC87AE0p-0961 },   // 4.0e-0274
        { 0x1.14FA7DDEFE39Fp-0908,  0x0.8A490506BD4CC8p-0960 },   // 5.0e-0274
        { 0x1.4C5FCA3ECAABFp-0908,  0x0.72BE0608165C28p-0960 },   // 6.0e-0274
        { 0x1.83C5169E971DFp-0908,  0x0.5B3307096F6B80p-0960 },   // 7.0e-0274
        { 0x1.BB2A62FE638FFp-0908,  0x0.43A8080AC87AE0p-0960 },   // 8.0e-0274
        { 0x1.F28FAF5E3001Fp-0908,  0x0.2C1D090C218A38p-0960 },   // 9.0e-0274
        { 0x1.14FA7DDEFE39Fp-0907,  0x0.8A490506BD4CC8p-0959 },   // 1.0e-0273
        { 0x1.14FA7DDEFE39Fp-0906,  0x0.8A490506BD4CC8p-0958 },   // 2.0e-0273
        { 0x1.9F77BCCE7D56Fp-0906,  0x0.4F6D878A1BF330p-0958 },   // 3.0e-0273
        { 0x1.14FA7DDEFE39Fp-0905,  0x0.8A490506BD4CC8p-0957 },   // 4.0e-0273
        { 0x1.5A391D56BDC87p-0905,  0x0.6CDB46486CA000p-0957 },   // 5.0e-0273
        { 0x1.9F77BCCE7D56Fp-0905,  0x0.4F6D878A1BF330p-0957 },   // 6.0e-0273
        { 0x1.E4B65C463CE57p-0905,  0x0.31FFC8CBCB4660p-0957 },   // 7.0e-0273
        { 0x1.14FA7DDEFE39Fp-0904,  0x0.8A490506BD4CC8p-0956 },   // 8.0e-0273
        { 0x1.3799CD9ADE013p-0904,  0x0.7B9225A794F660p-0956 },   // 9.0e-0273
        { 0x1.5A391D56BDC87p-0904,  0x0.6CDB46486CA000p-0956 },   // 1.0e-0272
        { 0x1.5A391D56BDC87p-0903,  0x0.6CDB46486CA000p-0955 },   // 2.0e-0272
        { 0x1.03AAD6010E565p-0902,  0x0.91A474B6517800p-0954 },   // 3.0e-0272
        { 0x1.5A391D56BDC87p-0902,  0x0.6CDB46486CA000p-0954 },   // 4.0e-0272
        { 0x1.B0C764AC6D3A9p-0902,  0x0.481217DA87C800p-0954 },   // 5.0e-0272
        { 0x1.03AAD6010E565p-0901,  0x0.91A474B6517800p-0953 },   // 6.0e-0272
        { 0x1.2EF1F9ABE60F6p-0901,  0x0.7F3FDD7F5F0C00p-0953 },   // 7.0e-0272
        { 0x1.5A391D56BDC87p-0901,  0x0.6CDB46486CA000p-0953 },   // 8.0e-0272
        { 0x1.8580410195818p-0901,  0x0.5A76AF117A3400p-0953 },   // 9.0e-0272
        { 0x1.B0C764AC6D3A9p-0901,  0x0.481217DA87C800p-0953 },   // 1.0e-0271
        { 0x1.B0C764AC6D3A9p-0900,  0x0.481217DA87C800p-0952 },   // 2.0e-0271
        { 0x1.44958B8151EBEp-0899,  0x0.F60D91E3E5D600p-0951 },   // 3.0e-0271
        { 0x1.B0C764AC6D3A9p-0899,  0x0.481217DA87C800p-0951 },   // 4.0e-0271
        { 0x1.0E7C9EEBC4449p-0898,  0x0.CD0B4EE894DD00p-0950 },   // 5.0e-0271
        { 0x1.44958B8151EBEp-0898,  0x0.F60D91E3E5D600p-0950 },   // 6.0e-0271
        { 0x1.7AAE7816DF934p-0898,  0x0.1F0FD4DF36CF00p-0950 },   // 7.0e-0271
        { 0x1.B0C764AC6D3A9p-0898,  0x0.481217DA87C800p-0950 },   // 8.0e-0271
        { 0x1.E6E05141FAE1Ep-0898,  0x0.71145AD5D8C100p-0950 },   // 9.0e-0271
        { 0x1.0E7C9EEBC4449p-0897,  0x0.CD0B4EE894DD00p-0949 },   // 1.0e-0270
        { 0x1.0E7C9EEBC4449p-0896,  0x0.CD0B4EE894DD00p-0948 },   // 2.0e-0270
        { 0x1.95BAEE61A666Ep-0896,  0x0.B390F65CDF4B80p-0948 },   // 3.0e-0270
        { 0x1.0E7C9EEBC4449p-0895,  0x0.CD0B4EE894DD00p-0947 },   // 4.0e-0270
        { 0x1.521BC6A6B555Cp-0895,  0x0.404E22A2BA1440p-0947 },   // 5.0e-0270
        { 0x1.95BAEE61A666Ep-0895,  0x0.B390F65CDF4B80p-0947 },   // 6.0e-0270
        { 0x1.D95A161C97781p-0895,  0x0.26D3CA170482C0p-0947 },   // 7.0e-0270
        { 0x1.0E7C9EEBC4449p-0894,  0x0.CD0B4EE894DD00p-0946 },   // 8.0e-0270
        { 0x1.304C32C93CCD3p-0894,  0x0.06ACB8C5A778A0p-0946 },   // 9.0e-0270
        { 0x1.521BC6A6B555Cp-0894,  0x0.404E22A2BA1440p-0946 },   // 1.0e-0269
        { 0x1.521BC6A6B555Cp-0893,  0x0.404E22A2BA1440p-0945 },   // 2.0e-0269
        { 0x1.FB29A9FA1000Ap-0893,  0x0.607533F4171E60p-0945 },   // 3.0e-0269
        { 0x1.521BC6A6B555Cp-0892,  0x0.404E22A2BA1440p-0944 },   // 4.0e-0269
        { 0x1.A6A2B85062AB3p-0892,  0x0.5061AB4B689950p-0944 },   // 5.0e-0269
        { 0x1.FB29A9FA1000Ap-0892,  0x0.607533F4171E60p-0944 },   // 6.0e-0269
        { 0x1.27D84DD1DEAB0p-0891,  0x0.B8445E4E62D1B8p-0943 },   // 7.0e-0269
        { 0x1.521BC6A6B555Cp-0891,  0x0.404E22A2BA1440p-0943 },   // 8.0e-0269
        { 0x1.7C5F3F7B8C007p-0891,  0x0.C857E6F71156C8p-0943 },   // 9.0e-0269
        { 0x1.A6A2B85062AB3p-0891,  0x0.5061AB4B689950p-0943 },   // 1.0e-0268
        { 0x1.A6A2B85062AB3p-0890,  0x0.5061AB4B689950p-0942 },   // 2.0e-0268
        { 0x1.3CFA0A3C4A006p-0889,  0x0.7C4940788E72F8p-0941 },   // 3.0e-0268
        { 0x1.A6A2B85062AB3p-0889,  0x0.5061AB4B689950p-0941 },   // 4.0e-0268
        { 0x1.0825B3323DAB0p-0888,  0x0.123D0B0F215FD0p-0940 },   // 5.0e-0268
        { 0x1.3CFA0A3C4A006p-0888,  0x0.7C4940788E72F8p-0940 },   // 6.0e-0268
        { 0x1.71CE61465655Cp-0888,  0x0.E65575E1FB8620p-0940 },   // 7.0e-0268
        { 0x1.A6A2B85062AB3p-0888,  0x0.5061AB4B689950p-0940 },   // 8.0e-0268
        { 0x1.DB770F5A6F009p-0888,  0x0.BA6DE0B4D5AC78p-0940 },   // 9.0e-0268
        { 0x1.0825B3323DAB0p-0887,  0x0.123D0B0F215FD0p-0939 },   // 1.0e-0267
        { 0x1.0825B3323DAB0p-0886,  0x0.123D0B0F215FD0p-0938 },   // 2.0e-0267
        { 0x1.8C388CCB5C808p-0886,  0x0.1B5B9096B20FB8p-0938 },   // 3.0e-0267
        { 0x1.0825B3323DAB0p-0885,  0x0.123D0B0F215FD0p-0937 },   // 4.0e-0267
        { 0x1.4A2F1FFECD15Cp-0885,  0x0.16CC4DD2E9B7C0p-0937 },   // 5.0e-0267
        { 0x1.8C388CCB5C808p-0885,  0x0.1B5B9096B20FB8p-0937 },   // 6.0e-0267
        { 0x1.CE41F997EBEB4p-0885,  0x0.1FEAD35A7A67B0p-0937 },   // 7.0e-0267
        { 0x1.0825B3323DAB0p-0884,  0x0.123D0B0F215FD0p-0936 },   // 8.0e-0267
        { 0x1.292A699885606p-0884,  0x0.1484AC71058BC8p-0936 },   // 9.0e-0267
        { 0x1.4A2F1FFECD15Cp-0884,  0x0.16CC4DD2E9B7C0p-0936 },   // 1.0e-0266
        { 0x1.4A2F1FFECD15Cp-0883,  0x0.16CC4DD2E9B7C0p-0935 },   // 2.0e-0266
        { 0x1.EF46AFFE33A0Ap-0883,  0x0.223274BC5E93A8p-0935 },   // 3.0e-0266
        { 0x1.4A2F1FFECD15Cp-0882,  0x0.16CC4DD2E9B7C0p-0934 },   // 4.0e-0266
        { 0x1.9CBAE7FE805B3p-0882,  0x0.1C7F6147A425B8p-0934 },   // 5.0e-0266
        { 0x1.EF46AFFE33A0Ap-0882,  0x0.223274BC5E93A8p-0934 },   // 6.0e-0266
        { 0x1.20E93BFEF3730p-0881,  0x0.93F2C4188C80C8p-0933 },   // 7.0e-0266
        { 0x1.4A2F1FFECD15Cp-0881,  0x0.16CC4DD2E9B7C0p-0933 },   // 8.0e-0266
        { 0x1.737503FEA6B87p-0881,  0x0.99A5D78D46EEC0p-0933 },   // 9.0e-0266
        { 0x1.9CBAE7FE805B3p-0881,  0x0.1C7F6147A425B8p-0933 },   // 1.0e-0265
        { 0x1.9CBAE7FE805B3p-0880,  0x0.1C7F6147A425B8p-0932 },   // 2.0e-0265
        { 0x1.358C2DFEE0446p-0879,  0x0.555F88F5BB1C48p-0931 },   // 3.0e-0265
        { 0x1.9CBAE7FE805B3p-0879,  0x0.1C7F6147A425B8p-0931 },   // 4.0e-0265
        { 0x1.01F4D0FF1038Fp-0878,  0x0.F1CF9CCCC69790p-0930 },   // 5.0e-0265
        { 0x1.358C2DFEE0446p-0878,  0x0.555F88F5BB1C48p-0930 },   // 6.0e-0265
        { 0x1.69238AFEB04FCp-0878,  0x0.B8EF751EAFA100p-0930 },   // 7.0e-0265
        { 0x1.9CBAE7FE805B3p-0878,  0x0.1C7F6147A425B8p-0930 },   // 8.0e-0265
        { 0x1.D05244FE50669p-0878,  0x0.800F4D7098AA70p-0930 },   // 9.0e-0265
        { 0x1.01F4D0FF1038Fp-0877,  0x0.F1CF9CCCC69790p-0929 },   // 1.0e-0264
        { 0x1.01F4D0FF1038Fp-0876,  0x0.F1CF9CCCC69790p-0928 },   // 2.0e-0264
        { 0x1.82EF397E98557p-0876,  0x0.EAB76B3329E358p-0928 },   // 3.0e-0264
        { 0x1.01F4D0FF1038Fp-0875,  0x0.F1CF9CCCC69790p-0927 },   // 4.0e-0264
        { 0x1.4272053ED4473p-0875,  0x0.EE4383FFF83D78p-0927 },   // 5.0e-0264
        { 0x1.82EF397E98557p-0875,  0x0.EAB76B3329E358p-0927 },   // 6.0e-0264
        { 0x1.C36C6DBE5C63Bp-0875,  0x0.E72B52665B8940p-0927 },   // 7.0e-0264
        { 0x1.01F4D0FF1038Fp-0874,  0x0.F1CF9CCCC69790p-0926 },   // 8.0e-0264
        { 0x1.22336B1EF2401p-0874,  0x0.F00990665F6A80p-0926 },   // 9.0e-0264
        { 0x1.4272053ED4473p-0874,  0x0.EE4383FFF83D78p-0926 },   // 1.0e-0263
        { 0x1.4272053ED4473p-0873,  0x0.EE4383FFF83D78p-0925 },   // 2.0e-0263
        { 0x1.E3AB07DE3E6ADp-0873,  0x0.E56545FFF45C30p-0925 },   // 3.0e-0263
        { 0x1.4272053ED4473p-0872,  0x0.EE4383FFF83D78p-0924 },   // 4.0e-0263
        { 0x1.930E868E89590p-0872,  0x0.E9D464FFF64CD0p-0924 },   // 5.0e-0263
        { 0x1.E3AB07DE3E6ADp-0872,  0x0.E56545FFF45C30p-0924 },   // 6.0e-0263
        { 0x1.1A23C496F9BE5p-0871,  0x0.707B137FF935C8p-0923 },   // 7.0e-0263
        { 0x1.4272053ED4473p-0871,  0x0.EE4383FFF83D78p-0923 },   // 8.0e-0263
        { 0x1.6AC045E6AED02p-0871,  0x0.6C0BF47FF74520p-0923 },   // 9.0e-0263
        { 0x1.930E868E89590p-0871,  0x0.E9D464FFF64CD0p-0923 },   // 1.0e-0262
        { 0x1.930E868E89590p-0870,  0x0.E9D464FFF64CD0p-0922 },   // 2.0e-0262
        { 0x1.2E4AE4EAE702Cp-0869,  0x0.AF5F4BBFF8B9A0p-0921 },   // 3.0e-0262
        { 0x1.930E868E89590p-0869,  0x0.E9D464FFF64CD0p-0921 },   // 4.0e-0262
        { 0x1.F7D228322BAF5p-0869,  0x0.24497E3FF3E008p-0921 },   // 5.0e-0262
        { 0x1.2E4AE4EAE702Cp-0868,  0x0.AF5F4BBFF8B9A0p-0920 },   // 6.0e-0262
        { 0x1.60ACB5BCB82DEp-0868,  0x0.CC99D85FF78338p-0920 },   // 7.0e-0262
        { 0x1.930E868E89590p-0868,  0x0.E9D464FFF64CD0p-0920 },   // 8.0e-0262
        { 0x1.C57057605A843p-0868,  0x0.070EF19FF51670p-0920 },   // 9.0e-0262
        { 0x1.F7D228322BAF5p-0868,  0x0.24497E3FF3E008p-0920 },   // 1.0e-0261
        { 0x1.F7D228322BAF5p-0867,  0x0.24497E3FF3E008p-0919 },   // 2.0e-0261
        { 0x1.79DD9E25A0C37p-0866,  0x0.DB371EAFF6E808p-0918 },   // 3.0e-0261
        { 0x1.F7D228322BAF5p-0866,  0x0.24497E3FF3E008p-0918 },   // 4.0e-0261
        { 0x1.3AE3591F5B4D9p-0865,  0x0.36ADEEE7F86C00p-0917 },   // 5.0e-0261
        { 0x1.79DD9E25A0C37p-0865,  0x0.DB371EAFF6E808p-0917 },   // 6.0e-0261
        { 0x1.B8D7E32BE6396p-0865,  0x0.7FC04E77F56408p-0917 },   // 7.0e-0261
        { 0x1.F7D228322BAF5p-0865,  0x0.24497E3FF3E008p-0917 },   // 8.0e-0261
        { 0x1.1B66369C38929p-0864,  0x0.E4695703F92E00p-0916 },   // 9.0e-0261
        { 0x1.3AE3591F5B4D9p-0864,  0x0.36ADEEE7F86C00p-0916 },   // 1.0e-0260
        { 0x1.3AE3591F5B4D9p-0863,  0x0.36ADEEE7F86C00p-0915 },   // 2.0e-0260
        { 0x1.D85505AF08F45p-0863,  0x0.D204E65BF4A208p-0915 },   // 3.0e-0260
        { 0x1.3AE3591F5B4D9p-0862,  0x0.36ADEEE7F86C00p-0914 },   // 4.0e-0260
        { 0x1.899C2F673220Fp-0862,  0x0.84596AA1F68708p-0914 },   // 5.0e-0260
        { 0x1.D85505AF08F45p-0862,  0x0.D204E65BF4A208p-0914 },   // 6.0e-0260
        { 0x1.1386EDFB6FE3Ep-0861,  0x0.0FD8310AF95E80p-0913 },   // 7.0e-0260
        { 0x1.3AE3591F5B4D9p-0861,  0x0.36ADEEE7F86C00p-0913 },   // 8.0e-0260
        { 0x1.623FC44346B74p-0861,  0x0.5D83ACC4F77988p-0913 },   // 9.0e-0260
        { 0x1.899C2F673220Fp-0861,  0x0.84596AA1F68708p-0913 },   // 1.0e-0259
        { 0x1.899C2F673220Fp-0860,  0x0.84596AA1F68708p-0912 },   // 2.0e-0259
        { 0x1.2735238D6598Bp-0859,  0x0.A3430FF978E540p-0911 },   // 3.0e-0259
        { 0x1.899C2F673220Fp-0859,  0x0.84596AA1F68708p-0911 },   // 4.0e-0259
        { 0x1.EC033B40FEA93p-0859,  0x0.656FC54A7428C8p-0911 },   // 5.0e-0259
        { 0x1.2735238D6598Bp-0858,  0x0.A3430FF978E540p-0910 },   // 6.0e-0259
        { 0x1.5868A97A4BDCDp-0858,  0x0.93CE3D4DB7B628p-0910 },   // 7.0e-0259
        { 0x1.899C2F673220Fp-0858,  0x0.84596AA1F68708p-0910 },   // 8.0e-0259
        { 0x1.BACFB55418651p-0858,  0x0.74E497F63557E8p-0910 },   // 9.0e-0259
        { 0x1.EC033B40FEA93p-0858,  0x0.656FC54A7428C8p-0910 },   // 1.0e-0258
        { 0x1.EC033B40FEA93p-0857,  0x0.656FC54A7428C8p-0909 },   // 2.0e-0258
        { 0x1.71026C70BEFEEp-0856,  0x0.8C13D3F7D71E98p-0908 },   // 3.0e-0258
        { 0x1.EC033B40FEA93p-0856,  0x0.656FC54A7428C8p-0908 },   // 4.0e-0258
        { 0x1.338205089F29Cp-0855,  0x0.1F65DB4E889978p-0907 },   // 5.0e-0258
        { 0x1.71026C70BEFEEp-0855,  0x0.8C13D3F7D71E98p-0907 },   // 6.0e-0258
        { 0x1.AE82D3D8DED40p-0855,  0x0.F8C1CCA125A3B0p-0907 },   // 7.0e-0258
        { 0x1.EC033B40FEA93p-0855,  0x0.656FC54A7428C8p-0907 },   // 8.0e-0258
        { 0x1.14C1D1548F3F2p-0854,  0x0.E90EDEF9E156F0p-0906 },   // 9.0e-0258
        { 0x1.338205089F29Cp-0854,  0x0.1F65DB4E889978p-0906 },   // 1.0e-0257
        { 0x1.338205089F29Cp-0853,  0x0.1F65DB4E889978p-0905 },   // 2.0e-0257
        { 0x1.CD43078CEEBEAp-0853,  0x0.2F18C8F5CCE638p-0905 },   // 3.0e-0257
        { 0x1.338205089F29Cp-0852,  0x0.1F65DB4E889978p-0904 },   // 4.0e-0257
        { 0x1.8062864AC6F43p-0852,  0x0.273F52222ABFD8p-0904 },   // 5.0e-0257
        { 0x1.CD43078CEEBEAp-0852,  0x0.2F18C8F5CCE638p-0904 },   // 6.0e-0257
        { 0x1.0D11C4678B448p-0851,  0x0.9B791FE4B78648p-0903 },   // 7.0e-0257
        { 0x1.338205089F29Cp-0851,  0x0.1F65DB4E889978p-0903 },   // 8.0e-0257
        { 0x1.59F245A9B30EFp-0851,  0x0.A35296B859ACA8p-0903 },   // 9.0e-0257
        { 0x1.8062864AC6F43p-0851,  0x0.273F52222ABFD8p-0903 },   // 1.0e-0256
        { 0x1.8062864AC6F43p-0850,  0x0.273F52222ABFD8p-0902 },   // 2.0e-0256
        { 0x1.2049E4B815372p-0849,  0x0.5D6F7D99A00FE0p-0901 },   // 3.0e-0256
        { 0x1.8062864AC6F43p-0849,  0x0.273F52222ABFD8p-0901 },   // 4.0e-0256
        { 0x1.E07B27DD78B13p-0849,  0x0.F10F26AAB56FD0p-0901 },   // 5.0e-0256
        { 0x1.2049E4B815372p-0848,  0x0.5D6F7D99A00FE0p-0900 },   // 6.0e-0256
        { 0x1.505635816E15Ap-0848,  0x0.C25767DDE567E0p-0900 },   // 7.0e-0256
        { 0x1.8062864AC6F43p-0848,  0x0.273F52222ABFD8p-0900 },   // 8.0e-0256
        { 0x1.B06ED7141FD2Bp-0848,  0x0.8C273C667017D8p-0900 },   // 9.0e-0256
        { 0x1.E07B27DD78B13p-0848,  0x0.F10F26AAB56FD0p-0900 },   // 1.0e-0255
        { 0x1.E07B27DD78B13p-0847,  0x0.F10F26AAB56FD0p-0899 },   // 2.0e-0255
        { 0x1.685C5DE61A84Ep-0846,  0x0.F4CB5D000813E0p-0898 },   // 3.0e-0255
        { 0x1.E07B27DD78B13p-0846,  0x0.F10F26AAB56FD0p-0898 },   // 4.0e-0255
        { 0x1.2C4CF8EA6B6ECp-0845,  0x0.76A9782AB165E0p-0897 },   // 5.0e-0255
        { 0x1.685C5DE61A84Ep-0845,  0x0.F4CB5D000813E0p-0897 },   // 6.0e-0255
        { 0x1.A46BC2E1C99B1p-0845,  0x0.72ED41D55EC1D8p-0897 },   // 7.0e-0255
        { 0x1.E07B27DD78B13p-0845,  0x0.F10F26AAB56FD0p-0897 },   // 8.0e-0255
        { 0x1.0E45466C93E3Bp-0844,  0x0.379885C0060EE8p-0896 },   // 9.0e-0255
        { 0x1.2C4CF8EA6B6ECp-0844,  0x0.76A9782AB165E0p-0896 },   // 1.0e-0254
        { 0x1.2C4CF8EA6B6ECp-0843,  0x0.76A9782AB165E0p-0895 },   // 2.0e-0254
        { 0x1.C273755FA1262p-0843,  0x0.B1FE34400A18D8p-0895 },   // 3.0e-0254
        { 0x1.2C4CF8EA6B6ECp-0842,  0x0.76A9782AB165E0p-0894 },   // 4.0e-0254
        { 0x1.77603725064A7p-0842,  0x0.9453D6355DBF60p-0894 },   // 5.0e-0254
        { 0x1.C273755FA1262p-0842,  0x0.B1FE34400A18D8p-0894 },   // 6.0e-0254
        { 0x1.06C359CD1E00Ep-0841,  0x0.E7D449255B3928p-0893 },   // 7.0e-0254
        { 0x1.2C4CF8EA6B6ECp-0841,  0x0.76A9782AB165E0p-0893 },   // 8.0e-0254
        { 0x1.51D69807B8DCAp-0841,  0x0.057EA7300792A0p-0893 },   // 9.0e-0254
        { 0x1.77603725064A7p-0841,  0x0.9453D6355DBF60p-0893 },   // 1.0e-0253
        { 0x1.77603725064A7p-0840,  0x0.9453D6355DBF60p-0892 },   // 2.0e-0253
        { 0x1.1988295BC4B7Dp-0839,  0x0.AF3EE0A8064F88p-0891 },   // 3.0e-0253
        { 0x1.77603725064A7p-0839,  0x0.9453D6355DBF60p-0891 },   // 4.0e-0253
        { 0x1.D53844EE47DD1p-0839,  0x0.7968CBC2B52F38p-0891 },   // 5.0e-0253
        { 0x1.1988295BC4B7Dp-0838,  0x0.AF3EE0A8064F88p-0890 },   // 6.0e-0253
        { 0x1.4874304065812p-0838,  0x0.A1C95B6EB20770p-0890 },   // 7.0e-0253
        { 0x1.77603725064A7p-0838,  0x0.9453D6355DBF60p-0890 },   // 8.0e-0253
        { 0x1.A64C3E09A713Cp-0838,  0x0.86DE50FC097748p-0890 },   // 9.0e-0253
        { 0x1.D53844EE47DD1p-0838,  0x0.7968CBC2B52F38p-0890 },   // 1.0e-0252
        { 0x1.D53844EE47DD1p-0837,  0x0.7968CBC2B52F38p-0889 },   // 2.0e-0252
        { 0x1.5FEA33B2B5E5Dp-0836,  0x0.1B0E98D207E368p-0888 },   // 3.0e-0252
        { 0x1.D53844EE47DD1p-0836,  0x0.7968CBC2B52F38p-0888 },   // 4.0e-0252
        { 0x1.25432B14ECEA2p-0835,  0x0.EBE17F59B13D80p-0887 },   // 5.0e-0252
        { 0x1.5FEA33B2B5E5Dp-0835,  0x0.1B0E98D207E368p-0887 },   // 6.0e-0252
        { 0x1.9A913C507EE17p-0835,  0x0.4A3BB24A5E8950p-0887 },   // 7.0e-0252
        { 0x1.D53844EE47DD1p-0835,  0x0.7968CBC2B52F38p-0887 },   // 8.0e-0252
        { 0x1.07EFA6C6086C5p-0834,  0x0.D44AF29D85EA88p-0886 },   // 9.0e-0252
        { 0x1.25432B14ECEA2p-0834,  0x0.EBE17F59B13D80p-0886 },   // 1.0e-0251
        { 0x1.25432B14ECEA2p-0833,  0x0.EBE17F59B13D80p-0885 },   // 2.0e-0251
        { 0x1.B7E4C09F635F4p-0833,  0x0.61D23F0689DC40p-0885 },   // 3.0e-0251
        { 0x1.25432B14ECEA2p-0832,  0x0.EBE17F59B13D80p-0884 },   // 4.0e-0251
        { 0x1.6E93F5DA2824Bp-0832,  0x0.A6D9DF301D8CE0p-0884 },   // 5.0e-0251
        { 0x1.B7E4C09F635F4p-0832,  0x0.61D23F0689DC40p-0884 },   // 6.0e-0251
        { 0x1.009AC5B24F4CEp-0831,  0x0.8E654F6E7B15D0p-0883 },   // 7.0e-0251
        { 0x1.25432B14ECEA2p-0831,  0x0.EBE17F59B13D80p-0883 },   // 8.0e-0251
        { 0x1.49EB90778A877p-0831,  0x0.495DAF44E76530p-0883 },   // 9.0e-0251
        { 0x1.6E93F5DA2824Bp-0831,  0x0.A6D9DF301D8CE0p-0883 },   // 1.0e-0250
        { 0x1.6E93F5DA2824Bp-0830,  0x0.A6D9DF301D8CE0p-0882 },   // 2.0e-0250
        { 0x1.12EEF8639E1B8p-0829,  0x0.BD2367641629A8p-0881 },   // 3.0e-0250
        { 0x1.6E93F5DA2824Bp-0829,  0x0.A6D9DF301D8CE0p-0881 },   // 4.0e-0250
        { 0x1.CA38F350B22DEp-0829,  0x0.909056FC24F018p-0881 },   // 5.0e-0250
        { 0x1.12EEF8639E1B8p-0828,  0x0.BD2367641629A8p-0880 },   // 6.0e-0250
        { 0x1.40C1771EE3202p-0828,  0x0.31FEA34A19DB40p-0880 },   // 7.0e-0250
        { 0x1.6E93F5DA2824Bp-0828,  0x0.A6D9DF301D8CE0p-0880 },   // 8.0e-0250
        { 0x1.9C6674956D295p-0828,  0x0.1BB51B16213E80p-0880 },   // 9.0e-0250
        { 0x1.CA38F350B22DEp-0828,  0x0.909056FC24F018p-0880 },   // 1.0e-0249
        { 0x1.CA38F350B22DEp-0827,  0x0.909056FC24F018p-0879 },   // 2.0e-0249
        { 0x1.57AAB67C85A26p-0826,  0x0.EC6C413D1BB410p-0878 },   // 3.0e-0249
        { 0x1.CA38F350B22DEp-0826,  0x0.909056FC24F018p-0878 },   // 4.0e-0249
        { 0x1.1E6398126F5CBp-0825,  0x0.1A5A365D971610p-0877 },   // 5.0e-0249
        { 0x1.57AAB67C85A26p-0825,  0x0.EC6C413D1BB410p-0877 },   // 6.0e-0249
        { 0x1.90F1D4E69BE82p-0825,  0x0.BE7E4C1CA05218p-0877 },   // 7.0e-0249
        { 0x1.CA38F350B22DEp-0825,  0x0.909056FC24F018p-0877 },   // 8.0e-0249
        { 0x1.01C008DD6439Dp-0824,  0x0.315130EDD4C710p-0876 },   // 9.0e-0249
        { 0x1.1E6398126F5CBp-0824,  0x0.1A5A365D971610p-0876 },   // 1.0e-0248
        { 0x1.1E6398126F5CBp-0823,  0x0.1A5A365D971610p-0875 },   // 2.0e-0248
        { 0x1.AD95641BA70B0p-0823,  0x0.A787518C62A118p-0875 },   // 3.0e-0248
        { 0x1.1E6398126F5CBp-0822,  0x0.1A5A365D971610p-0874 },   // 4.0e-0248
        { 0x1.65FC7E170B33Dp-0822,  0x0.E0F0C3F4FCDB90p-0874 },   // 5.0e-0248
        { 0x1.AD95641BA70B0p-0822,  0x0.A787518C62A118p-0874 },   // 6.0e-0248
        { 0x1.F52E4A2042E23p-0822,  0x0.6E1DDF23C86698p-0874 },   // 7.0e-0248
        { 0x1.1E6398126F5CBp-0821,  0x0.1A5A365D971610p-0873 },   // 8.0e-0248
        { 0x1.42300B14BD484p-0821,  0x0.7DA57D2949F8D0p-0873 },   // 9.0e-0248
        { 0x1.65FC7E170B33Dp-0821,  0x0.E0F0C3F4FCDB90p-0873 },   // 1.0e-0247
        { 0x1.65FC7E170B33Dp-0820,  0x0.E0F0C3F4FCDB90p-0872 },   // 2.0e-0247
        { 0x1.0C7D5E914866Ep-0819,  0x0.68B492F7BDA4B0p-0871 },   // 3.0e-0247
        { 0x1.65FC7E170B33Dp-0819,  0x0.E0F0C3F4FCDB90p-0871 },   // 4.0e-0247
        { 0x1.BF7B9D9CCE00Dp-0819,  0x0.592CF4F23C1278p-0871 },   // 5.0e-0247
        { 0x1.0C7D5E914866Ep-0818,  0x0.68B492F7BDA4B0p-0870 },   // 6.0e-0247
        { 0x1.393CEE5429CD6p-0818,  0x0.24D2AB765D4020p-0870 },   // 7.0e-0247
        { 0x1.65FC7E170B33Dp-0818,  0x0.E0F0C3F4FCDB90p-0870 },   // 8.0e-0247
        { 0x1.92BC0DD9EC9A5p-0818,  0x0.9D0EDC739C7708p-0870 },   // 9.0e-0247
        { 0x1.BF7B9D9CCE00Dp-0818,  0x0.592CF4F23C1278p-0870 },   // 1.0e-0246
        { 0x1.BF7B9D9CCE00Dp-0817,  0x0.592CF4F23C1278p-0869 },   // 2.0e-0246
        { 0x1.4F9CB6359A80Ap-0816,  0x0.02E1B7B5AD0DD8p-0868 },   // 3.0e-0246
        { 0x1.BF7B9D9CCE00Dp-0816,  0x0.592CF4F23C1278p-0868 },   // 4.0e-0246
        { 0x1.17AD428200C08p-0815,  0x0.57BC1917658B88p-0867 },   // 5.0e-0246
        { 0x1.4F9CB6359A80Ap-0815,  0x0.02E1B7B5AD0DD8p-0867 },   // 6.0e-0246
        { 0x1.878C29E93440Bp-0815,  0x0.AE075653F49028p-0867 },   // 7.0e-0246
        { 0x1.BF7B9D9CCE00Dp-0815,  0x0.592CF4F23C1278p-0867 },   // 8.0e-0246
        { 0x1.F76B115067C0Fp-0815,  0x0.045293908394C8p-0867 },   // 9.0e-0246
        { 0x1.17AD428200C08p-0814,  0x0.57BC1917658B88p-0866 },   // 1.0e-0245
        { 0x1.17AD428200C08p-0813,  0x0.57BC1917658B88p-0865 },   // 2.0e-0245
        { 0x1.A383E3C30120Cp-0813,  0x0.839A25A3185150p-0865 },   // 3.0e-0245
        { 0x1.17AD428200C08p-0812,  0x0.57BC1917658B88p-0864 },   // 4.0e-0245
        { 0x1.5D98932280F0Ap-0812,  0x0.6DAB1F5D3EEE70p-0864 },   // 5.0e-0245
        { 0x1.A383E3C30120Cp-0812,  0x0.839A25A3185150p-0864 },   // 6.0e-0245
        { 0x1.E96F34638150Ep-0812,  0x0.99892BE8F1B430p-0864 },   // 7.0e-0245
        { 0x1.17AD428200C08p-0811,  0x0.57BC1917658B88p-0863 },   // 8.0e-0245
        { 0x1.3AA2EAD240D89p-0811,  0x0.62B39C3A523CF8p-0863 },   // 9.0e-0245
        { 0x1.5D98932280F0Ap-0811,  0x0.6DAB1F5D3EEE70p-0863 },   // 1.0e-0244
        { 0x1.5D98932280F0Ap-0810,  0x0.6DAB1F5D3EEE70p-0862 },   // 2.0e-0244
        { 0x1.06326E59E0B47p-0809,  0x0.D2405785EF32D0p-0861 },   // 3.0e-0244
        { 0x1.5D98932280F0Ap-0809,  0x0.6DAB1F5D3EEE70p-0861 },   // 4.0e-0244
        { 0x1.B4FEB7EB212CDp-0809,  0x0.0915E7348EAA08p-0861 },   // 5.0e-0244
        { 0x1.06326E59E0B47p-0808,  0x0.D2405785EF32D0p-0860 },   // 6.0e-0244
        { 0x1.31E580BE30D29p-0808,  0x0.1FF5BB719710A0p-0860 },   // 7.0e-0244
        { 0x1.5D98932280F0Ap-0808,  0x0.6DAB1F5D3EEE70p-0860 },   // 8.0e-0244
        { 0x1.894BA586D10EBp-0808,  0x0.BB608348E6CC38p-0860 },   // 9.0e-0244
        { 0x1.B4FEB7EB212CDp-0808,  0x0.0915E7348EAA08p-0860 },   // 1.0e-0243
        { 0x1.B4FEB7EB212CDp-0807,  0x0.0915E7348EAA08p-0859 },   // 2.0e-0243
        { 0x1.47BF09F058E19p-0806,  0x0.C6D06D676AFF88p-0858 },   // 3.0e-0243
        { 0x1.B4FEB7EB212CDp-0806,  0x0.0915E7348EAA08p-0858 },   // 4.0e-0243
        { 0x1.111F32F2F4BC0p-0805,  0x0.25ADB080D92A48p-0857 },   // 5.0e-0243
        { 0x1.47BF09F058E19p-0805,  0x0.C6D06D676AFF88p-0857 },   // 6.0e-0243
        { 0x1.7E5EE0EDBD073p-0805,  0x0.67F32A4DFCD4C8p-0857 },   // 7.0e-0243
        { 0x1.B4FEB7EB212CDp-0805,  0x0.0915E7348EAA08p-0857 },   // 8.0e-0243
        { 0x1.EB9E8EE885526p-0805,  0x0.AA38A41B207F48p-0857 },   // 9.0e-0243
        { 0x1.111F32F2F4BC0p-0804,  0x0.25ADB080D92A48p-0856 },   // 1.0e-0242
        { 0x1.111F32F2F4BC0p-0803,  0x0.25ADB080D92A48p-0855 },   // 2.0e-0242
        { 0x1.99AECC6C6F1A0p-0803,  0x0.388488C145BF68p-0855 },   // 3.0e-0242
        { 0x1.111F32F2F4BC0p-0802,  0x0.25ADB080D92A48p-0854 },   // 4.0e-0242
        { 0x1.5566FFAFB1EB0p-0802,  0x0.2F191CA10F74D8p-0854 },   // 5.0e-0242
        { 0x1.99AECC6C6F1A0p-0802,  0x0.388488C145BF68p-0854 },   // 6.0e-0242
        { 0x1.DDF699292C490p-0802,  0x0.41EFF4E17C09F8p-0854 },   // 7.0e-0242
        { 0x1.111F32F2F4BC0p-0801,  0x0.25ADB080D92A48p-0853 },   // 8.0e-0242
        { 0x1.3343195153538p-0801,  0x0.2A636690F44F90p-0853 },   // 9.0e-0242
        { 0x1.5566FFAFB1EB0p-0801,  0x0.2F191CA10F74D8p-0853 },   // 1.0e-0241
        { 0x1.5566FFAFB1EB0p-0800,  0x0.2F191CA10F74D8p-0852 },   // 2.0e-0241
        { 0x1.000D3FC3C5704p-0799,  0x0.2352D578CB97A0p-0851 },   // 3.0e-0241
        { 0x1.5566FFAFB1EB0p-0799,  0x0.2F191CA10F74D8p-0851 },   // 4.0e-0241
        { 0x1.AAC0BF9B9E65Cp-0799,  0x0.3ADF63C9535210p-0851 },   // 5.0e-0241
        { 0x1.000D3FC3C5704p-0798,  0x0.2352D578CB97A0p-0850 },   // 6.0e-0241
        { 0x1.2ABA1FB9BBADAp-0798,  0x0.2935F90CED8638p-0850 },   // 7.0e-0241
        { 0x1.5566FFAFB1EB0p-0798,  0x0.2F191CA10F74D8p-0850 },   // 8.0e-0241
        { 0x1.8013DFA5A8286p-0798,  0x0.34FC4035316370p-0850 },   // 9.0e-0241
        { 0x1.AAC0BF9B9E65Cp-0798,  0x0.3ADF63C9535210p-0850 },   // 1.0e-0240
        { 0x1.AAC0BF9B9E65Cp-0797,  0x0.3ADF63C9535210p-0849 },   // 2.0e-0240
        { 0x1.40108FB4B6CC5p-0796,  0x0.2C278AD6FE7D88p-0848 },   // 3.0e-0240
        { 0x1.AAC0BF9B9E65Cp-0796,  0x0.3ADF63C9535210p-0848 },   // 4.0e-0240
        { 0x1.0AB877C142FF9p-0795,  0x0.A4CB9E5DD41348p-0847 },   // 5.0e-0240
        { 0x1.40108FB4B6CC5p-0795,  0x0.2C278AD6FE7D88p-0847 },   // 6.0e-0240
        { 0x1.7568A7A82A990p-0795,  0x0.B383775028E7C8p-0847 },   // 7.0e-0240
        { 0x1.AAC0BF9B9E65Cp-0795,  0x0.3ADF63C9535210p-0847 },   // 8.0e-0240
        { 0x1.E018D78F12327p-0795,  0x0.C23B50427DBC50p-0847 },   // 9.0e-0240
        { 0x1.0AB877C142FF9p-0794,  0x0.A4CB9E5DD41348p-0846 },   // 1.0e-0239
        { 0x1.0AB877C142FF9p-0793,  0x0.A4CB9E5DD41348p-0845 },   // 2.0e-0239
        { 0x1.9014B3A1E47F6p-0793,  0x0.77316D8CBE1CE8p-0845 },   // 3.0e-0239
        { 0x1.0AB877C142FF9p-0792,  0x0.A4CB9E5DD41348p-0844 },   // 4.0e-0239
        { 0x1.4D6695B193BF8p-0792,  0x0.0DFE85F5491818p-0844 },   // 5.0e-0239
        { 0x1.9014B3A1E47F6p-0792,  0x0.77316D8CBE1CE8p-0844 },   // 6.0e-0239
        { 0x1.D2C2D192353F4p-0792,  0x0.E06455243321C0p-0844 },   // 7.0e-0239
        { 0x1.0AB877C142FF9p-0791,  0x0.A4CB9E5DD41348p-0843 },   // 8.0e-0239
        { 0x1.2C0F86B96B5F8p-0791,  0x0.D96512298E95B0p-0843 },   // 9.0e-0239
        { 0x1.4D6695B193BF8p-0791,  0x0.0DFE85F5491818p-0843 },   // 1.0e-0238
        { 0x1.4D6695B193BF8p-0790,  0x0.0DFE85F5491818p-0842 },   // 2.0e-0238
        { 0x1.F419E08A5D9F4p-0790,  0x0.14FDC8EFEDA428p-0842 },   // 3.0e-0238
        { 0x1.4D6695B193BF8p-0789,  0x0.0DFE85F5491818p-0841 },   // 4.0e-0238
        { 0x1.A0C03B1DF8AF6p-0789,  0x0.117E27729B5E20p-0841 },   // 5.0e-0238
        { 0x1.F419E08A5D9F4p-0789,  0x0.14FDC8EFEDA428p-0841 },   // 6.0e-0238
        { 0x1.23B9C2FB61479p-0788,  0x0.0C3EB5369FF518p-0840 },   // 7.0e-0238
        { 0x1.4D6695B193BF8p-0788,  0x0.0DFE85F5491818p-0840 },   // 8.0e-0238
        { 0x1.77136867C6377p-0788,  0x0.0FBE56B3F23B20p-0840 },   // 9.0e-0238
        { 0x1.A0C03B1DF8AF6p-0788,  0x0.117E27729B5E20p-0840 },   // 1.0e-0237
        { 0x1.A0C03B1DF8AF6p-0787,  0x0.117E27729B5E20p-0839 },   // 2.0e-0237
        { 0x1.38902C567A838p-0786,  0x0.8D1E9D95F48698p-0838 },   // 3.0e-0237
        { 0x1.A0C03B1DF8AF6p-0786,  0x0.117E27729B5E20p-0838 },   // 4.0e-0237
        { 0x1.047824F2BB6D9p-0785,  0x0.CAEED8A7A11AD0p-0837 },   // 5.0e-0237
        { 0x1.38902C567A838p-0785,  0x0.8D1E9D95F48698p-0837 },   // 6.0e-0237
        { 0x1.6CA833BA39997p-0785,  0x0.4F4E628447F260p-0837 },   // 7.0e-0237
        { 0x1.A0C03B1DF8AF6p-0785,  0x0.117E27729B5E20p-0837 },   // 8.0e-0237
        { 0x1.D4D84281B7C54p-0785,  0x0.D3ADEC60EEC9E8p-0837 },   // 9.0e-0237
        { 0x1.047824F2BB6D9p-0784,  0x0.CAEED8A7A11AD0p-0836 },   // 1.0e-0236
        { 0x1.047824F2BB6D9p-0783,  0x0.CAEED8A7A11AD0p-0835 },   // 2.0e-0236
        { 0x1.86B4376C19246p-0783,  0x0.B06644FB71A840p-0835 },   // 3.0e-0236
        { 0x1.047824F2BB6D9p-0782,  0x0.CAEED8A7A11AD0p-0834 },   // 4.0e-0236
        { 0x1.45962E2F6A490p-0782,  0x0.3DAA8ED1896188p-0834 },   // 5.0e-0236
        { 0x1.86B4376C19246p-0782,  0x0.B06644FB71A840p-0834 },   // 6.0e-0236
        { 0x1.C7D240A8C7FFDp-0782,  0x0.2321FB2559EEF8p-0834 },   // 7.0e-0236
        { 0x1.047824F2BB6D9p-0781,  0x0.CAEED8A7A11AD0p-0833 },   // 8.0e-0236
        { 0x1.2507299112DB5p-0781,  0x0.044CB3BC953E30p-0833 },   // 9.0e-0236
        { 0x1.45962E2F6A490p-0781,  0x0.3DAA8ED1896188p-0833 },   // 1.0e-0235
        { 0x1.45962E2F6A490p-0780,  0x0.3DAA8ED1896188p-0832 },   // 2.0e-0235
        { 0x1.E86145471F6D8p-0780,  0x0.5C7FD63A4E1250p-0832 },   // 3.0e-0235
        { 0x1.45962E2F6A490p-0779,  0x0.3DAA8ED1896188p-0831 },   // 4.0e-0235
        { 0x1.96FBB9BB44DB4p-0779,  0x0.4D153285EBB9E8p-0831 },   // 5.0e-0235
        { 0x1.E86145471F6D8p-0779,  0x0.5C7FD63A4E1250p-0831 },   // 6.0e-0235
        { 0x1.1CE368697CFFEp-0778,  0x0.35F53CF7583558p-0830 },   // 7.0e-0235
        { 0x1.45962E2F6A490p-0778,  0x0.3DAA8ED1896188p-0830 },   // 8.0e-0235
        { 0x1.6E48F3F557922p-0778,  0x0.455FE0ABBA8DB8p-0830 },   // 9.0e-0235
        { 0x1.96FBB9BB44DB4p-0778,  0x0.4D153285EBB9E8p-0830 },   // 1.0e-0234
        { 0x1.96FBB9BB44DB4p-0777,  0x0.4D153285EBB9E8p-0829 },   // 2.0e-0234
        { 0x1.313CCB4C73A47p-0776,  0x0.39CFE5E470CB70p-0828 },   // 3.0e-0234
        { 0x1.96FBB9BB44DB4p-0776,  0x0.4D153285EBB9E8p-0828 },   // 4.0e-0234
        { 0x1.FCBAA82A16121p-0776,  0x0.605A7F2766A868p-0828 },   // 5.0e-0234
        { 0x1.313CCB4C73A47p-0775,  0x0.39CFE5E470CB70p-0827 },   // 6.0e-0234
        { 0x1.641C4283DC3FDp-0775,  0x0.C3728C352E42B0p-0827 },   // 7.0e-0234
        { 0x1.96FBB9BB44DB4p-0775,  0x0.4D153285EBB9E8p-0827 },   // 8.0e-0234
        { 0x1.C9DB30F2AD76Ap-0775,  0x0.D6B7D8D6A93128p-0827 },   // 9.0e-0234
        { 0x1.FCBAA82A16121p-0775,  0x0.605A7F2766A868p-0827 },   // 1.0e-0233
        { 0x1.FCBAA82A16121p-0774,  0x0.605A7F2766A868p-0826 },   // 2.0e-0233
        { 0x1.7D8BFE1F908D9p-0773,  0x0.0843DF5D8CFE50p-0825 },   // 3.0e-0233
        { 0x1.FCBAA82A16121p-0773,  0x0.605A7F2766A868p-0825 },   // 4.0e-0233
        { 0x1.3DF4A91A4DCB4p-0772,  0x0.DC388F78A02940p-0824 },   // 5.0e-0233
        { 0x1.7D8BFE1F908D9p-0772,  0x0.0843DF5D8CFE50p-0824 },   // 6.0e-0233
        { 0x1.BD235324D34FDp-0772,  0x0.344F2F4279D358p-0824 },   // 7.0e-0233
        { 0x1.FCBAA82A16121p-0772,  0x0.605A7F2766A868p-0824 },   // 8.0e-0233
        { 0x1.1E28FE97AC6A2p-0771,  0x0.C632E78629BEB8p-0823 },   // 9.0e-0233
        { 0x1.3DF4A91A4DCB4p-0771,  0x0.DC388F78A02940p-0823 },   // 1.0e-0232
        { 0x1.3DF4A91A4DCB4p-0770,  0x0.DC388F78A02940p-0822 },   // 2.0e-0232
        { 0x1.DCEEFDA774B0Fp-0770,  0x0.4A54D734F03DE0p-0822 },   // 3.0e-0232
        { 0x1.3DF4A91A4DCB4p-0769,  0x0.DC388F78A02940p-0821 },   // 4.0e-0232
        { 0x1.8D71D360E13E2p-0769,  0x0.1346B356C83390p-0821 },   // 5.0e-0232
        { 0x1.DCEEFDA774B0Fp-0769,  0x0.4A54D734F03DE0p-0821 },   // 6.0e-0232
        { 0x1.163613F70411Ep-0768,  0x0.40B17D898C2418p-0820 },   // 7.0e-0232
        { 0x1.3DF4A91A4DCB4p-0768,  0x0.DC388F78A02940p-0820 },   // 8.0e-0232
        { 0x1.65B33E3D9784Bp-0768,  0x0.77BFA167B42E68p-0820 },   // 9.0e-0232
        { 0x1.8D71D360E13E2p-0768,  0x0.1346B356C83390p-0820 },   // 1.0e-0231
        { 0x1.8D71D360E13E2p-0767,  0x0.1346B356C83390p-0819 },   // 2.0e-0231
        { 0x1.2A155E88A8EE9p-0766,  0x0.8E7506811626A8p-0818 },   // 3.0e-0231
        { 0x1.8D71D360E13E2p-0766,  0x0.1346B356C83390p-0818 },   // 4.0e-0231
        { 0x1.F0CE4839198DAp-0766,  0x0.9818602C7A4078p-0818 },   // 5.0e-0231
        { 0x1.2A155E88A8EE9p-0765,  0x0.8E7506811626A8p-0817 },   // 6.0e-0231
        { 0x1.5BC398F4C5165p-0765,  0x0.D0DDDCEBEF2D20p-0817 },   // 7.0e-0231
        { 0x1.8D71D360E13E2p-0765,  0x0.1346B356C83390p-0817 },   // 8.0e-0231
        { 0x1.BF200DCCFD65Ep-0765,  0x0.55AF89C1A13A00p-0817 },   // 9.0e-0231
        { 0x1.F0CE4839198DAp-0765,  0x0.9818602C7A4078p-0817 },   // 1.0e-0230
        { 0x1.F0CE4839198DAp-0764,  0x0.9818602C7A4078p-0816 },   // 2.0e-0230
        { 0x1.749AB62AD32A3p-0763,  0x0.F21248215BB058p-0815 },   // 3.0e-0230
        { 0x1.F0CE4839198DAp-0763,  0x0.9818602C7A4078p-0815 },   // 4.0e-0230
        { 0x1.3680ED23AFF88p-0762,  0x0.9F0F3C1BCC6848p-0814 },   // 5.0e-0230
        { 0x1.749AB62AD32A3p-0762,  0x0.F21248215BB058p-0814 },   // 6.0e-0230
        { 0x1.B2B47F31F65BFp-0762,  0x0.45155426EAF868p-0814 },   // 7.0e-0230
        { 0x1.F0CE4839198DAp-0762,  0x0.9818602C7A4078p-0814 },   // 8.0e-0230
        { 0x1.177408A01E5FAp-0761,  0x0.F58DB61904C440p-0813 },   // 9.0e-0230
        { 0x1.3680ED23AFF88p-0761,  0x0.9F0F3C1BCC6848p-0813 },   // 1.0e-0229
        { 0x1.3680ED23AFF88p-0760,  0x0.9F0F3C1BCC6848p-0812 },   // 2.0e-0229
        { 0x1.D1C163B587F4Cp-0760,  0x0.EE96DA29B29C70p-0812 },   // 3.0e-0229
        { 0x1.3680ED23AFF88p-0759,  0x0.9F0F3C1BCC6848p-0811 },   // 4.0e-0229
        { 0x1.8421286C9BF6Ap-0759,  0x0.C6D30B22BF8258p-0811 },   // 5.0e-0229
        { 0x1.D1C163B587F4Cp-0759,  0x0.EE96DA29B29C70p-0811 },   // 6.0e-0229
        { 0x1.0FB0CF7F39F97p-0758,  0x0.8B2D549852DB40p-0810 },   // 7.0e-0229
        { 0x1.3680ED23AFF88p-0758,  0x0.9F0F3C1BCC6848p-0810 },   // 8.0e-0229
        { 0x1.5D510AC825F79p-0758,  0x0.B2F1239F45F550p-0810 },   // 9.0e-0229
        { 0x1.8421286C9BF6Ap-0758,  0x0.C6D30B22BF8258p-0810 },   // 1.0e-0228
        { 0x1.8421286C9BF6Ap-0757,  0x0.C6D30B22BF8258p-0809 },   // 2.0e-0228
        { 0x1.2318DE5174F90p-0756,  0x0.151E485A0FA1C0p-0808 },   // 3.0e-0228
        { 0x1.8421286C9BF6Ap-0756,  0x0.C6D30B22BF8258p-0808 },   // 4.0e-0228
        { 0x1.E5297287C2F45p-0756,  0x0.7887CDEB6F62F0p-0808 },   // 5.0e-0228
        { 0x1.2318DE5174F90p-0755,  0x0.151E485A0FA1C0p-0807 },   // 6.0e-0228
        { 0x1.539D035F0877Dp-0755,  0x0.6DF8A9BE679210p-0807 },   // 7.0e-0228
        { 0x1.8421286C9BF6Ap-0755,  0x0.C6D30B22BF8258p-0807 },   // 8.0e-0228
        { 0x1.B4A54D7A2F758p-0755,  0x0.1FAD6C871772A8p-0807 },   // 9.0e-0228
        { 0x1.E5297287C2F45p-0755,  0x0.7887CDEB6F62F0p-0807 },   // 1.0e-0227
        { 0x1.E5297287C2F45p-0754,  0x0.7887CDEB6F62F0p-0806 },   // 2.0e-0227
        { 0x1.6BDF15E5D2374p-0753,  0x0.1A65DA70938A38p-0805 },   // 3.0e-0227
        { 0x1.E5297287C2F45p-0753,  0x0.7887CDEB6F62F0p-0805 },   // 4.0e-0227
        { 0x1.2F39E794D9D8Bp-0752,  0x0.6B54E0B3259DD8p-0804 },   // 5.0e-0227
        { 0x1.6BDF15E5D2374p-0752,  0x0.1A65DA70938A38p-0804 },   // 6.0e-0227
        { 0x1.A8844436CA95Cp-0752,  0x0.C976D42E017690p-0804 },   // 7.0e-0227
        { 0x1.E5297287C2F45p-0752,  0x0.7887CDEB6F62F0p-0804 },   // 8.0e-0227
        { 0x1.10E7506C5DA97p-0751,  0x0.13CC63D46EA7A8p-0803 },   // 9.0e-0227
        { 0x1.2F39E794D9D8Bp-0751,  0x0.6B54E0B3259DD8p-0803 },   // 1.0e-0226
        { 0x1.2F39E794D9D8Bp-0750,  0x0.6B54E0B3259DD8p-0802 },   // 2.0e-0226
        { 0x1.C6D6DB5F46C51p-0750,  0x0.20FF510CB86CC0p-0802 },   // 3.0e-0226
        { 0x1.2F39E794D9D8Bp-0749,  0x0.6B54E0B3259DD8p-0801 },   // 4.0e-0226
        { 0x1.7B08617A104EEp-0749,  0x0.462A18DFEF0550p-0801 },   // 5.0e-0226
        { 0x1.C6D6DB5F46C51p-0749,  0x0.20FF510CB86CC0p-0801 },   // 6.0e-0226
        { 0x1.0952AAA23E9D9p-0748,  0x0.FDEA449CC0EA18p-0800 },   // 7.0e-0226
        { 0x1.2F39E794D9D8Bp-0748,  0x0.6B54E0B3259DD8p-0800 },   // 8.0e-0226
        { 0x1.552124877513Cp-0748,  0x0.D8BF7CC98A5190p-0800 },   // 9.0e-0226
        { 0x1.7B08617A104EEp-0748,  0x0.462A18DFEF0550p-0800 },   // 1.0e-0225
        { 0x1.7B08617A104EEp-0747,  0x0.462A18DFEF0550p-0799 },   // 2.0e-0225
        { 0x1.1C46491B8C3B2p-0746,  0x0.B49F92A7F343F8p-0798 },   // 3.0e-0225
        { 0x1.7B08617A104EEp-0746,  0x0.462A18DFEF0550p-0798 },   // 4.0e-0225
        { 0x1.D9CA79D894629p-0746,  0x0.D7B49F17EAC6A0p-0798 },   // 5.0e-0225
        { 0x1.1C46491B8C3B2p-0745,  0x0.B49F92A7F343F8p-0797 },   // 6.0e-0225
        { 0x1.4BA7554ACE450p-0745,  0x0.7D64D5C3F124A0p-0797 },   // 7.0e-0225
        { 0x1.7B08617A104EEp-0745,  0x0.462A18DFEF0550p-0797 },   // 8.0e-0225
        { 0x1.AA696DA95258Cp-0745,  0x0.0EEF5BFBECE5F8p-0797 },   // 9.0e-0225
        { 0x1.D9CA79D894629p-0745,  0x0.D7B49F17EAC6A0p-0797 },   // 1.0e-0224
        { 0x1.D9CA79D894629p-0744,  0x0.D7B49F17EAC6A0p-0796 },   // 2.0e-0224
        { 0x1.6357DB626F49Fp-0743,  0x0.61C77751F014F8p-0795 },   // 3.0e-0224
        { 0x1.D9CA79D894629p-0743,  0x0.D7B49F17EAC6A0p-0795 },   // 4.0e-0224
        { 0x1.281E8C275CBDAp-0742,  0x0.26D0E36EF2BC20p-0794 },   // 5.0e-0224
        { 0x1.6357DB626F49Fp-0742,  0x0.61C77751F014F8p-0794 },   // 6.0e-0224
        { 0x1.9E912A9D81D64p-0742,  0x0.9CBE0B34ED6DD0p-0794 },   // 7.0e-0224
        { 0x1.D9CA79D894629p-0742,  0x0.D7B49F17EAC6A0p-0794 },   // 8.0e-0224
        { 0x1.0A81E489D3777p-0741,  0x0.8955997D740FB8p-0793 },   // 9.0e-0224
        { 0x1.281E8C275CBDAp-0741,  0x0.26D0E36EF2BC20p-0793 },   // 1.0e-0223
        { 0x1.281E8C275CBDAp-0740,  0x0.26D0E36EF2BC20p-0792 },   // 2.0e-0223
        { 0x1.BC2DD23B0B1C7p-0740,  0x0.3A3955266C1A38p-0792 },   // 3.0e-0223
        { 0x1.281E8C275CBDAp-0739,  0x0.26D0E36EF2BC20p-0791 },   // 4.0e-0223
        { 0x1.72262F3133ED0p-0739,  0x0.B0851C4AAF6B30p-0791 },   // 5.0e-0223
        { 0x1.BC2DD23B0B1C7p-0739,  0x0.3A3955266C1A38p-0791 },   // 6.0e-0223
        { 0x1.031ABAA27125Ep-0738,  0x0.E1F6C7011464A0p-0790 },   // 7.0e-0223
        { 0x1.281E8C275CBDAp-0738,  0x0.26D0E36EF2BC20p-0790 },   // 8.0e-0223
        { 0x1.4D225DAC48555p-0738,  0x0.6BAAFFDCD113A8p-0790 },   // 9.0e-0223
        { 0x1.72262F3133ED0p-0738,  0x0.B0851C4AAF6B30p-0790 },   // 1.0e-0222
        { 0x1.72262F3133ED0p-0737,  0x0.B0851C4AAF6B30p-0789 },   // 2.0e-0222
        { 0x1.159CA364E6F1Cp-0736,  0x0.8463D538039060p-0788 },   // 3.0e-0222
        { 0x1.72262F3133ED0p-0736,  0x0.B0851C4AAF6B30p-0788 },   // 4.0e-0222
        { 0x1.CEAFBAFD80E84p-0736,  0x0.DCA6635D5B45F8p-0788 },   // 5.0e-0222
        { 0x1.159CA364E6F1Cp-0735,  0x0.8463D538039060p-0787 },   // 6.0e-0222
        { 0x1.43E1694B0D6F6p-0735,  0x0.9A7478C1597DC8p-0787 },   // 7.0e-0222
        { 0x1.72262F3133ED0p-0735,  0x0.B0851C4AAF6B30p-0787 },   // 8.0e-0222
        { 0x1.A06AF5175A6AAp-0735,  0x0.C695BFD4055890p-0787 },   // 9.0e-0222
        { 0x1.CEAFBAFD80E84p-0735,  0x0.DCA6635D5B45F8p-0787 },   // 1.0e-0221
        { 0x1.CEAFBAFD80E84p-0734,  0x0.DCA6635D5B45F8p-0786 },   // 2.0e-0221
        { 0x1.5B03CC3E20AE3p-0733,  0x0.A57CCA86047478p-0785 },   // 3.0e-0221
        { 0x1.CEAFBAFD80E84p-0733,  0x0.DCA6635D5B45F8p-0785 },   // 4.0e-0221
        { 0x1.212DD4DE70913p-0732,  0x0.09E7FE1A590BB8p-0784 },   // 5.0e-0221
        { 0x1.5B03CC3E20AE3p-0732,  0x0.A57CCA86047478p-0784 },   // 6.0e-0221
        { 0x1.94D9C39DD0CB4p-0732,  0x0.411196F1AFDD38p-0784 },   // 7.0e-0221
        { 0x1.CEAFBAFD80E84p-0732,  0x0.DCA6635D5B45F8p-0784 },   // 8.0e-0221
        { 0x1.0442D92E9882Ap-0731,  0x0.BC1D97E4835758p-0783 },   // 9.0e-0221
        { 0x1.212DD4DE70913p-0731,  0x0.09E7FE1A590BB8p-0783 },   // 1.0e-0220
        { 0x1.212DD4DE70913p-0730,  0x0.09E7FE1A590BB8p-0782 },   // 2.0e-0220
        { 0x1.B1C4BF4DA8D9Cp-0730,  0x0.8EDBFD27859198p-0782 },   // 3.0e-0220
        { 0x1.212DD4DE70913p-0729,  0x0.09E7FE1A590BB8p-0781 },   // 4.0e-0220
        { 0x1.69794A160CB57p-0729,  0x0.CC61FDA0EF4EA8p-0781 },   // 5.0e-0220
        { 0x1.B1C4BF4DA8D9Cp-0729,  0x0.8EDBFD27859198p-0781 },   // 6.0e-0220
        { 0x1.FA10348544FE1p-0729,  0x0.5155FCAE1BD488p-0781 },   // 7.0e-0220
        { 0x1.212DD4DE70913p-0728,  0x0.09E7FE1A590BB8p-0780 },   // 8.0e-0220
        { 0x1.45538F7A3EA35p-0728,  0x0.6B24FDDDA42D30p-0780 },   // 9.0e-0220
        { 0x1.69794A160CB57p-0728,  0x0.CC61FDA0EF4EA8p-0780 },   // 1.0e-0219
        { 0x1.69794A160CB57p-0727,  0x0.CC61FDA0EF4EA8p-0779 },   // 2.0e-0219
        { 0x1.0F1AF79089881p-0726,  0x0.D9497E38B37B00p-0778 },   // 3.0e-0219
        { 0x1.69794A160CB57p-0726,  0x0.CC61FDA0EF4EA8p-0778 },   // 4.0e-0219
        { 0x1.C3D79C9B8FE2Dp-0726,  0x0.BF7A7D092B2258p-0778 },   // 5.0e-0219
        { 0x1.0F1AF79089881p-0725,  0x0.D9497E38B37B00p-0777 },   // 6.0e-0219
        { 0x1.3C4A20D34B1ECp-0725,  0x0.D2D5BDECD164D0p-0777 },   // 7.0e-0219
        { 0x1.69794A160CB57p-0725,  0x0.CC61FDA0EF4EA8p-0777 },   // 8.0e-0219
        { 0x1.96A87358CE4C2p-0725,  0x0.C5EE3D550D3880p-0777 },   // 9.0e-0219
        { 0x1.C3D79C9B8FE2Dp-0725,  0x0.BF7A7D092B2258p-0777 },   // 1.0e-0218
        { 0x1.C3D79C9B8FE2Dp-0724,  0x0.BF7A7D092B2258p-0776 },   // 2.0e-0218
        { 0x1.52E1B574ABEA2p-0723,  0x0.4F9BDDC6E059C0p-0775 },   // 3.0e-0218
        { 0x1.C3D79C9B8FE2Dp-0723,  0x0.BF7A7D092B2258p-0775 },   // 4.0e-0218
        { 0x1.1A66C1E139EDCp-0722,  0x0.97AC8E25BAF570p-0774 },   // 5.0e-0218
        { 0x1.52E1B574ABEA2p-0722,  0x0.4F9BDDC6E059C0p-0774 },   // 6.0e-0218
        { 0x1.8B5CA9081DE68p-0722,  0x0.078B2D6805BE08p-0774 },   // 7.0e-0218
        { 0x1.C3D79C9B8FE2Dp-0722,  0x0.BF7A7D092B2258p-0774 },   // 8.0e-0218
        { 0x1.FC52902F01DF3p-0722,  0x0.7769CCAA5086A0p-0774 },   // 9.0e-0218
        { 0x1.1A66C1E139EDCp-0721,  0x0.97AC8E25BAF570p-0773 },   // 1.0e-0217
        { 0x1.1A66C1E139EDCp-0720,  0x0.97AC8E25BAF570p-0772 },   // 2.0e-0217
        { 0x1.A79A22D1D6E4Ap-0720,  0x0.E382D538987030p-0772 },   // 3.0e-0217
        { 0x1.1A66C1E139EDCp-0719,  0x0.97AC8E25BAF570p-0771 },   // 4.0e-0217
        { 0x1.6100725988693p-0719,  0x0.BD97B1AF29B2D0p-0771 },   // 5.0e-0217
        { 0x1.A79A22D1D6E4Ap-0719,  0x0.E382D538987030p-0771 },   // 6.0e-0217
        { 0x1.EE33D34A25602p-0719,  0x0.096DF8C2072D90p-0771 },   // 7.0e-0217
        { 0x1.1A66C1E139EDCp-0718,  0x0.97AC8E25BAF570p-0770 },   // 8.0e-0217
        { 0x1.3DB39A1D612B8p-0718,  0x0.2AA21FEA725420p-0770 },   // 9.0e-0217
        { 0x1.6100725988693p-0718,  0x0.BD97B1AF29B2D0p-0770 },   // 1.0e-0216
        { 0x1.6100725988693p-0717,  0x0.BD97B1AF29B2D0p-0769 },   // 2.0e-0216
        { 0x1.08C055C3264EEp-0716,  0x0.CE31C5435F4620p-0768 },   // 3.0e-0216
        { 0x1.6100725988693p-0716,  0x0.BD97B1AF29B2D0p-0768 },   // 4.0e-0216
        { 0x1.B9408EEFEA838p-0716,  0x0.ACFD9E1AF41F88p-0768 },   // 5.0e-0216
        { 0x1.08C055C3264EEp-0715,  0x0.CE31C5435F4620p-0767 },   // 6.0e-0216
        { 0x1.34E0640E575C1p-0715,  0x0.45E4BB79447C78p-0767 },   // 7.0e-0216
        { 0x1.6100725988693p-0715,  0x0.BD97B1AF29B2D0p-0767 },   // 8.0e-0216
        { 0x1.8D2080A4B9766p-0715,  0x0.354AA7E50EE930p-0767 },   // 9.0e-0216
        { 0x1.B9408EEFEA838p-0715,  0x0.ACFD9E1AF41F88p-0767 },   // 1.0e-0215
        { 0x1.B9408EEFEA838p-0714,  0x0.ACFD9E1AF41F88p-0766 },   // 2.0e-0215
        { 0x1.4AF06B33EFE2Ap-0713,  0x0.81BE36943717A8p-0765 },   // 3.0e-0215
        { 0x1.B9408EEFEA838p-0713,  0x0.ACFD9E1AF41F88p-0765 },   // 4.0e-0215
        { 0x1.13C85955F2923p-0712,  0x0.6C1E82D0D893B0p-0764 },   // 5.0e-0215
        { 0x1.4AF06B33EFE2Ap-0712,  0x0.81BE36943717A8p-0764 },   // 6.0e-0215
        { 0x1.82187D11ED331p-0712,  0x0.975DEA57959B98p-0764 },   // 7.0e-0215
        { 0x1.B9408EEFEA838p-0712,  0x0.ACFD9E1AF41F88p-0764 },   // 8.0e-0215
        { 0x1.F068A0CDE7D3Fp-0712,  0x0.C29D51DE52A378p-0764 },   // 9.0e-0215
        { 0x1.13C85955F2923p-0711,  0x0.6C1E82D0D893B0p-0763 },   // 1.0e-0214
        { 0x1.13C85955F2923p-0710,  0x0.6C1E82D0D893B0p-0762 },   // 2.0e-0214
        { 0x1.9DAC8600EBDB5p-0710,  0x0.222DC43944DD90p-0762 },   // 3.0e-0214
        { 0x1.13C85955F2923p-0709,  0x0.6C1E82D0D893B0p-0761 },   // 4.0e-0214
        { 0x1.58BA6FAB6F36Cp-0709,  0x0.472623850EB8A0p-0761 },   // 5.0e-0214
        { 0x1.9DAC8600EBDB5p-0709,  0x0.222DC43944DD90p-0761 },   // 6.0e-0214
        { 0x1.E29E9C56687FDp-0709,  0x0.FD3564ED7B0278p-0761 },   // 7.0e-0214
        { 0x1.13C85955F2923p-0708,  0x0.6C1E82D0D893B0p-0760 },   // 8.0e-0214
        { 0x1.36416480B0E47p-0708,  0x0.D9A2532AF3A628p-0760 },   // 9.0e-0214
        { 0x1.58BA6FAB6F36Cp-0708,  0x0.472623850EB8A0p-0760 },   // 1.0e-0213
        { 0x1.58BA6FAB6F36Cp-0707,  0x0.472623850EB8A0p-0759 },   // 2.0e-0213
        { 0x1.028BD3C093691p-0706,  0x0.355C9AA3CB0A78p-0758 },   // 3.0e-0213
        { 0x1.58BA6FAB6F36Cp-0706,  0x0.472623850EB8A0p-0758 },   // 4.0e-0213
        { 0x1.AEE90B964B047p-0706,  0x0.58EFAC665266C8p-0758 },   // 5.0e-0213
        { 0x1.028BD3C093691p-0705,  0x0.355C9AA3CB0A78p-0757 },   // 6.0e-0213
        { 0x1.2DA321B6014FEp-0705,  0x0.BE415F146CE188p-0757 },   // 7.0e-0213
        { 0x1.58BA6FAB6F36Cp-0705,  0x0.472623850EB8A0p-0757 },   // 8.0e-0213
        { 0x1.83D1BDA0DD1D9p-0705,  0x0.D00AE7F5B08FB8p-0757 },   // 9.0e-0213
        { 0x1.AEE90B964B047p-0705,  0x0.58EFAC665266C8p-0757 },   // 1.0e-0212
        { 0x1.AEE90B964B047p-0704,  0x0.58EFAC665266C8p-0756 },   // 2.0e-0212
        { 0x1.432EC8B0B8435p-0703,  0x0.82B3C14CBDCD18p-0755 },   // 3.0e-0212
        { 0x1.AEE90B964B047p-0703,  0x0.58EFAC665266C8p-0755 },   // 4.0e-0212
        { 0x1.0D51A73DEEE2Cp-0702,  0x0.9795CBBFF38040p-0754 },   // 5.0e-0212
        { 0x1.432EC8B0B8435p-0702,  0x0.82B3C14CBDCD18p-0754 },   // 6.0e-0212
        { 0x1.790BEA2381A3Ep-0702,  0x0.6DD1B6D98819F0p-0754 },   // 7.0e-0212
        { 0x1.AEE90B964B047p-0702,  0x0.58EFAC665266C8p-0754 },   // 8.0e-0212
        { 0x1.E4C62D0914650p-0702,  0x0.440DA1F31CB3A0p-0754 },   // 9.0e-0212
        { 0x1.0D51A73DEEE2Cp-0701,  0x0.9795CBBFF38040p-0753 },   // 1.0e-0211
        { 0x1.0D51A73DEEE2Cp-0700,  0x0.9795CBBFF38040p-0752 },   // 2.0e-0211
        { 0x1.93FA7ADCE6542p-0700,  0x0.E360B19FED4060p-0752 },   // 3.0e-0211
        { 0x1.0D51A73DEEE2Cp-0699,  0x0.9795CBBFF38040p-0751 },   // 4.0e-0211
        { 0x1.50A6110D6A9B7p-0699,  0x0.BD7B3EAFF06050p-0751 },   // 5.0e-0211
        { 0x1.93FA7ADCE6542p-0699,  0x0.E360B19FED4060p-0751 },   // 6.0e-0211
        { 0x1.D74EE4AC620CEp-0699,  0x0.0946248FEA2070p-0751 },   // 7.0e-0211
        { 0x1.0D51A73DEEE2Cp-0698,  0x0.9795CBBFF38040p-0750 },   // 8.0e-0211
        { 0x1.2EFBDC25ACBF2p-0698,  0x0.2A888537F1F048p-0750 },   // 9.0e-0211
        { 0x1.50A6110D6A9B7p-0698,  0x0.BD7B3EAFF06050p-0750 },   // 1.0e-0210
        { 0x1.50A6110D6A9B7p-0697,  0x0.BD7B3EAFF06050p-0749 },   // 2.0e-0210
        { 0x1.F8F919941FE93p-0697,  0x0.9C38DE07E89078p-0749 },   // 3.0e-0210
        { 0x1.50A6110D6A9B7p-0696,  0x0.BD7B3EAFF06050p-0748 },   // 4.0e-0210
        { 0x1.A4CF9550C5425p-0696,  0x0.ACDA0E5BEC7860p-0748 },   // 5.0e-0210
        { 0x1.F8F919941FE93p-0696,  0x0.9C38DE07E89078p-0748 },   // 6.0e-0210
        { 0x1.26914EEBBD480p-0695,  0x0.C5CBD6D9F25440p-0747 },   // 7.0e-0210
        { 0x1.50A6110D6A9B7p-0695,  0x0.BD7B3EAFF06050p-0747 },   // 8.0e-0210
        { 0x1.7ABAD32F17EEEp-0695,  0x0.B52AA685EE6C58p-0747 },   // 9.0e-0210
        { 0x1.A4CF9550C5425p-0695,  0x0.ACDA0E5BEC7860p-0747 },   // 1.0e-0209
        { 0x1.A4CF9550C5425p-0694,  0x0.ACDA0E5BEC7860p-0746 },   // 2.0e-0209
        { 0x1.3B9BAFFC93F1Cp-0693,  0x0.41A38AC4F15A48p-0745 },   // 3.0e-0209
        { 0x1.A4CF9550C5425p-0693,  0x0.ACDA0E5BEC7860p-0745 },   // 4.0e-0209
        { 0x1.0701BD527B497p-0692,  0x0.8C0848F973CB38p-0744 },   // 5.0e-0209
        { 0x1.3B9BAFFC93F1Cp-0692,  0x0.41A38AC4F15A48p-0744 },   // 6.0e-0209
        { 0x1.7035A2A6AC9A0p-0692,  0x0.F73ECC906EE958p-0744 },   // 7.0e-0209
        { 0x1.A4CF9550C5425p-0692,  0x0.ACDA0E5BEC7860p-0744 },   // 8.0e-0209
        { 0x1.D96987FADDEAAp-0692,  0x0.627550276A0770p-0744 },   // 9.0e-0209
        { 0x1.0701BD527B497p-0691,  0x0.8C0848F973CB38p-0743 },   // 1.0e-0208
        { 0x1.0701BD527B497p-0690,  0x0.8C0848F973CB38p-0742 },   // 2.0e-0208
        { 0x1.8A829BFBB8EE3p-0690,  0x0.520C6D762DB0D8p-0742 },   // 3.0e-0208
        { 0x1.0701BD527B497p-0689,  0x0.8C0848F973CB38p-0741 },   // 4.0e-0208
        { 0x1.48C22CA71A1BDp-0689,  0x0.6F0A5B37D0BE08p-0741 },   // 5.0e-0208
        { 0x1.8A829BFBB8EE3p-0689,  0x0.520C6D762DB0D8p-0741 },   // 6.0e-0208
        { 0x1.CC430B5057C09p-0689,  0x0.350E7FB48AA3A8p-0741 },   // 7.0e-0208
        { 0x1.0701BD527B497p-0688,  0x0.8C0848F973CB38p-0740 },   // 8.0e-0208
        { 0x1.27E1F4FCCAB2Ap-0688,  0x0.7D895218A244A0p-0740 },   // 9.0e-0208
        { 0x1.48C22CA71A1BDp-0688,  0x0.6F0A5B37D0BE08p-0740 },   // 1.0e-0207
        { 0x1.48C22CA71A1BDp-0687,  0x0.6F0A5B37D0BE08p-0739 },   // 2.0e-0207
        { 0x1.ED2342FAA729Cp-0687,  0x0.268F88D3B91D10p-0739 },   // 3.0e-0207
        { 0x1.48C22CA71A1BDp-0686,  0x0.6F0A5B37D0BE08p-0738 },   // 4.0e-0207
        { 0x1.9AF2B7D0E0A2Cp-0686,  0x0.CACCF205C4ED90p-0738 },   // 5.0e-0207
        { 0x1.ED2342FAA729Cp-0686,  0x0.268F88D3B91D10p-0738 },   // 6.0e-0207
        { 0x1.1FA9E71236D85p-0685,  0x0.C1290FD0D6A648p-0737 },   // 7.0e-0207
        { 0x1.48C22CA71A1BDp-0685,  0x0.6F0A5B37D0BE08p-0737 },   // 8.0e-0207
        { 0x1.71DA723BFD5F5p-0685,  0x0.1CEBA69ECAD5D0p-0737 },   // 9.0e-0207
        { 0x1.9AF2B7D0E0A2Cp-0685,  0x0.CACCF205C4ED90p-0737 },   // 1.0e-0206
        { 0x1.9AF2B7D0E0A2Cp-0684,  0x0.CACCF205C4ED90p-0736 },   // 2.0e-0206
        { 0x1.343609DCA87A1p-0683,  0x0.9819B58453B228p-0735 },   // 3.0e-0206
        { 0x1.9AF2B7D0E0A2Cp-0683,  0x0.CACCF205C4ED90p-0735 },   // 4.0e-0206
        { 0x1.00D7B2E28C65Bp-0682,  0x0.FEC017439B1478p-0734 },   // 5.0e-0206
        { 0x1.343609DCA87A1p-0682,  0x0.9819B58453B228p-0734 },   // 6.0e-0206
        { 0x1.679460D6C48E7p-0682,  0x0.317353C50C4FE0p-0734 },   // 7.0e-0206
        { 0x1.9AF2B7D0E0A2Cp-0682,  0x0.CACCF205C4ED90p-0734 },   // 8.0e-0206
        { 0x1.CE510ECAFCB72p-0682,  0x0.642690467D8B40p-0734 },   // 9.0e-0206
        { 0x1.00D7B2E28C65Bp-0681,  0x0.FEC017439B1478p-0733 },   // 1.0e-0205
        { 0x1.00D7B2E28C65Bp-0680,  0x0.FEC017439B1478p-0732 },   // 2.0e-0205
        { 0x1.81438C53D2989p-0680,  0x0.FE2022E5689EB8p-0732 },   // 3.0e-0205
        { 0x1.00D7B2E28C65Bp-0679,  0x0.FEC017439B1478p-0731 },   // 4.0e-0205
        { 0x1.410D9F9B2F7F2p-0679,  0x0.FE701D1481D998p-0731 },   // 5.0e-0205
        { 0x1.81438C53D2989p-0679,  0x0.FE2022E5689EB8p-0731 },   // 6.0e-0205
        { 0x1.C179790C75B20p-0679,  0x0.FDD028B64F63D8p-0731 },   // 7.0e-0205
        { 0x1.00D7B2E28C65Bp-0678,  0x0.FEC017439B1478p-0730 },   // 8.0e-0205
        { 0x1.20F2A93EDDF27p-0678,  0x0.7E981A2C0E7708p-0730 },   // 9.0e-0205
        { 0x1.410D9F9B2F7F2p-0678,  0x0.FE701D1481D998p-0730 },   // 1.0e-0204
        { 0x1.410D9F9B2F7F2p-0677,  0x0.FE701D1481D998p-0729 },   // 2.0e-0204
        { 0x1.E1946F68C73ECp-0677,  0x0.7DA82B9EC2C660p-0729 },   // 3.0e-0204
        { 0x1.410D9F9B2F7F2p-0676,  0x0.FE701D1481D998p-0728 },   // 4.0e-0204
        { 0x1.91510781FB5EFp-0676,  0x0.BE0C2459A25000p-0728 },   // 5.0e-0204
        { 0x1.E1946F68C73ECp-0676,  0x0.7DA82B9EC2C660p-0728 },   // 6.0e-0204
        { 0x1.18EBEBA7C98F4p-0675,  0x0.9EA21971F19E60p-0727 },   // 7.0e-0204
        { 0x1.410D9F9B2F7F2p-0675,  0x0.FE701D1481D998p-0727 },   // 8.0e-0204
        { 0x1.692F538E956F1p-0675,  0x0.5E3E20B71214C8p-0727 },   // 9.0e-0204
        { 0x1.91510781FB5EFp-0675,  0x0.BE0C2459A25000p-0727 },   // 1.0e-0203
        { 0x1.91510781FB5EFp-0674,  0x0.BE0C2459A25000p-0726 },   // 2.0e-0203
        { 0x1.2CFCC5A17C873p-0673,  0x0.CE891B4339BC00p-0725 },   // 3.0e-0203
        { 0x1.91510781FB5EFp-0673,  0x0.BE0C2459A25000p-0725 },   // 4.0e-0203
        { 0x1.F5A549627A36Bp-0673,  0x0.AD8F2D700AE400p-0725 },   // 5.0e-0203
        { 0x1.2CFCC5A17C873p-0672,  0x0.CE891B4339BC00p-0724 },   // 6.0e-0203
        { 0x1.5F26E691BBF31p-0672,  0x0.C64A9FCE6E0600p-0724 },   // 7.0e-0203
        { 0x1.91510781FB5EFp-0672,  0x0.BE0C2459A25000p-0724 },   // 8.0e-0203
        { 0x1.C37B28723ACADp-0672,  0x0.B5CDA8E4D69A00p-0724 },   // 9.0e-0203
        { 0x1.F5A549627A36Bp-0672,  0x0.AD8F2D700AE400p-0724 },   // 1.0e-0202
        { 0x1.F5A549627A36Bp-0671,  0x0.AD8F2D700AE400p-0723 },   // 2.0e-0202
        { 0x1.783BF709DBA90p-0670,  0x0.C22B6214082B00p-0722 },   // 3.0e-0202
        { 0x1.F5A549627A36Bp-0670,  0x0.AD8F2D700AE400p-0722 },   // 4.0e-0202
        { 0x1.39874DDD8C623p-0669,  0x0.4C797C6606CE80p-0721 },   // 5.0e-0202
        { 0x1.783BF709DBA90p-0669,  0x0.C22B6214082B00p-0721 },   // 6.0e-0202
        { 0x1.B6F0A0362AEFEp-0669,  0x0.37DD47C2098780p-0721 },   // 7.0e-0202
        { 0x1.F5A549627A36Bp-0669,  0x0.AD8F2D700AE400p-0721 },   // 8.0e-0202
        { 0x1.1A2CF94764BECp-0668,  0x0.91A0898F062040p-0720 },   // 9.0e-0202
        { 0x1.39874DDD8C623p-0668,  0x0.4C797C6606CE80p-0720 },   // 1.0e-0201
        { 0x1.39874DDD8C623p-0667,  0x0.4C797C6606CE80p-0719 },   // 2.0e-0201
        { 0x1.D64AF4CC52934p-0667,  0x0.F2B63A990A35C0p-0719 },   // 3.0e-0201
        { 0x1.39874DDD8C623p-0666,  0x0.4C797C6606CE80p-0718 },   // 4.0e-0201
        { 0x1.87E92154EF7ACp-0666,  0x0.1F97DB7F888220p-0718 },   // 5.0e-0201
        { 0x1.D64AF4CC52934p-0666,  0x0.F2B63A990A35C0p-0718 },   // 6.0e-0201
        { 0x1.12566421DAD5Ep-0665,  0x0.E2EA4CD945F4B0p-0717 },   // 7.0e-0201
        { 0x1.39874DDD8C623p-0665,  0x0.4C797C6606CE80p-0717 },   // 8.0e-0201
        { 0x1.60B837993DEE7p-0665,  0x0.B608ABF2C7A850p-0717 },   // 9.0e-0201
        { 0x1.87E92154EF7ACp-0665,  0x0.1F97DB7F888220p-0717 },   // 1.0e-0200
        { 0x1.87E92154EF7ACp-0664,  0x0.1F97DB7F888220p-0716 },   // 2.0e-0200
        { 0x1.25EED8FFB39C1p-0663,  0x0.17B1E49FA66198p-0715 },   // 3.0e-0200
        { 0x1.87E92154EF7ACp-0663,  0x0.1F97DB7F888220p-0715 },   // 4.0e-0200
        { 0x1.E9E369AA2B597p-0663,  0x0.277DD25F6AA2A8p-0715 },   // 5.0e-0200
        { 0x1.25EED8FFB39C1p-0662,  0x0.17B1E49FA66198p-0714 },   // 6.0e-0200
        { 0x1.56EBFD2A518B6p-0662,  0x0.9BA4E00F9771D8p-0714 },   // 7.0e-0200
        { 0x1.87E92154EF7ACp-0662,  0x0.1F97DB7F888220p-0714 },   // 8.0e-0200
        { 0x1.B8E6457F8D6A1p-0662,  0x0.A38AD6EF799260p-0714 },   // 9.0e-0200
        { 0x1.E9E369AA2B597p-0662,  0x0.277DD25F6AA2A8p-0714 },   // 1.0e-0199
        { 0x1.E9E369AA2B597p-0661,  0x0.277DD25F6AA2A8p-0713 },   // 2.0e-0199
        { 0x1.6F6A8F3FA0831p-0660,  0x0.5D9E5DC78FF9F8p-0712 },   // 3.0e-0199
        { 0x1.E9E369AA2B597p-0660,  0x0.277DD25F6AA2A8p-0712 },   // 4.0e-0199
        { 0x1.322E220A5B17Ep-0659,  0x0.78AEA37BA2A5A8p-0711 },   // 5.0e-0199
        { 0x1.6F6A8F3FA0831p-0659,  0x0.5D9E5DC78FF9F8p-0711 },   // 6.0e-0199
        { 0x1.ACA6FC74E5EE4p-0659,  0x0.428E18137D4E50p-0711 },   // 7.0e-0199
        { 0x1.E9E369AA2B597p-0659,  0x0.277DD25F6AA2A8p-0711 },   // 8.0e-0199
        { 0x1.138FEB6FB8625p-0658,  0x0.0636C655ABFB78p-0710 },   // 9.0e-0199
        { 0x1.322E220A5B17Ep-0658,  0x0.78AEA37BA2A5A8p-0710 },   // 1.0e-0198
        { 0x1.322E220A5B17Ep-0657,  0x0.78AEA37BA2A5A8p-0709 },   // 2.0e-0198
        { 0x1.CB45330F88A3Dp-0657,  0x0.B505F53973F878p-0709 },   // 3.0e-0198
        { 0x1.322E220A5B17Ep-0656,  0x0.78AEA37BA2A5A8p-0708 },   // 4.0e-0198
        { 0x1.7EB9AA8CF1DDEp-0656,  0x0.16DA4C5A8B4F10p-0708 },   // 5.0e-0198
        { 0x1.CB45330F88A3Dp-0656,  0x0.B505F53973F878p-0708 },   // 6.0e-0198
        { 0x1.0BE85DC90FB4Ep-0655,  0x0.A998CF0C2E50F0p-0707 },   // 7.0e-0198
        { 0x1.322E220A5B17Ep-0655,  0x0.78AEA37BA2A5A8p-0707 },   // 8.0e-0198
        { 0x1.5873E64BA67AEp-0655,  0x0.47C477EB16FA58p-0707 },   // 9.0e-0198
        { 0x1.7EB9AA8CF1DDEp-0655,  0x0.16DA4C5A8B4F10p-0707 },   // 1.0e-0197
        { 0x1.7EB9AA8CF1DDEp-0654,  0x0.16DA4C5A8B4F10p-0706 },   // 2.0e-0197
        { 0x1.1F0B3FE9B5666p-0653,  0x0.9123B943E87B48p-0705 },   // 3.0e-0197
        { 0x1.7EB9AA8CF1DDEp-0653,  0x0.16DA4C5A8B4F10p-0705 },   // 4.0e-0197
        { 0x1.DE6815302E555p-0653,  0x0.9C90DF712E22D8p-0705 },   // 5.0e-0197
        { 0x1.1F0B3FE9B5666p-0652,  0x0.9123B943E87B48p-0704 },   // 6.0e-0197
        { 0x1.4EE2753B53A22p-0652,  0x0.53FF02CF39E530p-0704 },   // 7.0e-0197
        { 0x1.7EB9AA8CF1DDEp-0652,  0x0.16DA4C5A8B4F10p-0704 },   // 8.0e-0197
        { 0x1.AE90DFDE90199p-0652,  0x0.D9B595E5DCB8F0p-0704 },   // 9.0e-0197
        { 0x1.DE6815302E555p-0652,  0x0.9C90DF712E22D8p-0704 },   // 1.0e-0196
        { 0x1.DE6815302E555p-0651,  0x0.9C90DF712E22D8p-0703 },   // 2.0e-0196
        { 0x1.66CE0FE422C00p-0650,  0x0.356CA794E29A20p-0702 },   // 3.0e-0196
        { 0x1.DE6815302E555p-0650,  0x0.9C90DF712E22D8p-0702 },   // 4.0e-0196
        { 0x1.2B010D3E1CF55p-0649,  0x0.81DA8BA6BCD5C0p-0701 },   // 5.0e-0196
        { 0x1.66CE0FE422C00p-0649,  0x0.356CA794E29A20p-0701 },   // 6.0e-0196
        { 0x1.A29B128A288AAp-0649,  0x0.E8FEC383085E78p-0701 },   // 7.0e-0196
        { 0x1.DE6815302E555p-0649,  0x0.9C90DF712E22D8p-0701 },   // 8.0e-0196
        { 0x1.0D1A8BEB1A100p-0648,  0x0.28117DAFA9F398p-0700 },   // 9.0e-0196
        { 0x1.2B010D3E1CF55p-0648,  0x0.81DA8BA6BCD5C0p-0700 },   // 1.0e-0195
        { 0x1.2B010D3E1CF55p-0647,  0x0.81DA8BA6BCD5C0p-0699 },   // 2.0e-0195
        { 0x1.C08193DD2B700p-0647,  0x0.42C7D17A1B40A8p-0699 },   // 3.0e-0195
        { 0x1.2B010D3E1CF55p-0646,  0x0.81DA8BA6BCD5C0p-0698 },   // 4.0e-0195
        { 0x1.75C1508DA432Ap-0646,  0x0.E2512E906C0B38p-0698 },   // 5.0e-0195
        { 0x1.C08193DD2B700p-0646,  0x0.42C7D17A1B40A8p-0698 },   // 6.0e-0195
        { 0x1.05A0EB965956Ap-0645,  0x0.D19F3A31E53B08p-0697 },   // 7.0e-0195
        { 0x1.2B010D3E1CF55p-0645,  0x0.81DA8BA6BCD5C0p-0697 },   // 8.0e-0195
        { 0x1.50612EE5E0940p-0645,  0x0.3215DD1B947080p-0697 },   // 9.0e-0195
        { 0x1.75C1508DA432Ap-0645,  0x0.E2512E906C0B38p-0697 },   // 1.0e-0194
        { 0x1.75C1508DA432Ap-0644,  0x0.E2512E906C0B38p-0696 },   // 2.0e-0194
        { 0x1.1850FC6A3B260p-0643,  0x0.29BCE2EC510868p-0695 },   // 3.0e-0194
        { 0x1.75C1508DA432Ap-0643,  0x0.E2512E906C0B38p-0695 },   // 4.0e-0194
        { 0x1.D331A4B10D3F5p-0643,  0x0.9AE57A34870E08p-0695 },   // 5.0e-0194
        { 0x1.1850FC6A3B260p-0642,  0x0.29BCE2EC510868p-0694 },   // 6.0e-0194
        { 0x1.4709267BEFAC5p-0642,  0x0.860708BE5E89D0p-0694 },   // 7.0e-0194
        { 0x1.75C1508DA432Ap-0642,  0x0.E2512E906C0B38p-0694 },   // 8.0e-0194
        { 0x1.A4797A9F58B90p-0642,  0x0.3E9B5462798CA0p-0694 },   // 9.0e-0194
        { 0x1.D331A4B10D3F5p-0642,  0x0.9AE57A34870E08p-0694 },   // 1.0e-0193
        { 0x1.D331A4B10D3F5p-0641,  0x0.9AE57A34870E08p-0693 },   // 2.0e-0193
        { 0x1.5E653B84C9EF8p-0640,  0x0.342C1BA7654A80p-0692 },   // 3.0e-0193
        { 0x1.D331A4B10D3F5p-0640,  0x0.9AE57A34870E08p-0692 },   // 4.0e-0193
        { 0x1.23FF06EEA8479p-0639,  0x0.80CF6C60D468C0p-0691 },   // 5.0e-0193
        { 0x1.5E653B84C9EF8p-0639,  0x0.342C1BA7654A80p-0691 },   // 6.0e-0193
        { 0x1.98CB701AEB976p-0639,  0x0.E788CAEDF62C40p-0691 },   // 7.0e-0193
        { 0x1.D331A4B10D3F5p-0639,  0x0.9AE57A34870E08p-0691 },   // 8.0e-0193
        { 0x1.06CBECA39773Ap-0638,  0x0.272114BD8BF7E0p-0690 },   // 9.0e-0193
        { 0x1.23FF06EEA8479p-0638,  0x0.80CF6C60D468C0p-0690 },   // 1.0e-0192
        { 0x1.23FF06EEA8479p-0637,  0x0.80CF6C60D468C0p-0689 },   // 2.0e-0192
        { 0x1.B5FE8A65FC6B6p-0637,  0x0.413722913E9D20p-0689 },   // 3.0e-0192
        { 0x1.23FF06EEA8479p-0636,  0x0.80CF6C60D468C0p-0688 },   // 4.0e-0192
        { 0x1.6CFEC8AA52597p-0636,  0x0.E10347790982F0p-0688 },   // 5.0e-0192
        { 0x1.B5FE8A65FC6B6p-0636,  0x0.413722913E9D20p-0688 },   // 6.0e-0192
        { 0x1.FEFE4C21A67D4p-0636,  0x0.A16AFDA973B758p-0688 },   // 7.0e-0192
        { 0x1.23FF06EEA8479p-0635,  0x0.80CF6C60D468C0p-0687 },   // 8.0e-0192
        { 0x1.487EE7CC7D508p-0635,  0x0.B0E959ECEEF5D8p-0687 },   // 9.0e-0192
        { 0x1.6CFEC8AA52597p-0635,  0x0.E10347790982F0p-0687 },   // 1.0e-0191
        { 0x1.6CFEC8AA52597p-0634,  0x0.E10347790982F0p-0686 },   // 2.0e-0191
        { 0x1.11BF167FBDC31p-0633,  0x0.E8C2759AC72238p-0685 },   // 3.0e-0191
        { 0x1.6CFEC8AA52597p-0633,  0x0.E10347790982F0p-0685 },   // 4.0e-0191
        { 0x1.C83E7AD4E6EFDp-0633,  0x0.D94419574BE3B0p-0685 },   // 5.0e-0191
        { 0x1.11BF167FBDC31p-0632,  0x0.E8C2759AC72238p-0684 },   // 6.0e-0191
        { 0x1.3F5EEF95080E4p-0632,  0x0.E4E2DE89E85290p-0684 },   // 7.0e-0191
        { 0x1.6CFEC8AA52597p-0632,  0x0.E10347790982F0p-0684 },   // 8.0e-0191
        { 0x1.9A9EA1BF9CA4Ap-0632,  0x0.DD23B0682AB350p-0684 },   // 9.0e-0191
        { 0x1.C83E7AD4E6EFDp-0632,  0x0.D94419574BE3B0p-0684 },   // 1.0e-0190
        { 0x1.C83E7AD4E6EFDp-0631,  0x0.D94419574BE3B0p-0683 },   // 2.0e-0190
        { 0x1.562EDC1FAD33Ep-0630,  0x0.62F3130178EAC0p-0682 },   // 3.0e-0190
        { 0x1.C83E7AD4E6EFDp-0630,  0x0.D94419574BE3B0p-0682 },   // 4.0e-0190
        { 0x1.1D270CC51055Ep-0629,  0x0.A7CA8FD68F6E50p-0681 },   // 5.0e-0190
        { 0x1.562EDC1FAD33Ep-0629,  0x0.62F3130178EAC0p-0681 },   // 6.0e-0190
        { 0x1.8F36AB7A4A11Ep-0629,  0x0.1E1B962C626738p-0681 },   // 7.0e-0190
        { 0x1.C83E7AD4E6EFDp-0629,  0x0.D94419574BE3B0p-0681 },   // 8.0e-0190
        { 0x1.00A32517C1E6Ep-0628,  0x0.CA364E411AB010p-0680 },   // 9.0e-0190
        { 0x1.1D270CC51055Ep-0628,  0x0.A7CA8FD68F6E50p-0680 },   // 1.0e-0189
        { 0x1.1D270CC51055Ep-0627,  0x0.A7CA8FD68F6E50p-0679 },   // 2.0e-0189
        { 0x1.ABBA93279880Dp-0627,  0x0.FBAFD7C1D72578p-0679 },   // 3.0e-0189
        { 0x1.1D270CC51055Ep-0626,  0x0.A7CA8FD68F6E50p-0678 },   // 4.0e-0189
        { 0x1.6470CFF6546B6p-0626,  0x0.51BD33CC3349E0p-0678 },   // 5.0e-0189
        { 0x1.ABBA93279880Dp-0626,  0x0.FBAFD7C1D72578p-0678 },   // 6.0e-0189
        { 0x1.F3045658DC965p-0626,  0x0.A5A27BB77B0108p-0678 },   // 7.0e-0189
        { 0x1.1D270CC51055Ep-0625,  0x0.A7CA8FD68F6E50p-0677 },   // 8.0e-0189
        { 0x1.40CBEE5DB260Ap-0625,  0x0.7CC3E1D1615C18p-0677 },   // 9.0e-0189
        { 0x1.6470CFF6546B6p-0625,  0x0.51BD33CC3349E0p-0677 },   // 1.0e-0188
        { 0x1.6470CFF6546B6p-0624,  0x0.51BD33CC3349E0p-0676 },   // 2.0e-0188
        { 0x1.0B549BF8BF508p-0623,  0x0.BD4DE6D9267768p-0675 },   // 3.0e-0188
        { 0x1.6470CFF6546B6p-0623,  0x0.51BD33CC3349E0p-0675 },   // 4.0e-0188
        { 0x1.BD8D03F3E9863p-0623,  0x0.E62C80BF401C58p-0675 },   // 5.0e-0188
        { 0x1.0B549BF8BF508p-0622,  0x0.BD4DE6D9267768p-0674 },   // 6.0e-0188
        { 0x1.37E2B5F789DDFp-0622,  0x0.87858D52ACE0A0p-0674 },   // 7.0e-0188
        { 0x1.6470CFF6546B6p-0622,  0x0.51BD33CC3349E0p-0674 },   // 8.0e-0188
        { 0x1.90FEE9F51EF8Dp-0622,  0x0.1BF4DA45B9B320p-0674 },   // 9.0e-0188
        { 0x1.BD8D03F3E9863p-0622,  0x0.E62C80BF401C58p-0674 },   // 1.0e-0187
        { 0x1.BD8D03F3E9863p-0621,  0x0.E62C80BF401C58p-0673 },   // 2.0e-0187
        { 0x1.4E29C2F6EF24Ap-0620,  0x0.ECA1608F701540p-0672 },   // 3.0e-0187
        { 0x1.BD8D03F3E9863p-0620,  0x0.E62C80BF401C58p-0672 },   // 4.0e-0187
        { 0x1.1678227871F3Ep-0619,  0x0.6FDBD0778811B8p-0671 },   // 5.0e-0187
        { 0x1.4E29C2F6EF24Ap-0619,  0x0.ECA1608F701540p-0671 },   // 6.0e-0187
        { 0x1.85DB63756C557p-0619,  0x0.6966F0A75818D0p-0671 },   // 7.0e-0187
        { 0x1.BD8D03F3E9863p-0619,  0x0.E62C80BF401C58p-0671 },   // 8.0e-0187
        { 0x1.F53EA47266B70p-0619,  0x0.62F210D7281FE8p-0671 },   // 9.0e-0187
        { 0x1.1678227871F3Ep-0618,  0x0.6FDBD0778811B8p-0670 },   // 1.0e-0186
        { 0x1.1678227871F3Ep-0617,  0x0.6FDBD0778811B8p-0669 },   // 2.0e-0186
        { 0x1.A1B433B4AAEDDp-0617,  0x0.A7C9B8B34C1A90p-0669 },   // 3.0e-0186
        { 0x1.1678227871F3Ep-0616,  0x0.6FDBD0778811B8p-0668 },   // 4.0e-0186
        { 0x1.5C162B168E70Ep-0616,  0x0.0BD2C4956A1628p-0668 },   // 5.0e-0186
        { 0x1.A1B433B4AAEDDp-0616,  0x0.A7C9B8B34C1A90p-0668 },   // 6.0e-0186
        { 0x1.E7523C52C76ADp-0616,  0x0.43C0ACD12E1F00p-0668 },   // 7.0e-0186
        { 0x1.1678227871F3Ep-0615,  0x0.6FDBD0778811B8p-0667 },   // 8.0e-0186
        { 0x1.394726C780326p-0615,  0x0.3DD74A867913F0p-0667 },   // 9.0e-0186
        { 0x1.5C162B168E70Ep-0615,  0x0.0BD2C4956A1628p-0667 },   // 1.0e-0185
        { 0x1.5C162B168E70Ep-0614,  0x0.0BD2C4956A1628p-0666 },   // 2.0e-0185
        { 0x1.0510A050EAD4Ap-0613,  0x0.88DE13700F9098p-0665 },   // 3.0e-0185
        { 0x1.5C162B168E70Ep-0613,  0x0.0BD2C4956A1628p-0665 },   // 4.0e-0185
        { 0x1.B31BB5DC320D1p-0613,  0x0.8EC775BAC49BB0p-0665 },   // 5.0e-0185
        { 0x1.0510A050EAD4Ap-0612,  0x0.88DE13700F9098p-0664 },   // 6.0e-0185
        { 0x1.309365B3BCA2Cp-0612,  0x0.4A586C02BCD360p-0664 },   // 7.0e-0185
        { 0x1.5C162B168E70Ep-0612,  0x0.0BD2C4956A1628p-0664 },   // 8.0e-0185
        { 0x1.8798F079603EFp-0612,  0x0.CD4D1D281758E8p-0664 },   // 9.0e-0185
        { 0x1.B31BB5DC320D1p-0612,  0x0.8EC775BAC49BB0p-0664 },   // 1.0e-0184
        { 0x1.B31BB5DC320D1p-0611,  0x0.8EC775BAC49BB0p-0663 },   // 2.0e-0184
        { 0x1.4654C8652589Dp-0610,  0x0.2B15984C1374C0p-0662 },   // 3.0e-0184
        { 0x1.B31BB5DC320D1p-0610,  0x0.8EC775BAC49BB0p-0662 },   // 4.0e-0184
        { 0x1.0FF151A99F482p-0609,  0x0.F93CA994BAE150p-0661 },   // 5.0e-0184
        { 0x1.4654C8652589Dp-0609,  0x0.2B15984C1374C0p-0661 },   // 6.0e-0184
        { 0x1.7CB83F20ABCB7p-0609,  0x0.5CEE87036C0838p-0661 },   // 7.0e-0184
        { 0x1.B31BB5DC320D1p-0609,  0x0.8EC775BAC49BB0p-0661 },   // 8.0e-0184
        { 0x1.E97F2C97B84EBp-0609,  0x0.C0A064721D2F28p-0661 },   // 9.0e-0184
        { 0x1.0FF151A99F482p-0608,  0x0.F93CA994BAE150p-0660 },   // 1.0e-0183
        { 0x1.0FF151A99F482p-0607,  0x0.F93CA994BAE150p-0659 },   // 2.0e-0183
        { 0x1.97E9FA7E6EEC4p-0607,  0x0.75DAFE5F1851F8p-0659 },   // 3.0e-0183
        { 0x1.0FF151A99F482p-0606,  0x0.F93CA994BAE150p-0658 },   // 4.0e-0183
        { 0x1.53EDA614071A3p-0606,  0x0.B78BD3F9E999A0p-0658 },   // 5.0e-0183
        { 0x1.97E9FA7E6EEC4p-0606,  0x0.75DAFE5F1851F8p-0658 },   // 6.0e-0183
        { 0x1.DBE64EE8D6BE5p-0606,  0x0.342A28C4470A48p-0658 },   // 7.0e-0183
        { 0x1.0FF151A99F482p-0605,  0x0.F93CA994BAE150p-0657 },   // 8.0e-0183
        { 0x1.31EF7BDED3313p-0605,  0x0.58643EC7523D78p-0657 },   // 9.0e-0183
        { 0x1.53EDA614071A3p-0605,  0x0.B78BD3F9E999A0p-0657 },   // 1.0e-0182
        { 0x1.53EDA614071A3p-0604,  0x0.B78BD3F9E999A0p-0656 },   // 2.0e-0182
        { 0x1.FDE4791E0AA75p-0604,  0x0.9351BDF6DE6670p-0656 },   // 3.0e-0182
        { 0x1.53EDA614071A3p-0603,  0x0.B78BD3F9E999A0p-0655 },   // 4.0e-0182
        { 0x1.A8E90F9908E0Cp-0603,  0x0.A56EC8F8640008p-0655 },   // 5.0e-0182
        { 0x1.FDE4791E0AA75p-0603,  0x0.9351BDF6DE6670p-0655 },   // 6.0e-0182
        { 0x1.296FF1518636Fp-0602,  0x0.409A597AAC6668p-0654 },   // 7.0e-0182
        { 0x1.53EDA614071A3p-0602,  0x0.B78BD3F9E999A0p-0654 },   // 8.0e-0182
        { 0x1.7E6B5AD687FD8p-0602,  0x0.2E7D4E7926CCD8p-0654 },   // 9.0e-0182
        { 0x1.A8E90F9908E0Cp-0602,  0x0.A56EC8F8640008p-0654 },   // 1.0e-0181
        { 0x1.A8E90F9908E0Cp-0601,  0x0.A56EC8F8640008p-0653 },   // 2.0e-0181
        { 0x1.3EAECBB2C6A89p-0600,  0x0.7C1316BA4B0008p-0652 },   // 3.0e-0181
        { 0x1.A8E90F9908E0Cp-0600,  0x0.A56EC8F8640008p-0652 },   // 4.0e-0181
        { 0x1.0991A9BFA58C7p-0599,  0x0.E7653D9B3E8008p-0651 },   // 5.0e-0181
        { 0x1.3EAECBB2C6A89p-0599,  0x0.7C1316BA4B0008p-0651 },   // 6.0e-0181
        { 0x1.73CBEDA5E7C4Bp-0599,  0x0.10C0EFD9578008p-0651 },   // 7.0e-0181
        { 0x1.A8E90F9908E0Cp-0599,  0x0.A56EC8F8640008p-0651 },   // 8.0e-0181
        { 0x1.DE06318C29FCEp-0599,  0x0.3A1CA217708008p-0651 },   // 9.0e-0181
        { 0x1.0991A9BFA58C7p-0598,  0x0.E7653D9B3E8008p-0650 },   // 1.0e-0180
        { 0x1.0991A9BFA58C7p-0597,  0x0.E7653D9B3E8008p-0649 },   // 2.0e-0180
        { 0x1.8E5A7E9F7852Bp-0597,  0x0.DB17DC68DDC008p-0649 },   // 3.0e-0180
        { 0x1.0991A9BFA58C7p-0596,  0x0.E7653D9B3E8008p-0648 },   // 4.0e-0180
        { 0x1.4BF6142F8EEF9p-0596,  0x0.E13E8D020E2008p-0648 },   // 5.0e-0180
        { 0x1.8E5A7E9F7852Bp-0596,  0x0.DB17DC68DDC008p-0648 },   // 6.0e-0180
        { 0x1.D0BEE90F61B5Dp-0596,  0x0.D4F12BCFAD6008p-0648 },   // 7.0e-0180
        { 0x1.0991A9BFA58C7p-0595,  0x0.E7653D9B3E8008p-0647 },   // 8.0e-0180
        { 0x1.2AC3DEF79A3E0p-0595,  0x0.E451E54EA65008p-0647 },   // 9.0e-0180
        { 0x1.4BF6142F8EEF9p-0595,  0x0.E13E8D020E2008p-0647 },   // 1.0e-0179
        { 0x1.4BF6142F8EEF9p-0594,  0x0.E13E8D020E2008p-0646 },   // 2.0e-0179
        { 0x1.F1F11E4756676p-0594,  0x0.D1DDD383153008p-0646 },   // 3.0e-0179
        { 0x1.4BF6142F8EEF9p-0593,  0x0.E13E8D020E2008p-0645 },   // 4.0e-0179
        { 0x1.9EF3993B72AB8p-0593,  0x0.598E304291A808p-0645 },   // 5.0e-0179
        { 0x1.F1F11E4756676p-0593,  0x0.D1DDD383153008p-0645 },   // 6.0e-0179
        { 0x1.227751A99D11Ap-0592,  0x0.A516BB61CC5C08p-0644 },   // 7.0e-0179
        { 0x1.4BF6142F8EEF9p-0592,  0x0.E13E8D020E2008p-0644 },   // 8.0e-0179
        { 0x1.7574D6B580CD9p-0592,  0x0.1D665EA24FE408p-0644 },   // 9.0e-0179
        { 0x1.9EF3993B72AB8p-0592,  0x0.598E304291A808p-0644 },   // 1.0e-0178
        { 0x1.9EF3993B72AB8p-0591,  0x0.598E304291A808p-0643 },   // 2.0e-0178
        { 0x1.3736B2EC9600Ap-0590,  0x0.432AA431ED3E08p-0642 },   // 3.0e-0178
        { 0x1.9EF3993B72AB8p-0590,  0x0.598E304291A808p-0642 },   // 4.0e-0178
        { 0x1.03583FC527AB3p-0589,  0x0.37F8DE299B0908p-0641 },   // 5.0e-0178
        { 0x1.3736B2EC9600Ap-0589,  0x0.432AA431ED3E08p-0641 },   // 6.0e-0178
        { 0x1.6B15261404561p-0589,  0x0.4E5C6A3A3F7308p-0641 },   // 7.0e-0178
        { 0x1.9EF3993B72AB8p-0589,  0x0.598E304291A808p-0641 },   // 8.0e-0178
        { 0x1.D2D20C62E100Fp-0589,  0x0.64BFF64AE3DD08p-0641 },   // 9.0e-0178
        { 0x1.03583FC527AB3p-0588,  0x0.37F8DE299B0908p-0640 },   // 1.0e-0177
        { 0x1.03583FC527AB3p-0587,  0x0.37F8DE299B0908p-0639 },   // 2.0e-0177
        { 0x1.85045FA7BB80Cp-0587,  0x0.D3F54D3E688D88p-0639 },   // 3.0e-0177
        { 0x1.03583FC527AB3p-0586,  0x0.37F8DE299B0908p-0638 },   // 4.0e-0177
        { 0x1.442E4FB671960p-0586,  0x0.05F715B401CB48p-0638 },   // 5.0e-0177
        { 0x1.85045FA7BB80Cp-0586,  0x0.D3F54D3E688D88p-0638 },   // 6.0e-0177
        { 0x1.C5DA6F99056B9p-0586,  0x0.A1F384C8CF4FC8p-0638 },   // 7.0e-0177
        { 0x1.03583FC527AB3p-0585,  0x0.37F8DE299B0908p-0637 },   // 8.0e-0177
        { 0x1.23C347BDCCA09p-0585,  0x0.9EF7F9EECE6A28p-0637 },   // 9.0e-0177
        { 0x1.442E4FB671960p-0585,  0x0.05F715B401CB48p-0637 },   // 1.0e-0176
        { 0x1.442E4FB671960p-0584,  0x0.05F715B401CB48p-0636 },   // 2.0e-0176
        { 0x1.E6457791AA610p-0584,  0x0.08F2A08E02B0E8p-0636 },   // 3.0e-0176
        { 0x1.442E4FB671960p-0583,  0x0.05F715B401CB48p-0635 },   // 4.0e-0176
        { 0x1.9539E3A40DFB8p-0583,  0x0.0774DB21023E18p-0635 },   // 5.0e-0176
        { 0x1.E6457791AA610p-0583,  0x0.08F2A08E02B0E8p-0635 },   // 6.0e-0176
        { 0x1.1BA885BFA3634p-0582,  0x0.053832FD8191E0p-0634 },   // 7.0e-0176
        { 0x1.442E4FB671960p-0582,  0x0.05F715B401CB48p-0634 },   // 8.0e-0176
        { 0x1.6CB419AD3FC8Cp-0582,  0x0.06B5F86A8204B0p-0634 },   // 9.0e-0176
        { 0x1.9539E3A40DFB8p-0582,  0x0.0774DB21023E18p-0634 },   // 1.0e-0175
        { 0x1.9539E3A40DFB8p-0581,  0x0.0774DB21023E18p-0633 },   // 2.0e-0175
        { 0x1.2FEB6ABB0A7CAp-0580,  0x0.0597A458C1AE90p-0632 },   // 3.0e-0175
        { 0x1.9539E3A40DFB8p-0580,  0x0.0774DB21023E18p-0632 },   // 4.0e-0175
        { 0x1.FA885C8D117A6p-0580,  0x0.095211E942CDA0p-0632 },   // 5.0e-0175
        { 0x1.2FEB6ABB0A7CAp-0579,  0x0.0597A458C1AE90p-0631 },   // 6.0e-0175
        { 0x1.6292A72F8C3C1p-0579,  0x0.06863FBCE1F658p-0631 },   // 7.0e-0175
        { 0x1.9539E3A40DFB8p-0579,  0x0.0774DB21023E18p-0631 },   // 8.0e-0175
        { 0x1.C7E120188FBAFp-0579,  0x0.086376852285E0p-0631 },   // 9.0e-0175
        { 0x1.FA885C8D117A6p-0579,  0x0.095211E942CDA0p-0631 },   // 1.0e-0174
        { 0x1.FA885C8D117A6p-0578,  0x0.095211E942CDA0p-0630 },   // 2.0e-0174
        { 0x1.7BE64569CD1BCp-0577,  0x0.86FD8D6EF21A38p-0629 },   // 3.0e-0174
        { 0x1.FA885C8D117A6p-0577,  0x0.095211E942CDA0p-0629 },   // 4.0e-0174
        { 0x1.3C9539D82AEC7p-0576,  0x0.C5D34B31C9C080p-0628 },   // 5.0e-0174
        { 0x1.7BE64569CD1BCp-0576,  0x0.86FD8D6EF21A38p-0628 },   // 6.0e-0174
        { 0x1.BB3750FB6F4B1p-0576,  0x0.4827CFAC1A73E8p-0628 },   // 7.0e-0174
        { 0x1.FA885C8D117A6p-0576,  0x0.095211E942CDA0p-0628 },   // 8.0e-0174
        { 0x1.1CECB40F59D4Dp-0575,  0x0.653E2A133593A8p-0627 },   // 9.0e-0174
        { 0x1.3C9539D82AEC7p-0575,  0x0.C5D34B31C9C080p-0627 },   // 1.0e-0173
        { 0x1.3C9539D82AEC7p-0574,  0x0.C5D34B31C9C080p-0626 },   // 2.0e-0173
        { 0x1.DADFD6C44062Bp-0574,  0x0.A8BCF0CAAEA0C8p-0626 },   // 3.0e-0173
        { 0x1.3C9539D82AEC7p-0573,  0x0.C5D34B31C9C080p-0625 },   // 4.0e-0173
        { 0x1.8BBA884E35A79p-0573,  0x0.B7481DFE3C30A0p-0625 },   // 5.0e-0173
        { 0x1.DADFD6C44062Bp-0573,  0x0.A8BCF0CAAEA0C8p-0625 },   // 6.0e-0173
        { 0x1.1502929D258EEp-0572,  0x0.CD18E1CB908870p-0624 },   // 7.0e-0173
        { 0x1.3C9539D82AEC7p-0572,  0x0.C5D34B31C9C080p-0624 },   // 8.0e-0173
        { 0x1.6427E113304A0p-0572,  0x0.BE8DB49802F890p-0624 },   // 9.0e-0173
        { 0x1.8BBA884E35A79p-0572,  0x0.B7481DFE3C30A0p-0624 },   // 1.0e-0172
        { 0x1.8BBA884E35A79p-0571,  0x0.B7481DFE3C30A0p-0623 },   // 2.0e-0172
        { 0x1.28CBE63AA83DBp-0570,  0x0.4976167EAD2478p-0622 },   // 3.0e-0172
        { 0x1.8BBA884E35A79p-0570,  0x0.B7481DFE3C30A0p-0622 },   // 4.0e-0172
        { 0x1.EEA92A61C3118p-0570,  0x0.251A257DCB3CD0p-0622 },   // 5.0e-0172
        { 0x1.28CBE63AA83DBp-0569,  0x0.4976167EAD2478p-0621 },   // 6.0e-0172
        { 0x1.5A4337446EF2Ap-0569,  0x0.805F1A3E74AA90p-0621 },   // 7.0e-0172
        { 0x1.8BBA884E35A79p-0569,  0x0.B7481DFE3C30A0p-0621 },   // 8.0e-0172
        { 0x1.BD31D957FC5C8p-0569,  0x0.EE3121BE03B6B8p-0621 },   // 9.0e-0172
        { 0x1.EEA92A61C3118p-0569,  0x0.251A257DCB3CD0p-0621 },   // 1.0e-0171
        { 0x1.EEA92A61C3118p-0568,  0x0.251A257DCB3CD0p-0620 },   // 2.0e-0171
        { 0x1.72FEDFC9524D2p-0567,  0x0.1BD39C1E586D98p-0619 },   // 3.0e-0171
        { 0x1.EEA92A61C3118p-0567,  0x0.251A257DCB3CD0p-0619 },   // 4.0e-0171
        { 0x1.3529BA7D19EAFp-0566,  0x0.1730576E9F0600p-0618 },   // 5.0e-0171
        { 0x1.72FEDFC9524D2p-0566,  0x0.1BD39C1E586D98p-0618 },   // 6.0e-0171
        { 0x1.B0D405158AAF5p-0566,  0x0.2076E0CE11D530p-0618 },   // 7.0e-0171
        { 0x1.EEA92A61C3118p-0566,  0x0.251A257DCB3CD0p-0618 },   // 8.0e-0171
        { 0x1.163F27D6FDB9Dp-0565,  0x0.94DEB516C25230p-0617 },   // 9.0e-0171
        { 0x1.3529BA7D19EAFp-0565,  0x0.1730576E9F0600p-0617 },   // 1.0e-0170
        { 0x1.3529BA7D19EAFp-0564,  0x0.1730576E9F0600p-0616 },   // 2.0e-0170
        { 0x1.CFBE97BBA6E06p-0564,  0x0.A2C88325EE8900p-0616 },   // 3.0e-0170
        { 0x1.3529BA7D19EAFp-0563,  0x0.1730576E9F0600p-0615 },   // 4.0e-0170
        { 0x1.8274291C6065Ap-0563,  0x0.DCFC6D4A46C780p-0615 },   // 5.0e-0170
        { 0x1.CFBE97BBA6E06p-0563,  0x0.A2C88325EE8900p-0615 },   // 6.0e-0170
        { 0x1.0E84832D76AD9p-0562,  0x0.344A4C80CB2540p-0614 },   // 7.0e-0170
        { 0x1.3529BA7D19EAFp-0562,  0x0.1730576E9F0600p-0614 },   // 8.0e-0170
        { 0x1.5BCEF1CCBD284p-0562,  0x0.FA16625C72E6C0p-0614 },   // 9.0e-0170
        { 0x1.8274291C6065Ap-0562,  0x0.DCFC6D4A46C780p-0614 },   // 1.0e-0169
        { 0x1.8274291C6065Ap-0561,  0x0.DCFC6D4A46C780p-0613 },   // 2.0e-0169
        { 0x1.21D71ED5484C4p-0560,  0x0.25BD51F7B515A0p-0612 },   // 3.0e-0169
        { 0x1.8274291C6065Ap-0560,  0x0.DCFC6D4A46C780p-0612 },   // 4.0e-0169
        { 0x1.E3113363787F1p-0560,  0x0.943B889CD87960p-0612 },   // 5.0e-0169
        { 0x1.21D71ED5484C4p-0559,  0x0.25BD51F7B515A0p-0611 },   // 6.0e-0169
        { 0x1.5225A3F8D458Fp-0559,  0x0.815CDFA0FDEE90p-0611 },   // 7.0e-0169
        { 0x1.8274291C6065Ap-0559,  0x0.DCFC6D4A46C780p-0611 },   // 8.0e-0169
        { 0x1.B2C2AE3FEC726p-0559,  0x0.389BFAF38FA070p-0611 },   // 9.0e-0169
        { 0x1.E3113363787F1p-0559,  0x0.943B889CD87960p-0611 },   // 1.0e-0168
        { 0x1.E3113363787F1p-0558,  0x0.943B889CD87960p-0610 },   // 2.0e-0168
        { 0x1.6A4CE68A9A5F5p-0557,  0x0.2F2CA675A25B08p-0609 },   // 3.0e-0168
        { 0x1.E3113363787F1p-0557,  0x0.943B889CD87960p-0609 },   // 4.0e-0168
        { 0x1.2DEAC01E2B4F6p-0556,  0x0.FCA53562074BD8p-0608 },   // 5.0e-0168
        { 0x1.6A4CE68A9A5F5p-0556,  0x0.2F2CA675A25B08p-0608 },   // 6.0e-0168
        { 0x1.A6AF0CF7096F3p-0556,  0x0.61B417893D6A38p-0608 },   // 7.0e-0168
        { 0x1.E3113363787F1p-0556,  0x0.943B889CD87960p-0608 },   // 8.0e-0168
        { 0x1.0FB9ACE7F3C77p-0555,  0x0.E3617CD839C448p-0607 },   // 9.0e-0168
        { 0x1.2DEAC01E2B4F6p-0555,  0x0.FCA53562074BD8p-0607 },   // 1.0e-0167
        { 0x1.2DEAC01E2B4F6p-0554,  0x0.FCA53562074BD8p-0606 },   // 2.0e-0167
        { 0x1.C4E0202D40F72p-0554,  0x0.7AF7D0130AF1C8p-0606 },   // 3.0e-0167
        { 0x1.2DEAC01E2B4F6p-0553,  0x0.FCA53562074BD8p-0605 },   // 4.0e-0167
        { 0x1.79657025B6234p-0553,  0x0.BBCE82BA891ED0p-0605 },   // 5.0e-0167
        { 0x1.C4E0202D40F72p-0553,  0x0.7AF7D0130AF1C8p-0605 },   // 6.0e-0167
        { 0x1.082D681A65E58p-0552,  0x0.1D108EB5C66260p-0604 },   // 7.0e-0167
        { 0x1.2DEAC01E2B4F6p-0552,  0x0.FCA53562074BD8p-0604 },   // 8.0e-0167
        { 0x1.53A81821F0B95p-0552,  0x0.DC39DC0E483558p-0604 },   // 9.0e-0167
        { 0x1.79657025B6234p-0552,  0x0.BBCE82BA891ED0p-0604 },   // 1.0e-0166
        { 0x1.79657025B6234p-0551,  0x0.BBCE82BA891ED0p-0603 },   // 2.0e-0166
        { 0x1.1B0C141C489A7p-0550,  0x0.8CDAE20BE6D720p-0602 },   // 3.0e-0166
        { 0x1.79657025B6234p-0550,  0x0.BBCE82BA891ED0p-0602 },   // 4.0e-0166
        { 0x1.D7BECC2F23AC1p-0550,  0x0.EAC223692B6688p-0602 },   // 5.0e-0166
        { 0x1.1B0C141C489A7p-0549,  0x0.8CDAE20BE6D720p-0601 },   // 6.0e-0166
        { 0x1.4A38C220FF5EEp-0549,  0x0.2454B26337FAF8p-0601 },   // 7.0e-0166
        { 0x1.79657025B6234p-0549,  0x0.BBCE82BA891ED0p-0601 },   // 8.0e-0166
        { 0x1.A8921E2A6CE7Bp-0549,  0x0.53485311DA42B0p-0601 },   // 9.0e-0166
        { 0x1.D7BECC2F23AC1p-0549,  0x0.EAC223692B6688p-0601 },   // 1.0e-0165
        { 0x1.D7BECC2F23AC1p-0548,  0x0.EAC223692B6688p-0600 },   // 2.0e-0165
        { 0x1.61CF19235AC11p-0547,  0x0.70119A8EE08CE8p-0599 },   // 3.0e-0165
        { 0x1.D7BECC2F23AC1p-0547,  0x0.EAC223692B6688p-0599 },   // 4.0e-0165
        { 0x1.26D73F9D764B9p-0546,  0x0.32B95621BB2010p-0598 },   // 5.0e-0165
        { 0x1.61CF19235AC11p-0546,  0x0.70119A8EE08CE8p-0598 },   // 6.0e-0165
        { 0x1.9CC6F2A93F369p-0546,  0x0.AD69DEFC05F9B8p-0598 },   // 7.0e-0165
        { 0x1.D7BECC2F23AC1p-0546,  0x0.EAC223692B6688p-0598 },   // 8.0e-0165
        { 0x1.095B52DA8410Dp-0545,  0x0.140D33EB2869A8p-0597 },   // 9.0e-0165
        { 0x1.26D73F9D764B9p-0545,  0x0.32B95621BB2010p-0597 },   // 1.0e-0164
        { 0x1.26D73F9D764B9p-0544,  0x0.32B95621BB2010p-0596 },   // 2.0e-0164
        { 0x1.BA42DF6C31715p-0544,  0x0.CC16013298B020p-0596 },   // 3.0e-0164
        { 0x1.26D73F9D764B9p-0543,  0x0.32B95621BB2010p-0595 },   // 4.0e-0164
        { 0x1.708D0F84D3DE7p-0543,  0x0.7F67ABAA29E818p-0595 },   // 5.0e-0164
        { 0x1.BA42DF6C31715p-0543,  0x0.CC16013298B020p-0595 },   // 6.0e-0164
        { 0x1.01FC57A9C7822p-0542,  0x0.0C622B5D83BC10p-0594 },   // 7.0e-0164
        { 0x1.26D73F9D764B9p-0542,  0x0.32B95621BB2010p-0594 },   // 8.0e-0164
        { 0x1.4BB2279125150p-0542,  0x0.591080E5F28418p-0594 },   // 9.0e-0164
        { 0x1.708D0F84D3DE7p-0542,  0x0.7F67ABAA29E818p-0594 },   // 1.0e-0163
        { 0x1.708D0F84D3DE7p-0541,  0x0.7F67ABAA29E818p-0593 },   // 2.0e-0163
        { 0x1.1469CBA39EE6Dp-0540,  0x0.9F8DC0BF9F6E10p-0592 },   // 3.0e-0163
        { 0x1.708D0F84D3DE7p-0540,  0x0.7F67ABAA29E818p-0592 },   // 4.0e-0163
        { 0x1.CCB0536608D61p-0540,  0x0.5F419694B46220p-0592 },   // 5.0e-0163
        { 0x1.1469CBA39EE6Dp-0539,  0x0.9F8DC0BF9F6E10p-0591 },   // 6.0e-0163
        { 0x1.427B6D943962Ap-0539,  0x0.8F7AB634E4AB18p-0591 },   // 7.0e-0163
        { 0x1.708D0F84D3DE7p-0539,  0x0.7F67ABAA29E818p-0591 },   // 8.0e-0163
        { 0x1.9E9EB1756E5A4p-0539,  0x0.6F54A11F6F2520p-0591 },   // 9.0e-0163
        { 0x1.CCB0536608D61p-0539,  0x0.5F419694B46220p-0591 },   // 1.0e-0162
        { 0x1.CCB0536608D61p-0538,  0x0.5F419694B46220p-0590 },   // 2.0e-0162
        { 0x1.59843E8C86A09p-0537,  0x0.077130EF874998p-0589 },   // 3.0e-0162
        { 0x1.CCB0536608D61p-0537,  0x0.5F419694B46220p-0589 },   // 4.0e-0162
        { 0x1.1FEE341FC585Cp-0536,  0x0.DB88FE1CF0BD50p-0588 },   // 5.0e-0162
        { 0x1.59843E8C86A09p-0536,  0x0.077130EF874998p-0588 },   // 6.0e-0162
        { 0x1.931A48F947BB5p-0536,  0x0.335963C21DD5E0p-0588 },   // 7.0e-0162
        { 0x1.CCB0536608D61p-0536,  0x0.5F419694B46220p-0588 },   // 8.0e-0162
        { 0x1.03232EE964F86p-0535,  0x0.C594E4B3A57730p-0587 },   // 9.0e-0162
        { 0x1.1FEE341FC585Cp-0535,  0x0.DB88FE1CF0BD50p-0587 },   // 1.0e-0161
        { 0x1.1FEE341FC585Cp-0534,  0x0.DB88FE1CF0BD50p-0586 },   // 2.0e-0161
        { 0x1.AFE54E2FA848Bp-0534,  0x0.494D7D2B691C00p-0586 },   // 3.0e-0161
        { 0x1.1FEE341FC585Cp-0533,  0x0.DB88FE1CF0BD50p-0585 },   // 4.0e-0161
        { 0x1.67E9C127B6E74p-0533,  0x0.126B3DA42CECA8p-0585 },   // 5.0e-0161
        { 0x1.AFE54E2FA848Bp-0533,  0x0.494D7D2B691C00p-0585 },   // 6.0e-0161
        { 0x1.F7E0DB3799AA2p-0533,  0x0.802FBCB2A54B58p-0585 },   // 7.0e-0161
        { 0x1.1FEE341FC585Cp-0532,  0x0.DB88FE1CF0BD50p-0584 },   // 8.0e-0161
        { 0x1.43EBFAA3BE368p-0532,  0x0.76FA1DE08ED500p-0584 },   // 9.0e-0161
        { 0x1.67E9C127B6E74p-0532,  0x0.126B3DA42CECA8p-0584 },   // 1.0e-0160
        { 0x1.67E9C127B6E74p-0531,  0x0.126B3DA42CECA8p-0583 },   // 2.0e-0160
        { 0x1.0DEF50DDC92D7p-0530,  0x0.0DD06E3B21B180p-0582 },   // 3.0e-0160
        { 0x1.67E9C127B6E74p-0530,  0x0.126B3DA42CECA8p-0582 },   // 4.0e-0160
        { 0x1.C1E43171A4A11p-0530,  0x0.17060D0D3827D8p-0582 },   // 5.0e-0160
        { 0x1.0DEF50DDC92D7p-0529,  0x0.0DD06E3B21B180p-0581 },   // 6.0e-0160
        { 0x1.3AEC8902C00A5p-0529,  0x0.901DD5EFA74F10p-0581 },   // 7.0e-0160
        { 0x1.67E9C127B6E74p-0529,  0x0.126B3DA42CECA8p-0581 },   // 8.0e-0160
        { 0x1.94E6F94CADC42p-0529,  0x0.94B8A558B28A40p-0581 },   // 9.0e-0160
        { 0x1.C1E43171A4A11p-0529,  0x0.17060D0D3827D8p-0581 },   // 1.0e-0159
        { 0x1.C1E43171A4A11p-0528,  0x0.17060D0D3827D8p-0580 },   // 2.0e-0159
        { 0x1.516B25153B78Cp-0527,  0x0.D14489C9EA1DE0p-0579 },   // 3.0e-0159
        { 0x1.C1E43171A4A11p-0527,  0x0.17060D0D3827D8p-0579 },   // 4.0e-0159
        { 0x1.192E9EE706E4Ap-0526,  0x0.AE63C8284318E0p-0578 },   // 5.0e-0159
        { 0x1.516B25153B78Cp-0526,  0x0.D14489C9EA1DE0p-0578 },   // 6.0e-0159
        { 0x1.89A7AB43700CEp-0526,  0x0.F4254B6B9122D8p-0578 },   // 7.0e-0159
        { 0x1.C1E43171A4A11p-0526,  0x0.17060D0D3827D8p-0578 },   // 8.0e-0159
        { 0x1.FA20B79FD9353p-0526,  0x0.39E6CEAEDF2CD0p-0578 },   // 9.0e-0159
        { 0x1.192E9EE706E4Ap-0525,  0x0.AE63C8284318E0p-0577 },   // 1.0e-0158
        { 0x1.192E9EE706E4Ap-0524,  0x0.AE63C8284318E0p-0576 },   // 2.0e-0158
        { 0x1.A5C5EE5A8A570p-0524,  0x0.0595AC3C64A558p-0576 },   // 3.0e-0158
        { 0x1.192E9EE706E4Ap-0523,  0x0.AE63C8284318E0p-0575 },   // 4.0e-0158
        { 0x1.5F7A46A0C89DDp-0523,  0x0.59FCBA3253DF20p-0575 },   // 5.0e-0158
        { 0x1.A5C5EE5A8A570p-0523,  0x0.0595AC3C64A558p-0575 },   // 6.0e-0158
        { 0x1.EC1196144C102p-0523,  0x0.B12E9E46756B90p-0575 },   // 7.0e-0158
        { 0x1.192E9EE706E4Ap-0522,  0x0.AE63C8284318E0p-0574 },   // 8.0e-0158
        { 0x1.3C5472C3E7C14p-0522,  0x0.0430412D4B7C00p-0574 },   // 9.0e-0158
        { 0x1.5F7A46A0C89DDp-0522,  0x0.59FCBA3253DF20p-0574 },   // 1.0e-0157
        { 0x1.5F7A46A0C89DDp-0521,  0x0.59FCBA3253DF20p-0573 },   // 2.0e-0157
        { 0x1.079BB4F896766p-0520,  0x0.037D8BA5BEE758p-0572 },   // 3.0e-0157
        { 0x1.5F7A46A0C89DDp-0520,  0x0.59FCBA3253DF20p-0572 },   // 4.0e-0157
        { 0x1.B758D848FAC54p-0520,  0x0.B07BE8BEE8D6E8p-0572 },   // 5.0e-0157
        { 0x1.079BB4F896766p-0519,  0x0.037D8BA5BEE758p-0571 },   // 6.0e-0157
        { 0x1.338AFDCCAF8A1p-0519,  0x0.AEBD22EC096338p-0571 },   // 7.0e-0157
        { 0x1.5F7A46A0C89DDp-0519,  0x0.59FCBA3253DF20p-0571 },   // 8.0e-0157
        { 0x1.8B698F74E1B19p-0519,  0x0.053C51789E5B00p-0571 },   // 9.0e-0157
        { 0x1.B758D848FAC54p-0519,  0x0.B07BE8BEE8D6E8p-0571 },   // 1.0e-0156
        { 0x1.B758D848FAC54p-0518,  0x0.B07BE8BEE8D6E8p-0570 },   // 2.0e-0156
        { 0x1.4982A236BC13Fp-0517,  0x0.845CEE8F2EA128p-0569 },   // 3.0e-0156
        { 0x1.B758D848FAC54p-0517,  0x0.B07BE8BEE8D6E8p-0569 },   // 4.0e-0156
        { 0x1.1297872D9CBB4p-0516,  0x0.EE4D7177518650p-0568 },   // 5.0e-0156
        { 0x1.4982A236BC13Fp-0516,  0x0.845CEE8F2EA128p-0568 },   // 6.0e-0156
        { 0x1.806DBD3FDB6CAp-0516,  0x0.1A6C6BA70BBC08p-0568 },   // 7.0e-0156
        { 0x1.B758D848FAC54p-0516,  0x0.B07BE8BEE8D6E8p-0568 },   // 8.0e-0156
        { 0x1.EE43F3521A1DFp-0516,  0x0.468B65D6C5F1C0p-0568 },   // 9.0e-0156
        { 0x1.1297872D9CBB4p-0515,  0x0.EE4D7177518650p-0567 },   // 1.0e-0155
        { 0x1.1297872D9CBB4p-0514,  0x0.EE4D7177518650p-0566 },   // 2.0e-0155
        { 0x1.9BE34AC46B18Fp-0514,  0x0.65742A32FA4978p-0566 },   // 3.0e-0155
        { 0x1.1297872D9CBB4p-0513,  0x0.EE4D7177518650p-0565 },   // 4.0e-0155
        { 0x1.573D68F903EA2p-0513,  0x0.29E0CDD525E7E0p-0565 },   // 5.0e-0155
        { 0x1.9BE34AC46B18Fp-0513,  0x0.65742A32FA4978p-0565 },   // 6.0e-0155
        { 0x1.E0892C8FD247Cp-0513,  0x0.A1078690CEAB08p-0565 },   // 7.0e-0155
        { 0x1.1297872D9CBB4p-0512,  0x0.EE4D7177518650p-0564 },   // 8.0e-0155
        { 0x1.34EA78135052Bp-0512,  0x0.8C171FA63BB718p-0564 },   // 9.0e-0155
        { 0x1.573D68F903EA2p-0512,  0x0.29E0CDD525E7E0p-0564 },   // 1.0e-0154
        { 0x1.573D68F903EA2p-0511,  0x0.29E0CDD525E7E0p-0563 },   // 2.0e-0154
        { 0x1.016E0EBAC2EF9p-0510,  0x0.9F689A5FDC6DE8p-0562 },   // 3.0e-0154
        { 0x1.573D68F903EA2p-0510,  0x0.29E0CDD525E7E0p-0562 },   // 4.0e-0154
        { 0x1.AD0CC33744E4Ap-0510,  0x0.B459014A6F61D8p-0562 },   // 5.0e-0154
        { 0x1.016E0EBAC2EF9p-0509,  0x0.9F689A5FDC6DE8p-0561 },   // 6.0e-0154
        { 0x1.2C55BBD9E36CDp-0509,  0x0.E4A4B41A812AE8p-0561 },   // 7.0e-0154
        { 0x1.573D68F903EA2p-0509,  0x0.29E0CDD525E7E0p-0561 },   // 8.0e-0154
        { 0x1.8225161824676p-0509,  0x0.6F1CE78FCAA4E0p-0561 },   // 9.0e-0154
        { 0x1.AD0CC33744E4Ap-0509,  0x0.B459014A6F61D8p-0561 },   // 1.0e-0153
        { 0x1.AD0CC33744E4Ap-0508,  0x0.B459014A6F61D8p-0560 },   // 2.0e-0153
        { 0x1.41C9926973AB8p-0507,  0x0.0742C0F7D38960p-0559 },   // 3.0e-0153
        { 0x1.AD0CC33744E4Ap-0507,  0x0.B459014A6F61D8p-0559 },   // 4.0e-0153
        { 0x1.0C27FA028B0EEp-0506,  0x0.B0B7A0CE859D28p-0558 },   // 5.0e-0153
        { 0x1.41C9926973AB8p-0506,  0x0.0742C0F7D38960p-0558 },   // 6.0e-0153
        { 0x1.776B2AD05C481p-0506,  0x0.5DCDE1212175A0p-0558 },   // 7.0e-0153
        { 0x1.AD0CC33744E4Ap-0506,  0x0.B459014A6F61D8p-0558 },   // 8.0e-0153
        { 0x1.E2AE5B9E2D814p-0506,  0x0.0AE42173BD4E18p-0558 },   // 9.0e-0153
        { 0x1.0C27FA028B0EEp-0505,  0x0.B0B7A0CE859D28p-0557 },   // 1.0e-0152
        { 0x1.0C27FA028B0EEp-0504,  0x0.B0B7A0CE859D28p-0556 },   // 2.0e-0152
        { 0x1.923BF703D0966p-0504,  0x0.09137135C86BC0p-0556 },   // 3.0e-0152
        { 0x1.0C27FA028B0EEp-0503,  0x0.B0B7A0CE859D28p-0555 },   // 4.0e-0152
        { 0x1.4F31F8832DD2Ap-0503,  0x0.5CE58902270470p-0555 },   // 5.0e-0152
        { 0x1.923BF703D0966p-0503,  0x0.09137135C86BC0p-0555 },   // 6.0e-0152
        { 0x1.D545F584735A1p-0503,  0x0.B541596969D308p-0555 },   // 7.0e-0152
        { 0x1.0C27FA028B0EEp-0502,  0x0.B0B7A0CE859D28p-0554 },   // 8.0e-0152
        { 0x1.2DACF942DC70Cp-0502,  0x0.86CE94E85650D0p-0554 },   // 9.0e-0152
        { 0x1.4F31F8832DD2Ap-0502,  0x0.5CE58902270470p-0554 },   // 1.0e-0151
        { 0x1.4F31F8832DD2Ap-0501,  0x0.5CE58902270470p-0553 },   // 2.0e-0151
        { 0x1.F6CAF4C4C4BBFp-0501,  0x0.8B584D833A86B0p-0553 },   // 3.0e-0151
        { 0x1.4F31F8832DD2Ap-0500,  0x0.5CE58902270470p-0552 },   // 4.0e-0151
        { 0x1.A2FE76A3F9474p-0500,  0x0.F41EEB42B0C590p-0552 },   // 5.0e-0151
        { 0x1.F6CAF4C4C4BBFp-0500,  0x0.8B584D833A86B0p-0552 },   // 6.0e-0151
        { 0x1.254BB972C8185p-0499,  0x0.1148D7E1E223E8p-0551 },   // 7.0e-0151
        { 0x1.4F31F8832DD2Ap-0499,  0x0.5CE58902270470p-0551 },   // 8.0e-0151
        { 0x1.79183793938CFp-0499,  0x0.A8823A226BE500p-0551 },   // 9.0e-0151
        { 0x1.A2FE76A3F9474p-0499,  0x0.F41EEB42B0C590p-0551 },   // 1.0e-0150
        { 0x1.A2FE76A3F9474p-0498,  0x0.F41EEB42B0C590p-0550 },   // 2.0e-0150
        { 0x1.3A3ED8FAFAF57p-0497,  0x0.B7173072049428p-0549 },   // 3.0e-0150
        { 0x1.A2FE76A3F9474p-0497,  0x0.F41EEB42B0C590p-0549 },   // 4.0e-0150
        { 0x1.05DF0A267BCC9p-0496,  0x0.18935309AE7B78p-0548 },   // 5.0e-0150
        { 0x1.3A3ED8FAFAF57p-0496,  0x0.B7173072049428p-0548 },   // 6.0e-0150
        { 0x1.6E9EA7CF7A1E6p-0496,  0x0.559B0DDA5AACE0p-0548 },   // 7.0e-0150
        { 0x1.A2FE76A3F9474p-0496,  0x0.F41EEB42B0C590p-0548 },   // 8.0e-0150
        { 0x1.D75E457878703p-0496,  0x0.92A2C8AB06DE40p-0548 },   // 9.0e-0150
        { 0x1.05DF0A267BCC9p-0495,  0x0.18935309AE7B78p-0547 },   // 1.0e-0149
        { 0x1.05DF0A267BCC9p-0494,  0x0.18935309AE7B78p-0546 },   // 2.0e-0149
        { 0x1.88CE8F39B9B2Dp-0494,  0x0.A4DCFC8E85B938p-0546 },   // 3.0e-0149
        { 0x1.05DF0A267BCC9p-0493,  0x0.18935309AE7B78p-0545 },   // 4.0e-0149
        { 0x1.4756CCB01ABFBp-0493,  0x0.5EB827CC1A1A58p-0545 },   // 5.0e-0149
        { 0x1.88CE8F39B9B2Dp-0493,  0x0.A4DCFC8E85B938p-0545 },   // 6.0e-0149
        { 0x1.CA4651C358A5Fp-0493,  0x0.EB01D150F15818p-0545 },   // 7.0e-0149
        { 0x1.05DF0A267BCC9p-0492,  0x0.18935309AE7B78p-0544 },   // 8.0e-0149
        { 0x1.269AEB6B4B462p-0492,  0x0.3BA5BD6AE44AE8p-0544 },   // 9.0e-0149
        { 0x1.4756CCB01ABFBp-0492,  0x0.5EB827CC1A1A58p-0544 },   // 1.0e-0148
        { 0x1.4756CCB01ABFBp-0491,  0x0.5EB827CC1A1A58p-0543 },   // 2.0e-0148
        { 0x1.EB023308281F9p-0491,  0x0.0E143BB2272788p-0543 },   // 3.0e-0148
        { 0x1.4756CCB01ABFBp-0490,  0x0.5EB827CC1A1A58p-0542 },   // 4.0e-0148
        { 0x1.992C7FDC216FAp-0490,  0x0.366631BF20A0F0p-0542 },   // 5.0e-0148
        { 0x1.EB023308281F9p-0490,  0x0.0E143BB2272788p-0542 },   // 6.0e-0148
        { 0x1.1E6BF31A1767Bp-0489,  0x0.F2E122D296D710p-0541 },   // 7.0e-0148
        { 0x1.4756CCB01ABFBp-0489,  0x0.5EB827CC1A1A58p-0541 },   // 8.0e-0148
        { 0x1.7041A6461E17Ap-0489,  0x0.CA8F2CC59D5DA0p-0541 },   // 9.0e-0148
        { 0x1.992C7FDC216FAp-0489,  0x0.366631BF20A0F0p-0541 },   // 1.0e-0147
        { 0x1.992C7FDC216FAp-0488,  0x0.366631BF20A0F0p-0540 },   // 2.0e-0147
        { 0x1.32E15FE51913Bp-0487,  0x0.A8CCA54F5878B0p-0539 },   // 3.0e-0147
        { 0x1.992C7FDC216FAp-0487,  0x0.366631BF20A0F0p-0539 },   // 4.0e-0147
        { 0x1.FF779FD329CB8p-0487,  0x0.C3FFBE2EE8C928p-0539 },   // 5.0e-0147
        { 0x1.32E15FE51913Bp-0486,  0x0.A8CCA54F5878B0p-0538 },   // 6.0e-0147
        { 0x1.6606EFE09D41Ap-0486,  0x0.EF996B873C8CD0p-0538 },   // 7.0e-0147
        { 0x1.992C7FDC216FAp-0486,  0x0.366631BF20A0F0p-0538 },   // 8.0e-0147
        { 0x1.CC520FD7A59D9p-0486,  0x0.7D32F7F704B510p-0538 },   // 9.0e-0147
        { 0x1.FF779FD329CB8p-0486,  0x0.C3FFBE2EE8C928p-0538 },   // 1.0e-0146
        { 0x1.FF779FD329CB8p-0485,  0x0.C3FFBE2EE8C928p-0537 },   // 2.0e-0146
        { 0x1.7F99B7DE5F58Ap-0484,  0x0.92FFCEA32E96E0p-0536 },   // 3.0e-0146
        { 0x1.FF779FD329CB8p-0484,  0x0.C3FFBE2EE8C928p-0536 },   // 4.0e-0146
        { 0x1.3FAAC3E3FA1F3p-0483,  0x0.7A7FD6DD517DB8p-0535 },   // 5.0e-0146
        { 0x1.7F99B7DE5F58Ap-0483,  0x0.92FFCEA32E96E0p-0535 },   // 6.0e-0146
        { 0x1.BF88ABD8C4921p-0483,  0x0.AB7FC6690BB008p-0535 },   // 7.0e-0146
        { 0x1.FF779FD329CB8p-0483,  0x0.C3FFBE2EE8C928p-0535 },   // 8.0e-0146
        { 0x1.1FB349E6C7827p-0482,  0x0.EE3FDAFA62F128p-0534 },   // 9.0e-0146
        { 0x1.3FAAC3E3FA1F3p-0482,  0x0.7A7FD6DD517DB8p-0534 },   // 1.0e-0145
        { 0x1.3FAAC3E3FA1F3p-0481,  0x0.7A7FD6DD517DB8p-0533 },   // 2.0e-0145
        { 0x1.DF8025D5F72EDp-0481,  0x0.37BFC24BFA3C98p-0533 },   // 3.0e-0145
        { 0x1.3FAAC3E3FA1F3p-0480,  0x0.7A7FD6DD517DB8p-0532 },   // 4.0e-0145
        { 0x1.8F9574DCF8A70p-0480,  0x0.591FCC94A5DD28p-0532 },   // 5.0e-0145
        { 0x1.DF8025D5F72EDp-0480,  0x0.37BFC24BFA3C98p-0532 },   // 6.0e-0145
        { 0x1.17B56B677ADB5p-0479,  0x0.0B2FDC01A74E00p-0531 },   // 7.0e-0145
        { 0x1.3FAAC3E3FA1F3p-0479,  0x0.7A7FD6DD517DB8p-0531 },   // 8.0e-0145
        { 0x1.67A01C6079631p-0479,  0x0.E9CFD1B8FBAD70p-0531 },   // 9.0e-0145
        { 0x1.8F9574DCF8A70p-0479,  0x0.591FCC94A5DD28p-0531 },   // 1.0e-0144
        { 0x1.8F9574DCF8A70p-0478,  0x0.591FCC94A5DD28p-0530 },   // 2.0e-0144
        { 0x1.2BB017A5BA7D4p-0477,  0x0.42D7D96F7C65E0p-0529 },   // 3.0e-0144
        { 0x1.8F9574DCF8A70p-0477,  0x0.591FCC94A5DD28p-0529 },   // 4.0e-0144
        { 0x1.F37AD21436D0Cp-0477,  0x0.6F67BFB9CF5478p-0529 },   // 5.0e-0144
        { 0x1.2BB017A5BA7D4p-0476,  0x0.42D7D96F7C65E0p-0528 },   // 6.0e-0144
        { 0x1.5DA2C64159922p-0476,  0x0.4DFBD302112180p-0528 },   // 7.0e-0144
        { 0x1.8F9574DCF8A70p-0476,  0x0.591FCC94A5DD28p-0528 },   // 8.0e-0144
        { 0x1.C188237897BBEp-0476,  0x0.6443C6273A98D0p-0528 },   // 9.0e-0144
        { 0x1.F37AD21436D0Cp-0476,  0x0.6F67BFB9CF5478p-0528 },   // 1.0e-0143
        { 0x1.F37AD21436D0Cp-0475,  0x0.6F67BFB9CF5478p-0527 },   // 2.0e-0143
        { 0x1.769C1D8F291C9p-0474,  0x0.538DCFCB5B7F58p-0526 },   // 3.0e-0143
        { 0x1.F37AD21436D0Cp-0474,  0x0.6F67BFB9CF5478p-0526 },   // 4.0e-0143
        { 0x1.382CC34CA2427p-0473,  0x0.C5A0D7D42194C8p-0525 },   // 5.0e-0143
        { 0x1.769C1D8F291C9p-0473,  0x0.538DCFCB5B7F58p-0525 },   // 6.0e-0143
        { 0x1.B50B77D1AFF6Ap-0473,  0x0.E17AC7C29569E8p-0525 },   // 7.0e-0143
        { 0x1.F37AD21436D0Cp-0473,  0x0.6F67BFB9CF5478p-0525 },   // 8.0e-0143
        { 0x1.18F5162B5ED56p-0472,  0x0.FEAA5BD8849F80p-0524 },   // 9.0e-0143
        { 0x1.382CC34CA2427p-0472,  0x0.C5A0D7D42194C8p-0524 },   // 1.0e-0142
        { 0x1.382CC34CA2427p-0471,  0x0.C5A0D7D42194C8p-0523 },   // 2.0e-0142
        { 0x1.D44324F2F363Bp-0471,  0x0.A87143BE325F30p-0523 },   // 3.0e-0142
        { 0x1.382CC34CA2427p-0470,  0x0.C5A0D7D42194C8p-0522 },   // 4.0e-0142
        { 0x1.8637F41FCAD31p-0470,  0x0.B7090DC929F9F8p-0522 },   // 5.0e-0142
        { 0x1.D44324F2F363Bp-0470,  0x0.A87143BE325F30p-0522 },   // 6.0e-0142
        { 0x1.11272AE30DFA2p-0469,  0x0.CCECBCD99D6230p-0521 },   // 7.0e-0142
        { 0x1.382CC34CA2427p-0469,  0x0.C5A0D7D42194C8p-0521 },   // 8.0e-0142
        { 0x1.5F325BB6368ACp-0469,  0x0.BE54F2CEA5C760p-0521 },   // 9.0e-0142
        { 0x1.8637F41FCAD31p-0469,  0x0.B7090DC929F9F8p-0521 },   // 1.0e-0141
        { 0x1.8637F41FCAD31p-0468,  0x0.B7090DC929F9F8p-0520 },   // 2.0e-0141
        { 0x1.24A9F717D81E5p-0467,  0x0.4946CA56DF7B78p-0519 },   // 3.0e-0141
        { 0x1.8637F41FCAD31p-0467,  0x0.B7090DC929F9F8p-0519 },   // 4.0e-0141
        { 0x1.E7C5F127BD87Ep-0467,  0x0.24CB513B747878p-0519 },   // 5.0e-0141
        { 0x1.24A9F717D81E5p-0466,  0x0.4946CA56DF7B78p-0518 },   // 6.0e-0141
        { 0x1.5570F59BD178Bp-0466,  0x0.8027EC1004BAB8p-0518 },   // 7.0e-0141
        { 0x1.8637F41FCAD31p-0466,  0x0.B7090DC929F9F8p-0518 },   // 8.0e-0141
        { 0x1.B6FEF2A3C42D7p-0466,  0x0.EDEA2F824F3938p-0518 },   // 9.0e-0141
        { 0x1.E7C5F127BD87Ep-0466,  0x0.24CB513B747878p-0518 },   // 1.0e-0140
        { 0x1.E7C5F127BD87Ep-0465,  0x0.24CB513B747878p-0517 },   // 2.0e-0140
        { 0x1.6DD474DDCE25Ep-0464,  0x0.9B987CEC975A58p-0516 },   // 3.0e-0140
        { 0x1.E7C5F127BD87Ep-0464,  0x0.24CB513B747878p-0516 },   // 4.0e-0140
        { 0x1.30DBB6B8D674Ep-0463,  0x0.D6FF12C528CB48p-0515 },   // 5.0e-0140
        { 0x1.6DD474DDCE25Ep-0463,  0x0.9B987CEC975A58p-0515 },   // 6.0e-0140
        { 0x1.AACD3302C5D6Ep-0463,  0x0.6031E71405E968p-0515 },   // 7.0e-0140
        { 0x1.E7C5F127BD87Ep-0463,  0x0.24CB513B747878p-0515 },   // 8.0e-0140
        { 0x1.125F57A65A9C6p-0462,  0x0.F4B25DB17183C0p-0514 },   // 9.0e-0140
        { 0x1.30DBB6B8D674Ep-0462,  0x0.D6FF12C528CB48p-0514 },   // 1.0e-0139
        { 0x1.30DBB6B8D674Ep-0461,  0x0.D6FF12C528CB48p-0513 },   // 2.0e-0139
        { 0x1.C949921541AF6p-0461,  0x0.427E9C27BD30F0p-0513 },   // 3.0e-0139
        { 0x1.30DBB6B8D674Ep-0460,  0x0.D6FF12C528CB48p-0512 },   // 4.0e-0139
        { 0x1.7D12A4670C122p-0460,  0x0.8CBED77672FE20p-0512 },   // 5.0e-0139
        { 0x1.C949921541AF6p-0460,  0x0.427E9C27BD30F0p-0512 },   // 6.0e-0139
        { 0x1.0AC03FE1BBA64p-0459,  0x0.FC1F306C83B1E0p-0511 },   // 7.0e-0139
        { 0x1.30DBB6B8D674Ep-0459,  0x0.D6FF12C528CB48p-0511 },   // 8.0e-0139
        { 0x1.56F72D8FF1438p-0459,  0x0.B1DEF51DCDE4B8p-0511 },   // 9.0e-0139
        { 0x1.7D12A4670C122p-0459,  0x0.8CBED77672FE20p-0511 },   // 1.0e-0138
        { 0x1.7D12A4670C122p-0458,  0x0.8CBED77672FE20p-0510 },   // 2.0e-0138
        { 0x1.1DCDFB4D490D9p-0457,  0x0.E98F2198D63E98p-0509 },   // 3.0e-0138
        { 0x1.7D12A4670C122p-0457,  0x0.8CBED77672FE20p-0509 },   // 4.0e-0138
        { 0x1.DC574D80CF16Bp-0457,  0x0.2FEE8D540FBDA8p-0509 },   // 5.0e-0138
        { 0x1.1DCDFB4D490D9p-0456,  0x0.E98F2198D63E98p-0508 },   // 6.0e-0138
        { 0x1.4D704FDA2A8FEp-0456,  0x0.3B26FC87A49E58p-0508 },   // 7.0e-0138
        { 0x1.7D12A4670C122p-0456,  0x0.8CBED77672FE20p-0508 },   // 8.0e-0138
        { 0x1.ACB4F8F3ED946p-0456,  0x0.DE56B265415DE0p-0508 },   // 9.0e-0138
        { 0x1.DC574D80CF16Bp-0456,  0x0.2FEE8D540FBDA8p-0508 },   // 1.0e-0137
        { 0x1.DC574D80CF16Bp-0455,  0x0.2FEE8D540FBDA8p-0507 },   // 2.0e-0137
        { 0x1.65417A209B510p-0454,  0x0.63F2E9FF0BCE40p-0506 },   // 3.0e-0137
        { 0x1.DC574D80CF16Bp-0454,  0x0.2FEE8D540FBDA8p-0506 },   // 4.0e-0137
        { 0x1.29B69070816E2p-0453,  0x0.FDF5185489D688p-0505 },   // 5.0e-0137
        { 0x1.65417A209B510p-0453,  0x0.63F2E9FF0BCE40p-0505 },   // 6.0e-0137
        { 0x1.A0CC63D0B533Dp-0453,  0x0.C9F0BBA98DC5F0p-0505 },   // 7.0e-0137
        { 0x1.DC574D80CF16Bp-0453,  0x0.2FEE8D540FBDA8p-0505 },   // 8.0e-0137
        { 0x1.0BF11B98747CCp-0452,  0x0.4AF62F7F48DAB0p-0504 },   // 9.0e-0137
        { 0x1.29B69070816E2p-0452,  0x0.FDF5185489D688p-0504 },   // 1.0e-0136
        { 0x1.29B69070816E2p-0451,  0x0.FDF5185489D688p-0503 },   // 2.0e-0136
        { 0x1.BE91D8A8C2254p-0451,  0x0.7CEFA47ECEC1D0p-0503 },   // 3.0e-0136
        { 0x1.29B69070816E2p-0450,  0x0.FDF5185489D688p-0502 },   // 4.0e-0136
        { 0x1.7424348CA1C9Bp-0450,  0x0.BD725E69AC4C28p-0502 },   // 5.0e-0136
        { 0x1.BE91D8A8C2254p-0450,  0x0.7CEFA47ECEC1D0p-0502 },   // 6.0e-0136
        { 0x1.047FBE6271406p-0449,  0x0.9E367549F89BB8p-0501 },   // 7.0e-0136
        { 0x1.29B69070816E2p-0449,  0x0.FDF5185489D688p-0501 },   // 8.0e-0136
        { 0x1.4EED627E919BFp-0449,  0x0.5DB3BB5F1B1158p-0501 },   // 9.0e-0136
        { 0x1.7424348CA1C9Bp-0449,  0x0.BD725E69AC4C28p-0501 },   // 1.0e-0135
        { 0x1.7424348CA1C9Bp-0448,  0x0.BD725E69AC4C28p-0500 },   // 2.0e-0135
        { 0x1.171B276979574p-0447,  0x0.CE15C6CF413920p-0499 },   // 3.0e-0135
        { 0x1.7424348CA1C9Bp-0447,  0x0.BD725E69AC4C28p-0499 },   // 4.0e-0135
        { 0x1.D12D41AFCA3C2p-0447,  0x0.ACCEF604175F38p-0499 },   // 5.0e-0135
        { 0x1.171B276979574p-0446,  0x0.CE15C6CF413920p-0498 },   // 6.0e-0135
        { 0x1.459FADFB0D908p-0446,  0x0.45C4129C76C2A0p-0498 },   // 7.0e-0135
        { 0x1.7424348CA1C9Bp-0446,  0x0.BD725E69AC4C28p-0498 },   // 8.0e-0135
        { 0x1.A2A8BB1E3602Fp-0446,  0x0.3520AA36E1D5B0p-0498 },   // 9.0e-0135
        { 0x1.D12D41AFCA3C2p-0446,  0x0.ACCEF604175F38p-0498 },   // 1.0e-0134
        { 0x1.D12D41AFCA3C2p-0445,  0x0.ACCEF604175F38p-0497 },   // 2.0e-0134
        { 0x1.5CE1F143D7AD2p-0444,  0x0.019B3883118768p-0496 },   // 3.0e-0134
        { 0x1.D12D41AFCA3C2p-0444,  0x0.ACCEF604175F38p-0496 },   // 4.0e-0134
        { 0x1.22BC490DDE659p-0443,  0x0.AC0159C28E9B80p-0495 },   // 5.0e-0134
        { 0x1.5CE1F143D7AD2p-0443,  0x0.019B3883118768p-0495 },   // 6.0e-0134
        { 0x1.97079979D0F4Ap-0443,  0x0.57351743947350p-0495 },   // 7.0e-0134
        { 0x1.D12D41AFCA3C2p-0443,  0x0.ACCEF604175F38p-0495 },   // 8.0e-0134
        { 0x1.05A974F2E1C1Dp-0442,  0x0.81346A624D2590p-0494 },   // 9.0e-0134
        { 0x1.22BC490DDE659p-0442,  0x0.AC0159C28E9B80p-0494 },   // 1.0e-0133
        { 0x1.22BC490DDE659p-0441,  0x0.AC0159C28E9B80p-0493 },   // 2.0e-0133
        { 0x1.B41A6D94CD986p-0441,  0x0.820206A3D5E940p-0493 },   // 3.0e-0133
        { 0x1.22BC490DDE659p-0440,  0x0.AC0159C28E9B80p-0492 },   // 4.0e-0133
        { 0x1.6B6B5B5155FF0p-0440,  0x0.1701B033324260p-0492 },   // 5.0e-0133
        { 0x1.B41A6D94CD986p-0440,  0x0.820206A3D5E940p-0492 },   // 6.0e-0133
        { 0x1.FCC97FD84531Cp-0440,  0x0.ED025D14799020p-0492 },   // 7.0e-0133
        { 0x1.22BC490DDE659p-0439,  0x0.AC0159C28E9B80p-0491 },   // 8.0e-0133
        { 0x1.4713D22F9A324p-0439,  0x0.E18184FAE06EF0p-0491 },   // 9.0e-0133
        { 0x1.6B6B5B5155FF0p-0439,  0x0.1701B033324260p-0491 },   // 1.0e-0132
        { 0x1.6B6B5B5155FF0p-0438,  0x0.1701B033324260p-0490 },   // 2.0e-0132
        { 0x1.1090847D007F4p-0437,  0x0.1141442665B1C8p-0489 },   // 3.0e-0132
        { 0x1.6B6B5B5155FF0p-0437,  0x0.1701B033324260p-0489 },   // 4.0e-0132
        { 0x1.C6463225AB7ECp-0437,  0x0.1CC21C3FFED2F8p-0489 },   // 5.0e-0132
        { 0x1.1090847D007F4p-0436,  0x0.1141442665B1C8p-0488 },   // 6.0e-0132
        { 0x1.3DFDEFE72B3F2p-0436,  0x0.14217A2CCBFA10p-0488 },   // 7.0e-0132
        { 0x1.6B6B5B5155FF0p-0436,  0x0.1701B033324260p-0488 },   // 8.0e-0132
        { 0x1.98D8C6BB80BEEp-0436,  0x0.19E1E639988AB0p-0488 },   // 9.0e-0132
        { 0x1.C6463225AB7ECp-0436,  0x0.1CC21C3FFED2F8p-0488 },   // 1.0e-0131
        { 0x1.C6463225AB7ECp-0435,  0x0.1CC21C3FFED2F8p-0487 },   // 2.0e-0131
        { 0x1.54B4A59C409F1p-0434,  0x0.1591952FFF1E38p-0486 },   // 3.0e-0131
        { 0x1.C6463225AB7ECp-0434,  0x0.1CC21C3FFED2F8p-0486 },   // 4.0e-0131
        { 0x1.1BEBDF578B2F3p-0433,  0x0.91F951A7FF43D8p-0485 },   // 5.0e-0131
        { 0x1.54B4A59C409F1p-0433,  0x0.1591952FFF1E38p-0485 },   // 6.0e-0131
        { 0x1.8D7D6BE0F60EEp-0433,  0x0.9929D8B7FEF898p-0485 },   // 7.0e-0131
        { 0x1.C6463225AB7ECp-0433,  0x0.1CC21C3FFED2F8p-0485 },   // 8.0e-0131
        { 0x1.FF0EF86A60EE9p-0433,  0x0.A05A5FC7FEAD58p-0485 },   // 9.0e-0131
        { 0x1.1BEBDF578B2F3p-0432,  0x0.91F951A7FF43D8p-0484 },   // 1.0e-0130
        { 0x1.1BEBDF578B2F3p-0431,  0x0.91F951A7FF43D8p-0483 },   // 2.0e-0130
        { 0x1.A9E1CF0350C6Dp-0431,  0x0.5AF5FA7BFEE5C8p-0483 },   // 3.0e-0130
        { 0x1.1BEBDF578B2F3p-0430,  0x0.91F951A7FF43D8p-0482 },   // 4.0e-0130
        { 0x1.62E6D72D6DFB0p-0430,  0x0.7677A611FF14D0p-0482 },   // 5.0e-0130
        { 0x1.A9E1CF0350C6Dp-0430,  0x0.5AF5FA7BFEE5C8p-0482 },   // 6.0e-0130
        { 0x1.F0DCC6D93392Ap-0430,  0x0.3F744EE5FEB6C0p-0482 },   // 7.0e-0130
        { 0x1.1BEBDF578B2F3p-0429,  0x0.91F951A7FF43D8p-0481 },   // 8.0e-0130
        { 0x1.3F695B427C952p-0429,  0x0.04387BDCFF2C58p-0481 },   // 9.0e-0130
        { 0x1.62E6D72D6DFB0p-0429,  0x0.7677A611FF14D0p-0481 },   // 1.0e-0129
        { 0x1.62E6D72D6DFB0p-0428,  0x0.7677A611FF14D0p-0480 },   // 2.0e-0129
        { 0x1.0A2D2162127C4p-0427,  0x0.58D9BC8D7F4FA0p-0479 },   // 3.0e-0129
        { 0x1.62E6D72D6DFB0p-0427,  0x0.7677A611FF14D0p-0479 },   // 4.0e-0129
        { 0x1.BBA08CF8C979Cp-0427,  0x0.94158F967EDA08p-0479 },   // 5.0e-0129
        { 0x1.0A2D2162127C4p-0426,  0x0.58D9BC8D7F4FA0p-0478 },   // 6.0e-0129
        { 0x1.3689FC47C03BAp-0426,  0x0.67A8B14FBF3238p-0478 },   // 7.0e-0129
        { 0x1.62E6D72D6DFB0p-0426,  0x0.7677A611FF14D0p-0478 },   // 8.0e-0129
        { 0x1.8F43B2131BBA6p-0426,  0x0.85469AD43EF770p-0478 },   // 9.0e-0129
        { 0x1.BBA08CF8C979Cp-0426,  0x0.94158F967EDA08p-0478 },   // 1.0e-0128
        { 0x1.BBA08CF8C979Cp-0425,  0x0.94158F967EDA08p-0477 },   // 2.0e-0128
        { 0x1.4CB869BA971B5p-0424,  0x0.6F102BB0DF2388p-0476 },   // 3.0e-0128
        { 0x1.BBA08CF8C979Cp-0424,  0x0.94158F967EDA08p-0476 },   // 4.0e-0128
        { 0x1.1544581B7DEC1p-0423,  0x0.DC8D79BE0F4840p-0475 },   // 5.0e-0128
        { 0x1.4CB869BA971B5p-0423,  0x0.6F102BB0DF2388p-0475 },   // 6.0e-0128
        { 0x1.842C7B59B04A9p-0423,  0x0.0192DDA3AEFEC8p-0475 },   // 7.0e-0128
        { 0x1.BBA08CF8C979Cp-0423,  0x0.94158F967EDA08p-0475 },   // 8.0e-0128
        { 0x1.F3149E97E2A90p-0423,  0x0.269841894EB548p-0475 },   // 9.0e-0128
        { 0x1.1544581B7DEC1p-0422,  0x0.DC8D79BE0F4840p-0474 },   // 1.0e-0127
        { 0x1.1544581B7DEC1p-0421,  0x0.DC8D79BE0F4840p-0473 },   // 2.0e-0127
        { 0x1.9FE684293CE22p-0421,  0x0.CAD4369D16EC68p-0473 },   // 3.0e-0127
        { 0x1.1544581B7DEC1p-0420,  0x0.DC8D79BE0F4840p-0472 },   // 4.0e-0127
        { 0x1.5A956E225D672p-0420,  0x0.53B0D82D931A58p-0472 },   // 5.0e-0127
        { 0x1.9FE684293CE22p-0420,  0x0.CAD4369D16EC68p-0472 },   // 6.0e-0127
        { 0x1.E5379A301C5D3p-0420,  0x0.41F7950C9ABE78p-0472 },   // 7.0e-0127
        { 0x1.1544581B7DEC1p-0419,  0x0.DC8D79BE0F4840p-0471 },   // 8.0e-0127
        { 0x1.37ECE31EEDA9Ap-0419,  0x0.181F28F5D13150p-0471 },   // 9.0e-0127
        { 0x1.5A956E225D672p-0419,  0x0.53B0D82D931A58p-0471 },   // 1.0e-0126
        { 0x1.5A956E225D672p-0418,  0x0.53B0D82D931A58p-0470 },   // 2.0e-0126
        { 0x1.03F01299C60D5p-0417,  0x0.BEC4A2222E53C0p-0469 },   // 3.0e-0126
        { 0x1.5A956E225D672p-0417,  0x0.53B0D82D931A58p-0469 },   // 4.0e-0126
        { 0x1.B13AC9AAF4C0Ep-0417,  0x0.E89D0E38F7E0E8p-0469 },   // 5.0e-0126
        { 0x1.03F01299C60D5p-0416,  0x0.BEC4A2222E53C0p-0468 },   // 6.0e-0126
        { 0x1.2F42C05E11BA4p-0416,  0x0.093ABD27E0B708p-0468 },   // 7.0e-0126
        { 0x1.5A956E225D672p-0416,  0x0.53B0D82D931A58p-0468 },   // 8.0e-0126
        { 0x1.85E81BE6A9140p-0416,  0x0.9E26F333457DA0p-0468 },   // 9.0e-0126
        { 0x1.B13AC9AAF4C0Ep-0416,  0x0.E89D0E38F7E0E8p-0468 },   // 1.0e-0125
        { 0x1.B13AC9AAF4C0Ep-0415,  0x0.E89D0E38F7E0E8p-0467 },   // 2.0e-0125
        { 0x1.44EC17403790Bp-0414,  0x0.2E75CAAAB9E8B0p-0466 },   // 3.0e-0125
        { 0x1.B13AC9AAF4C0Ep-0414,  0x0.E89D0E38F7E0E8p-0466 },   // 4.0e-0125
        { 0x1.0EC4BE0AD8F89p-0413,  0x0.516228E39AEC90p-0465 },   // 5.0e-0125
        { 0x1.44EC17403790Bp-0413,  0x0.2E75CAAAB9E8B0p-0465 },   // 6.0e-0125
        { 0x1.7B1370759628Dp-0413,  0x0.0B896C71D8E4D0p-0465 },   // 7.0e-0125
        { 0x1.B13AC9AAF4C0Ep-0413,  0x0.E89D0E38F7E0E8p-0465 },   // 8.0e-0125
        { 0x1.E76222E053590p-0413,  0x0.C5B0B00016DD08p-0465 },   // 9.0e-0125
        { 0x1.0EC4BE0AD8F89p-0412,  0x0.516228E39AEC90p-0464 },   // 1.0e-0124
        { 0x1.0EC4BE0AD8F89p-0411,  0x0.516228E39AEC90p-0463 },   // 2.0e-0124
        { 0x1.96271D104574Dp-0411,  0x0.FA133D556862E0p-0463 },   // 3.0e-0124
        { 0x1.0EC4BE0AD8F89p-0410,  0x0.516228E39AEC90p-0462 },   // 4.0e-0124
        { 0x1.5275ED8D8F36Bp-0410,  0x0.A5BAB31C81A7B8p-0462 },   // 5.0e-0124
        { 0x1.96271D104574Dp-0410,  0x0.FA133D556862E0p-0462 },   // 6.0e-0124
        { 0x1.D9D84C92FBB30p-0410,  0x0.4E6BC78E4F1E00p-0462 },   // 7.0e-0124
        { 0x1.0EC4BE0AD8F89p-0409,  0x0.516228E39AEC90p-0461 },   // 8.0e-0124
        { 0x1.309D55CC3417Ap-0409,  0x0.7B8E6E000E4A28p-0461 },   // 9.0e-0124
        { 0x1.5275ED8D8F36Bp-0409,  0x0.A5BAB31C81A7B8p-0461 },   // 1.0e-0123
        { 0x1.5275ED8D8F36Bp-0408,  0x0.A5BAB31C81A7B8p-0460 },   // 2.0e-0123
        { 0x1.FBB0E45456D21p-0408,  0x0.78980CAAC27B98p-0460 },   // 3.0e-0123
        { 0x1.5275ED8D8F36Bp-0407,  0x0.A5BAB31C81A7B8p-0459 },   // 4.0e-0123
        { 0x1.A71368F0F3046p-0407,  0x0.8F295FE3A211A8p-0459 },   // 5.0e-0123
        { 0x1.FBB0E45456D21p-0407,  0x0.78980CAAC27B98p-0459 },   // 6.0e-0123
        { 0x1.28272FDBDD4FEp-0406,  0x0.31035CB8F172C0p-0458 },   // 7.0e-0123
        { 0x1.5275ED8D8F36Bp-0406,  0x0.A5BAB31C81A7B8p-0458 },   // 8.0e-0123
        { 0x1.7CC4AB3F411D9p-0406,  0x0.1A72098011DCB0p-0458 },   // 9.0e-0123
        { 0x1.A71368F0F3046p-0406,  0x0.8F295FE3A211A8p-0458 },   // 1.0e-0122
        { 0x1.A71368F0F3046p-0405,  0x0.8F295FE3A211A8p-0457 },   // 2.0e-0122
        { 0x1.3D4E8EB4B6434p-0404,  0x0.EB5F07EAB98D38p-0456 },   // 3.0e-0122
        { 0x1.A71368F0F3046p-0404,  0x0.8F295FE3A211A8p-0456 },   // 4.0e-0122
        { 0x1.086C219697E2Cp-0403,  0x0.1979DBEE454B08p-0455 },   // 5.0e-0122
        { 0x1.3D4E8EB4B6434p-0403,  0x0.EB5F07EAB98D38p-0455 },   // 6.0e-0122
        { 0x1.7230FBD2D4A3Dp-0403,  0x0.BD4433E72DCF70p-0455 },   // 7.0e-0122
        { 0x1.A71368F0F3046p-0403,  0x0.8F295FE3A211A8p-0455 },   // 8.0e-0122
        { 0x1.DBF5D60F1164Fp-0403,  0x0.610E8BE01653D8p-0455 },   // 9.0e-0122
        { 0x1.086C219697E2Cp-0402,  0x0.1979DBEE454B08p-0454 },   // 1.0e-0121
        { 0x1.086C219697E2Cp-0401,  0x0.1979DBEE454B08p-0453 },   // 2.0e-0121
        { 0x1.8CA23261E3D42p-0401,  0x0.2636C9E567F088p-0453 },   // 3.0e-0121
        { 0x1.086C219697E2Cp-0400,  0x0.1979DBEE454B08p-0452 },   // 4.0e-0121
        { 0x1.4A8729FC3DDB7p-0400,  0x0.1FD852E9D69DC8p-0452 },   // 5.0e-0121
        { 0x1.8CA23261E3D42p-0400,  0x0.2636C9E567F088p-0452 },   // 6.0e-0121
        { 0x1.CEBD3AC789CCDp-0400,  0x0.2C9540E0F94350p-0452 },   // 7.0e-0121
        { 0x1.086C219697E2Cp-0399,  0x0.1979DBEE454B08p-0451 },   // 8.0e-0121
        { 0x1.2979A5C96ADF1p-0399,  0x0.9CA9176C0DF468p-0451 },   // 9.0e-0121
        { 0x1.4A8729FC3DDB7p-0399,  0x0.1FD852E9D69DC8p-0451 },   // 1.0e-0120
        { 0x1.4A8729FC3DDB7p-0398,  0x0.1FD852E9D69DC8p-0450 },   // 2.0e-0120
        { 0x1.EFCABEFA5CC92p-0398,  0x0.AFC47C5EC1ECB0p-0450 },   // 3.0e-0120
        { 0x1.4A8729FC3DDB7p-0397,  0x0.1FD852E9D69DC8p-0449 },   // 4.0e-0120
        { 0x1.9D28F47B4D524p-0397,  0x0.E7CE67A44C4538p-0449 },   // 5.0e-0120
        { 0x1.EFCABEFA5CC92p-0397,  0x0.AFC47C5EC1ECB0p-0449 },   // 6.0e-0120
        { 0x1.213644BCB6200p-0396,  0x0.3BDD488C9BCA10p-0448 },   // 7.0e-0120
        { 0x1.4A8729FC3DDB7p-0396,  0x0.1FD852E9D69DC8p-0448 },   // 8.0e-0120
        { 0x1.73D80F3BC596Ep-0396,  0x0.03D35D47117180p-0448 },   // 9.0e-0120
        { 0x1.9D28F47B4D524p-0396,  0x0.E7CE67A44C4538p-0448 },   // 1.0e-0119
        { 0x1.9D28F47B4D524p-0395,  0x0.E7CE67A44C4538p-0447 },   // 2.0e-0119
        { 0x1.35DEB75C79FDBp-0394,  0x0.ADDACDBB3933E8p-0446 },   // 3.0e-0119
        { 0x1.9D28F47B4D524p-0394,  0x0.E7CE67A44C4538p-0446 },   // 4.0e-0119
        { 0x1.023998CD10537p-0393,  0x0.10E100C6AFAB40p-0445 },   // 5.0e-0119
        { 0x1.35DEB75C79FDBp-0393,  0x0.ADDACDBB3933E8p-0445 },   // 6.0e-0119
        { 0x1.6983D5EBE3A80p-0393,  0x0.4AD49AAFC2BC90p-0445 },   // 7.0e-0119
        { 0x1.9D28F47B4D524p-0393,  0x0.E7CE67A44C4538p-0445 },   // 8.0e-0119
        { 0x1.D0CE130AB6FC9p-0393,  0x0.84C83498D5CDE0p-0445 },   // 9.0e-0119
        { 0x1.023998CD10537p-0392,  0x0.10E100C6AFAB40p-0444 },   // 1.0e-0118
        { 0x1.023998CD10537p-0391,  0x0.10E100C6AFAB40p-0443 },   // 2.0e-0118
        { 0x1.83566533987D2p-0391,  0x0.9951812A0780E8p-0443 },   // 3.0e-0118
        { 0x1.023998CD10537p-0390,  0x0.10E100C6AFAB40p-0442 },   // 4.0e-0118
        { 0x1.42C7FF0054684p-0390,  0x0.D51940F85B9618p-0442 },   // 5.0e-0118
        { 0x1.83566533987D2p-0390,  0x0.9951812A0780E8p-0442 },   // 6.0e-0118
        { 0x1.C3E4CB66DC920p-0390,  0x0.5D89C15BB36BB8p-0442 },   // 7.0e-0118
        { 0x1.023998CD10537p-0389,  0x0.10E100C6AFAB40p-0441 },   // 8.0e-0118
        { 0x1.2280CBE6B25DDp-0389,  0x0.F2FD20DF85A0B0p-0441 },   // 9.0e-0118
        { 0x1.42C7FF0054684p-0389,  0x0.D51940F85B9618p-0441 },   // 1.0e-0117
        { 0x1.42C7FF0054684p-0388,  0x0.D51940F85B9618p-0440 },   // 2.0e-0117
        { 0x1.E42BFE807E9C7p-0388,  0x0.3FA5E174896120p-0440 },   // 3.0e-0117
        { 0x1.42C7FF0054684p-0387,  0x0.D51940F85B9618p-0439 },   // 4.0e-0117
        { 0x1.9379FEC069826p-0387,  0x0.0A5F9136727BA0p-0439 },   // 5.0e-0117
        { 0x1.E42BFE807E9C7p-0387,  0x0.3FA5E174896120p-0439 },   // 6.0e-0117
        { 0x1.1A6EFF2049DB4p-0386,  0x0.3A7618D9502350p-0438 },   // 7.0e-0117
        { 0x1.42C7FF0054684p-0386,  0x0.D51940F85B9618p-0438 },   // 8.0e-0117
        { 0x1.6B20FEE05EF55p-0386,  0x0.6FBC69176708D8p-0438 },   // 9.0e-0117
        { 0x1.9379FEC069826p-0386,  0x0.0A5F9136727BA0p-0438 },   // 1.0e-0116
        { 0x1.9379FEC069826p-0385,  0x0.0A5F9136727BA0p-0437 },   // 2.0e-0116
        { 0x1.2E9B7F104F21Cp-0384,  0x0.87C7ACE8D5DCB8p-0436 },   // 3.0e-0116
        { 0x1.9379FEC069826p-0384,  0x0.0A5F9136727BA0p-0436 },   // 4.0e-0116
        { 0x1.F8587E7083E2Fp-0384,  0x0.8CF775840F1A88p-0436 },   // 5.0e-0116
        { 0x1.2E9B7F104F21Cp-0383,  0x0.87C7ACE8D5DCB8p-0435 },   // 6.0e-0116
        { 0x1.610ABEE85C521p-0383,  0x0.49139F0FA42C28p-0435 },   // 7.0e-0116
        { 0x1.9379FEC069826p-0383,  0x0.0A5F9136727BA0p-0435 },   // 8.0e-0116
        { 0x1.C5E93E9876B2Ap-0383,  0x0.CBAB835D40CB10p-0435 },   // 9.0e-0116
        { 0x1.F8587E7083E2Fp-0383,  0x0.8CF775840F1A88p-0435 },   // 1.0e-0115
        { 0x1.F8587E7083E2Fp-0382,  0x0.8CF775840F1A88p-0434 },   // 2.0e-0115
        { 0x1.7A425ED462EA3p-0381,  0x0.A9B998230B53E0p-0433 },   // 3.0e-0115
        { 0x1.F8587E7083E2Fp-0381,  0x0.8CF775840F1A88p-0433 },   // 4.0e-0115
        { 0x1.3B374F06526DDp-0380,  0x0.B81AA972897090p-0432 },   // 5.0e-0115
        { 0x1.7A425ED462EA3p-0380,  0x0.A9B998230B53E0p-0432 },   // 6.0e-0115
        { 0x1.B94D6EA273669p-0380,  0x0.9B5886D38D3730p-0432 },   // 7.0e-0115
        { 0x1.F8587E7083E2Fp-0380,  0x0.8CF775840F1A88p-0432 },   // 8.0e-0115
        { 0x1.1BB1C71F4A2FAp-0379,  0x0.BF4B321A487EE8p-0431 },   // 9.0e-0115
        { 0x1.3B374F06526DDp-0379,  0x0.B81AA972897090p-0431 },   // 1.0e-0114
        { 0x1.3B374F06526DDp-0378,  0x0.B81AA972897090p-0430 },   // 2.0e-0114
        { 0x1.D8D2F6897BA4Cp-0378,  0x0.9427FE2BCE28E0p-0430 },   // 3.0e-0114
        { 0x1.3B374F06526DDp-0377,  0x0.B81AA972897090p-0429 },   // 4.0e-0114
        { 0x1.8A0522C7E7095p-0377,  0x0.262153CF2BCCB8p-0429 },   // 5.0e-0114
        { 0x1.D8D2F6897BA4Cp-0377,  0x0.9427FE2BCE28E0p-0429 },   // 6.0e-0114
        { 0x1.13D0652588202p-0376,  0x0.01175444384280p-0428 },   // 7.0e-0114
        { 0x1.3B374F06526DDp-0376,  0x0.B81AA972897090p-0428 },   // 8.0e-0114
        { 0x1.629E38E71CBB9p-0376,  0x0.6F1DFEA0DA9EA8p-0428 },   // 9.0e-0114
        { 0x1.8A0522C7E7095p-0376,  0x0.262153CF2BCCB8p-0428 },   // 1.0e-0113
        { 0x1.8A0522C7E7095p-0375,  0x0.262153CF2BCCB8p-0427 },   // 2.0e-0113
        { 0x1.2783DA15ED46Fp-0374,  0x0.DC98FEDB60D988p-0426 },   // 3.0e-0113
        { 0x1.8A0522C7E7095p-0374,  0x0.262153CF2BCCB8p-0426 },   // 4.0e-0113
        { 0x1.EC866B79E0CBAp-0374,  0x0.6FA9A8C2F6BFE8p-0426 },   // 5.0e-0113
        { 0x1.2783DA15ED46Fp-0373,  0x0.DC98FEDB60D988p-0425 },   // 6.0e-0113
        { 0x1.58C47E6EEA282p-0373,  0x0.815D2955465320p-0425 },   // 7.0e-0113
        { 0x1.8A0522C7E7095p-0373,  0x0.262153CF2BCCB8p-0425 },   // 8.0e-0113
        { 0x1.BB45C720E3EA7p-0373,  0x0.CAE57E49114650p-0425 },   // 9.0e-0113
        { 0x1.EC866B79E0CBAp-0373,  0x0.6FA9A8C2F6BFE8p-0425 },   // 1.0e-0112
        { 0x1.EC866B79E0CBAp-0372,  0x0.6FA9A8C2F6BFE8p-0424 },   // 2.0e-0112
        { 0x1.7164D09B6898Bp-0371,  0x0.D3BF3E92390FE8p-0423 },   // 3.0e-0112
        { 0x1.EC866B79E0CBAp-0371,  0x0.6FA9A8C2F6BFE8p-0423 },   // 4.0e-0112
        { 0x1.33D4032C2C7F4p-0370,  0x0.85CA0979DA37F0p-0422 },   // 5.0e-0112
        { 0x1.7164D09B6898Bp-0370,  0x0.D3BF3E92390FE8p-0422 },   // 6.0e-0112
        { 0x1.AEF59E0AA4B23p-0370,  0x0.21B473AA97E7E8p-0422 },   // 7.0e-0112
        { 0x1.EC866B79E0CBAp-0370,  0x0.6FA9A8C2F6BFE8p-0422 },   // 8.0e-0112
        { 0x1.150B9C748E728p-0369,  0x0.DECF6EEDAACBF0p-0421 },   // 9.0e-0112
        { 0x1.33D4032C2C7F4p-0369,  0x0.85CA0979DA37F0p-0421 },   // 1.0e-0111
        { 0x1.33D4032C2C7F4p-0368,  0x0.85CA0979DA37F0p-0420 },   // 2.0e-0111
        { 0x1.CDBE04C242BEEp-0368,  0x0.C8AF0E36C753E8p-0420 },   // 3.0e-0111
        { 0x1.33D4032C2C7F4p-0367,  0x0.85CA0979DA37F0p-0419 },   // 4.0e-0111
        { 0x1.80C903F7379F1p-0367,  0x0.A73C8BD850C5E8p-0419 },   // 5.0e-0111
        { 0x1.CDBE04C242BEEp-0367,  0x0.C8AF0E36C753E8p-0419 },   // 6.0e-0111
        { 0x1.0D5982C6A6EF5p-0366,  0x0.F510C84A9EF0F0p-0418 },   // 7.0e-0111
        { 0x1.33D4032C2C7F4p-0366,  0x0.85CA0979DA37F0p-0418 },   // 8.0e-0111
        { 0x1.5A4E8391B20F3p-0366,  0x0.16834AA9157EF0p-0418 },   // 9.0e-0111
        { 0x1.80C903F7379F1p-0366,  0x0.A73C8BD850C5E8p-0418 },   // 1.0e-0110
        { 0x1.80C903F7379F1p-0365,  0x0.A73C8BD850C5E8p-0417 },   // 2.0e-0110
        { 0x1.2096C2F969B75p-0364,  0x0.3D6D68E23C9470p-0416 },   // 3.0e-0110
        { 0x1.80C903F7379F1p-0364,  0x0.A73C8BD850C5E8p-0416 },   // 4.0e-0110
        { 0x1.E0FB44F50586Ep-0364,  0x0.110BAECE64F768p-0416 },   // 5.0e-0110
        { 0x1.2096C2F969B75p-0363,  0x0.3D6D68E23C9470p-0415 },   // 6.0e-0110
        { 0x1.50AFE37850AB3p-0363,  0x0.7254FA5D46AD30p-0415 },   // 7.0e-0110
        { 0x1.80C903F7379F1p-0363,  0x0.A73C8BD850C5E8p-0415 },   // 8.0e-0110
        { 0x1.B0E224761E92Fp-0363,  0x0.DC241D535ADEA8p-0415 },   // 9.0e-0110
        { 0x1.E0FB44F50586Ep-0363,  0x0.110BAECE64F768p-0415 },   // 1.0e-0109
        { 0x1.E0FB44F50586Ep-0362,  0x0.110BAECE64F768p-0414 },   // 2.0e-0109
        { 0x1.68BC73B7C4252p-0361,  0x0.8CC8C31ACBB988p-0413 },   // 3.0e-0109
        { 0x1.E0FB44F50586Ep-0361,  0x0.110BAECE64F768p-0413 },   // 4.0e-0109
        { 0x1.2C9D0B1923744p-0360,  0x0.CAA74D40FF1AA0p-0412 },   // 5.0e-0109
        { 0x1.68BC73B7C4252p-0360,  0x0.8CC8C31ACBB988p-0412 },   // 6.0e-0109
        { 0x1.A4DBDC5664D60p-0360,  0x0.4EEA38F4985878p-0412 },   // 7.0e-0109
        { 0x1.E0FB44F50586Ep-0360,  0x0.110BAECE64F768p-0412 },   // 8.0e-0109
        { 0x1.0E8D56C9D31BDp-0359,  0x0.E996925418CB28p-0411 },   // 9.0e-0109
        { 0x1.2C9D0B1923744p-0359,  0x0.CAA74D40FF1AA0p-0411 },   // 1.0e-0108
        { 0x1.2C9D0B1923744p-0358,  0x0.CAA74D40FF1AA0p-0410 },   // 2.0e-0108
        { 0x1.C2EB90A5B52E7p-0358,  0x0.2FFAF3E17EA7F0p-0410 },   // 3.0e-0108
        { 0x1.2C9D0B1923744p-0357,  0x0.CAA74D40FF1AA0p-0409 },   // 4.0e-0108
        { 0x1.77C44DDF6C515p-0357,  0x0.FD5120913EE148p-0409 },   // 5.0e-0108
        { 0x1.C2EB90A5B52E7p-0357,  0x0.2FFAF3E17EA7F0p-0409 },   // 6.0e-0108
        { 0x1.070969B5FF05Cp-0356,  0x0.31526398DF3748p-0408 },   // 7.0e-0108
        { 0x1.2C9D0B1923744p-0356,  0x0.CAA74D40FF1AA0p-0408 },   // 8.0e-0108
        { 0x1.5230AC7C47E2Dp-0356,  0x0.63FC36E91EFDF0p-0408 },   // 9.0e-0108
        { 0x1.77C44DDF6C515p-0356,  0x0.FD5120913EE148p-0408 },   // 1.0e-0107
        { 0x1.77C44DDF6C515p-0355,  0x0.FD5120913EE148p-0407 },   // 2.0e-0107
        { 0x1.19D33A67913D0p-0354,  0x0.7DFCD86CEF28F8p-0406 },   // 3.0e-0107
        { 0x1.77C44DDF6C515p-0354,  0x0.FD5120913EE148p-0406 },   // 4.0e-0107
        { 0x1.D5B561574765Bp-0354,  0x0.7CA568B58E9998p-0406 },   // 5.0e-0107
        { 0x1.19D33A67913D0p-0353,  0x0.7DFCD86CEF28F8p-0405 },   // 6.0e-0107
        { 0x1.48CBC4237EC73p-0353,  0x0.3DA6FC7F170520p-0405 },   // 7.0e-0107
        { 0x1.77C44DDF6C515p-0353,  0x0.FD5120913EE148p-0405 },   // 8.0e-0107
        { 0x1.A6BCD79B59DB8p-0353,  0x0.BCFB44A366BD70p-0405 },   // 9.0e-0107
        { 0x1.D5B561574765Bp-0353,  0x0.7CA568B58E9998p-0405 },   // 1.0e-0106
        { 0x1.D5B561574765Bp-0352,  0x0.7CA568B58E9998p-0404 },   // 2.0e-0106
        { 0x1.60480901758C4p-0351,  0x0.9D7C0E882AF330p-0403 },   // 3.0e-0106
        { 0x1.D5B561574765Bp-0351,  0x0.7CA568B58E9998p-0403 },   // 4.0e-0106
        { 0x1.25915CD68C9F9p-0350,  0x0.2DE76171792000p-0402 },   // 5.0e-0106
        { 0x1.60480901758C4p-0350,  0x0.9D7C0E882AF330p-0402 },   // 6.0e-0106
        { 0x1.9AFEB52C5E790p-0350,  0x0.0D10BB9EDCC668p-0402 },   // 7.0e-0106
        { 0x1.D5B561574765Bp-0350,  0x0.7CA568B58E9998p-0402 },   // 8.0e-0106
        { 0x1.083606C118293p-0349,  0x0.761D0AE6203668p-0401 },   // 9.0e-0106
        { 0x1.25915CD68C9F9p-0349,  0x0.2DE76171792000p-0401 },   // 1.0e-0105
        { 0x1.25915CD68C9F9p-0348,  0x0.2DE76171792000p-0400 },   // 2.0e-0105
        { 0x1.B85A0B41D2EF5p-0348,  0x0.C4DB122A35B000p-0400 },   // 3.0e-0105
        { 0x1.25915CD68C9F9p-0347,  0x0.2DE76171792000p-0399 },   // 4.0e-0105
        { 0x1.6EF5B40C2FC77p-0347,  0x0.796139CDD76800p-0399 },   // 5.0e-0105
        { 0x1.B85A0B41D2EF5p-0347,  0x0.C4DB122A35B000p-0399 },   // 6.0e-0105
        { 0x1.00DF313BBB0BAp-0346,  0x0.082A754349FC00p-0398 },   // 7.0e-0105
        { 0x1.25915CD68C9F9p-0346,  0x0.2DE76171792000p-0398 },   // 8.0e-0105
        { 0x1.4A4388715E338p-0346,  0x0.53A44D9FA84400p-0398 },   // 9.0e-0105
        { 0x1.6EF5B40C2FC77p-0346,  0x0.796139CDD76800p-0398 },   // 1.0e-0104
        { 0x1.6EF5B40C2FC77p-0345,  0x0.796139CDD76800p-0397 },   // 2.0e-0104
        { 0x1.1338470923D59p-0344,  0x0.9B08EB5A618E00p-0396 },   // 3.0e-0104
        { 0x1.6EF5B40C2FC77p-0344,  0x0.796139CDD76800p-0396 },   // 4.0e-0104
        { 0x1.CAB3210F3BB95p-0344,  0x0.57B988414D4200p-0396 },   // 5.0e-0104
        { 0x1.1338470923D59p-0343,  0x0.9B08EB5A618E00p-0395 },   // 6.0e-0104
        { 0x1.4116FD8AA9CE8p-0343,  0x0.8A3512941C7B00p-0395 },   // 7.0e-0104
        { 0x1.6EF5B40C2FC77p-0343,  0x0.796139CDD76800p-0395 },   // 8.0e-0104
        { 0x1.9CD46A8DB5C06p-0343,  0x0.688D6107925500p-0395 },   // 9.0e-0104
        { 0x1.CAB3210F3BB95p-0343,  0x0.57B988414D4200p-0395 },   // 1.0e-0103
        { 0x1.CAB3210F3BB95p-0342,  0x0.57B988414D4200p-0394 },   // 2.0e-0103
        { 0x1.580658CB6CCB0p-0341,  0x0.01CB2630F9F180p-0393 },   // 3.0e-0103
        { 0x1.CAB3210F3BB95p-0341,  0x0.57B988414D4200p-0393 },   // 4.0e-0103
        { 0x1.1EAFF4A98553Dp-0340,  0x0.56D3F528D04940p-0392 },   // 5.0e-0103
        { 0x1.580658CB6CCB0p-0340,  0x0.01CB2630F9F180p-0392 },   // 6.0e-0103
        { 0x1.915CBCED54422p-0340,  0x0.ACC257392399C0p-0392 },   // 7.0e-0103
        { 0x1.CAB3210F3BB95p-0340,  0x0.57B988414D4200p-0392 },   // 8.0e-0103
        { 0x1.0204C29891984p-0339,  0x0.01585CA4BB7520p-0391 },   // 9.0e-0103
        { 0x1.1EAFF4A98553Dp-0339,  0x0.56D3F528D04940p-0391 },   // 1.0e-0102
        { 0x1.1EAFF4A98553Dp-0338,  0x0.56D3F528D04940p-0390 },   // 2.0e-0102
        { 0x1.AE07EEFE47FDCp-0338,  0x0.023DEFBD386DE0p-0390 },   // 3.0e-0102
        { 0x1.1EAFF4A98553Dp-0337,  0x0.56D3F528D04940p-0389 },   // 4.0e-0102
        { 0x1.665BF1D3E6A8Cp-0337,  0x0.AC88F273045B90p-0389 },   // 5.0e-0102
        { 0x1.AE07EEFE47FDCp-0337,  0x0.023DEFBD386DE0p-0389 },   // 6.0e-0102
        { 0x1.F5B3EC28A952Bp-0337,  0x0.57F2ED076C8030p-0389 },   // 7.0e-0102
        { 0x1.1EAFF4A98553Dp-0336,  0x0.56D3F528D04940p-0388 },   // 8.0e-0102
        { 0x1.4285F33EB5FE5p-0336,  0x0.01AE73CDEA5268p-0388 },   // 9.0e-0102
        { 0x1.665BF1D3E6A8Cp-0336,  0x0.AC88F273045B90p-0388 },   // 1.0e-0101
        { 0x1.665BF1D3E6A8Cp-0335,  0x0.AC88F273045B90p-0387 },   // 2.0e-0101
        { 0x1.0CC4F55EECFE9p-0334,  0x0.8166B5D64344A8p-0386 },   // 3.0e-0101
        { 0x1.665BF1D3E6A8Cp-0334,  0x0.AC88F273045B90p-0386 },   // 4.0e-0101
        { 0x1.BFF2EE48E052Fp-0334,  0x0.D7AB2F0FC57270p-0386 },   // 5.0e-0101
        { 0x1.0CC4F55EECFE9p-0333,  0x0.8166B5D64344A8p-0385 },   // 6.0e-0101
        { 0x1.3990739969D3Bp-0333,  0x0.16F7D424A3D020p-0385 },   // 7.0e-0101
        { 0x1.665BF1D3E6A8Cp-0333,  0x0.AC88F273045B90p-0385 },   // 8.0e-0101
        { 0x1.9327700E637DEp-0333,  0x0.421A10C164E700p-0385 },   // 9.0e-0101
        { 0x1.BFF2EE48E052Fp-0333,  0x0.D7AB2F0FC57270p-0385 },   // 1.0e-0100
        { 0x1.BFF2EE48E052Fp-0332,  0x0.D7AB2F0FC57270p-0384 },   // 2.0e-0100
        { 0x1.4FF632B6A83E3p-0331,  0x0.E1C0634BD415D8p-0383 },   // 3.0e-0100
        { 0x1.BFF2EE48E052Fp-0331,  0x0.D7AB2F0FC57270p-0383 },   // 4.0e-0100
        { 0x1.17F7D4ED8C33Dp-0330,  0x0.E6CAFD69DB6788p-0382 },   // 5.0e-0100
        { 0x1.4FF632B6A83E3p-0330,  0x0.E1C0634BD415D8p-0382 },   // 6.0e-0100
        { 0x1.87F4907FC4489p-0330,  0x0.DCB5C92DCCC428p-0382 },   // 7.0e-0100
        { 0x1.BFF2EE48E052Fp-0330,  0x0.D7AB2F0FC57270p-0382 },   // 8.0e-0100
        { 0x1.F7F14C11FC5D5p-0330,  0x0.D2A094F1BE20C0p-0382 },   // 9.0e-0100
        { 0x1.17F7D4ED8C33Dp-0329,  0x0.E6CAFD69DB6788p-0381 },   // 1.0e-0099
        { 0x1.17F7D4ED8C33Dp-0328,  0x0.E6CAFD69DB6788p-0380 },   // 2.0e-0099
        { 0x1.A3F3BF64524DCp-0328,  0x0.DA307C1EC91B50p-0380 },   // 3.0e-0099
        { 0x1.17F7D4ED8C33Dp-0327,  0x0.E6CAFD69DB6788p-0379 },   // 4.0e-0099
        { 0x1.5DF5CA28EF40Dp-0327,  0x0.607DBCC4524168p-0379 },   // 5.0e-0099
        { 0x1.A3F3BF64524DCp-0327,  0x0.DA307C1EC91B50p-0379 },   // 6.0e-0099
        { 0x1.E9F1B49FB55ACp-0327,  0x0.53E33B793FF530p-0379 },   // 7.0e-0099
        { 0x1.17F7D4ED8C33Dp-0326,  0x0.E6CAFD69DB6788p-0378 },   // 8.0e-0099
        { 0x1.3AF6CF8B3DBA5p-0326,  0x0.A3A45D1716D478p-0378 },   // 9.0e-0099
        { 0x1.5DF5CA28EF40Dp-0326,  0x0.607DBCC4524168p-0378 },   // 1.0e-0098
        { 0x1.5DF5CA28EF40Dp-0325,  0x0.607DBCC4524168p-0377 },   // 2.0e-0098
        { 0x1.0678579EB370Ap-0324,  0x0.085E4D933DB110p-0376 },   // 3.0e-0098
        { 0x1.5DF5CA28EF40Dp-0324,  0x0.607DBCC4524168p-0376 },   // 4.0e-0098
        { 0x1.B5733CB32B110p-0324,  0x0.B89D2BF566D1C8p-0376 },   // 5.0e-0098
        { 0x1.0678579EB370Ap-0323,  0x0.085E4D933DB110p-0375 },   // 6.0e-0098
        { 0x1.323710E3D158Bp-0323,  0x0.B46E052BC7F938p-0375 },   // 7.0e-0098
        { 0x1.5DF5CA28EF40Dp-0323,  0x0.607DBCC4524168p-0375 },   // 8.0e-0098
        { 0x1.89B4836E0D28Fp-0323,  0x0.0C8D745CDC8998p-0375 },   // 9.0e-0098
        { 0x1.B5733CB32B110p-0323,  0x0.B89D2BF566D1C8p-0375 },   // 1.0e-0097
        { 0x1.B5733CB32B110p-0322,  0x0.B89D2BF566D1C8p-0374 },   // 2.0e-0097
        { 0x1.48166D86604CCp-0321,  0x0.8A75E0F80D1D50p-0373 },   // 3.0e-0097
        { 0x1.B5733CB32B110p-0321,  0x0.B89D2BF566D1C8p-0373 },   // 4.0e-0097
        { 0x1.116805EFFAEAAp-0320,  0x0.73623B79604318p-0372 },   // 5.0e-0097
        { 0x1.48166D86604CCp-0320,  0x0.8A75E0F80D1D50p-0372 },   // 6.0e-0097
        { 0x1.7EC4D51CC5AEEp-0320,  0x0.A1898676B9F788p-0372 },   // 7.0e-0097
        { 0x1.B5733CB32B110p-0320,  0x0.B89D2BF566D1C8p-0372 },   // 8.0e-0097
        { 0x1.EC21A44990732p-0320,  0x0.CFB0D17413AC00p-0372 },   // 9.0e-0097
        { 0x1.116805EFFAEAAp-0319,  0x0.73623B79604318p-0371 },   // 1.0e-0096
        { 0x1.116805EFFAEAAp-0318,  0x0.73623B79604318p-0370 },   // 2.0e-0096
        { 0x1.9A1C08E7F85FFp-0318,  0x0.AD1359361064A8p-0370 },   // 3.0e-0096
        { 0x1.116805EFFAEAAp-0317,  0x0.73623B79604318p-0369 },   // 4.0e-0096
        { 0x1.55C2076BF9A55p-0317,  0x0.103ACA57B853E0p-0369 },   // 5.0e-0096
        { 0x1.9A1C08E7F85FFp-0317,  0x0.AD1359361064A8p-0369 },   // 6.0e-0096
        { 0x1.DE760A63F71AAp-0317,  0x0.49EBE814687570p-0369 },   // 7.0e-0096
        { 0x1.116805EFFAEAAp-0316,  0x0.73623B79604318p-0368 },   // 8.0e-0096
        { 0x1.339506ADFA47Fp-0316,  0x0.C1CE82E88C4B80p-0368 },   // 9.0e-0096
        { 0x1.55C2076BF9A55p-0316,  0x0.103ACA57B853E0p-0368 },   // 1.0e-0095
        { 0x1.55C2076BF9A55p-0315,  0x0.103ACA57B853E0p-0367 },   // 2.0e-0095
        { 0x1.00518590FB3BFp-0314,  0x0.CC2C17C1CA3EE8p-0366 },   // 3.0e-0095
        { 0x1.55C2076BF9A55p-0314,  0x0.103ACA57B853E0p-0366 },   // 4.0e-0095
        { 0x1.AB328946F80EAp-0314,  0x0.54497CEDA668D8p-0366 },   // 5.0e-0095
        { 0x1.00518590FB3BFp-0313,  0x0.CC2C17C1CA3EE8p-0365 },   // 6.0e-0095
        { 0x1.2B09C67E7A70Ap-0313,  0x0.6E33710CC14968p-0365 },   // 7.0e-0095
        { 0x1.55C2076BF9A55p-0313,  0x0.103ACA57B853E0p-0365 },   // 8.0e-0095
        { 0x1.807A485978D9Fp-0313,  0x0.B24223A2AF5E60p-0365 },   // 9.0e-0095
        { 0x1.AB328946F80EAp-0313,  0x0.54497CEDA668D8p-0365 },   // 1.0e-0094
        { 0x1.AB328946F80EAp-0312,  0x0.54497CEDA668D8p-0364 },   // 2.0e-0094
        { 0x1.4065E6F53A0AFp-0311,  0x0.BF371DB23CCEA0p-0363 },   // 3.0e-0094
        { 0x1.AB328946F80EAp-0311,  0x0.54497CEDA668D8p-0363 },   // 4.0e-0094
        { 0x1.0AFF95CC5B092p-0310,  0x0.74ADEE14880188p-0362 },   // 5.0e-0094
        { 0x1.4065E6F53A0AFp-0310,  0x0.BF371DB23CCEA0p-0362 },   // 6.0e-0094
        { 0x1.75CC381E190CDp-0310,  0x0.09C04D4FF19BC0p-0362 },   // 7.0e-0094
        { 0x1.AB328946F80EAp-0310,  0x0.54497CEDA668D8p-0362 },   // 8.0e-0094
        { 0x1.E098DA6FD7107p-0310,  0x0.9ED2AC8B5B35F8p-0362 },   // 9.0e-0094
        { 0x1.0AFF95CC5B092p-0309,  0x0.74ADEE14880188p-0361 },   // 1.0e-0093
        { 0x1.0AFF95CC5B092p-0308,  0x0.74ADEE14880188p-0360 },   // 2.0e-0093
        { 0x1.907F60B2888DBp-0308,  0x0.AF04E51ECC0250p-0360 },   // 3.0e-0093
        { 0x1.0AFF95CC5B092p-0307,  0x0.74ADEE14880188p-0359 },   // 4.0e-0093
        { 0x1.4DBF7B3F71CB7p-0307,  0x0.11D96999AA01E8p-0359 },   // 5.0e-0093
        { 0x1.907F60B2888DBp-0307,  0x0.AF04E51ECC0250p-0359 },   // 6.0e-0093
        { 0x1.D33F46259F500p-0307,  0x0.4C3060A3EE02B0p-0359 },   // 7.0e-0093
        { 0x1.0AFF95CC5B092p-0306,  0x0.74ADEE14880188p-0358 },   // 8.0e-0093
        { 0x1.2C5F8885E66A4p-0306,  0x0.C343ABD71901B8p-0358 },   // 9.0e-0093
        { 0x1.4DBF7B3F71CB7p-0306,  0x0.11D96999AA01E8p-0358 },   // 1.0e-0092
        { 0x1.4DBF7B3F71CB7p-0305,  0x0.11D96999AA01E8p-0357 },   // 2.0e-0092
        { 0x1.F49F38DF2AB12p-0305,  0x0.9AC61E667F02E0p-0357 },   // 3.0e-0092
        { 0x1.4DBF7B3F71CB7p-0304,  0x0.11D96999AA01E8p-0356 },   // 4.0e-0092
        { 0x1.A12F5A0F4E3E4p-0304,  0x0.D64FC400148268p-0356 },   // 5.0e-0092
        { 0x1.F49F38DF2AB12p-0304,  0x0.9AC61E667F02E0p-0356 },   // 6.0e-0092
        { 0x1.24078BD783920p-0303,  0x0.2F9E3C6674C1A8p-0355 },   // 7.0e-0092
        { 0x1.4DBF7B3F71CB7p-0303,  0x0.11D96999AA01E8p-0355 },   // 8.0e-0092
        { 0x1.77776AA76004Dp-0303,  0x0.F41496CCDF4228p-0355 },   // 9.0e-0092
        { 0x1.A12F5A0F4E3E4p-0303,  0x0.D64FC400148268p-0355 },   // 1.0e-0091
        { 0x1.A12F5A0F4E3E4p-0302,  0x0.D64FC400148268p-0354 },   // 2.0e-0091
        { 0x1.38E3838B7AAEBp-0301,  0x0.A0BBD3000F61C8p-0353 },   // 3.0e-0091
        { 0x1.A12F5A0F4E3E4p-0301,  0x0.D64FC400148268p-0353 },   // 4.0e-0091
        { 0x1.04BD984990E6Fp-0300,  0x0.05F1DA800CD180p-0352 },   // 5.0e-0091
        { 0x1.38E3838B7AAEBp-0300,  0x0.A0BBD3000F61C8p-0352 },   // 6.0e-0091
        { 0x1.6D096ECD64768p-0300,  0x0.3B85CB8011F218p-0352 },   // 7.0e-0091
        { 0x1.A12F5A0F4E3E4p-0300,  0x0.D64FC400148268p-0352 },   // 8.0e-0091
        { 0x1.D555455138061p-0300,  0x0.7119BC801712B0p-0352 },   // 9.0e-0091
        { 0x1.04BD984990E6Fp-0299,  0x0.05F1DA800CD180p-0351 },   // 1.0e-0090
        { 0x1.04BD984990E6Fp-0298,  0x0.05F1DA800CD180p-0350 },   // 2.0e-0090
        { 0x1.871C646E595A6p-0298,  0x0.88EAC7C0133A40p-0350 },   // 3.0e-0090
        { 0x1.04BD984990E6Fp-0297,  0x0.05F1DA800CD180p-0349 },   // 4.0e-0090
        { 0x1.45ECFE5BF520Ap-0297,  0x0.C76E51201005E0p-0349 },   // 5.0e-0090
        { 0x1.871C646E595A6p-0297,  0x0.88EAC7C0133A40p-0349 },   // 6.0e-0090
        { 0x1.C84BCA80BD942p-0297,  0x0.4A673E60166EA0p-0349 },   // 7.0e-0090
        { 0x1.04BD984990E6Fp-0296,  0x0.05F1DA800CD180p-0348 },   // 8.0e-0090
        { 0x1.25554B52C303Cp-0296,  0x0.E6B015D00E6BB0p-0348 },   // 9.0e-0090
        { 0x1.45ECFE5BF520Ap-0296,  0x0.C76E51201005E0p-0348 },   // 1.0e-0089
        { 0x1.45ECFE5BF520Ap-0295,  0x0.C76E51201005E0p-0347 },   // 2.0e-0089
        { 0x1.E8E37D89EFB10p-0295,  0x0.2B2579B01808D0p-0347 },   // 3.0e-0089
        { 0x1.45ECFE5BF520Ap-0294,  0x0.C76E51201005E0p-0346 },   // 4.0e-0089
        { 0x1.97683DF2F268Dp-0294,  0x0.7949E568140758p-0346 },   // 5.0e-0089
        { 0x1.E8E37D89EFB10p-0294,  0x0.2B2579B01808D0p-0346 },   // 6.0e-0089
        { 0x1.1D2F5E90767C9p-0293,  0x0.6E8086FC0E0520p-0345 },   // 7.0e-0089
        { 0x1.45ECFE5BF520Ap-0293,  0x0.C76E51201005E0p-0345 },   // 8.0e-0089
        { 0x1.6EAA9E2773C4Cp-0293,  0x0.205C1B44120698p-0345 },   // 9.0e-0089
        { 0x1.97683DF2F268Dp-0293,  0x0.7949E568140758p-0345 },   // 1.0e-0088
        { 0x1.97683DF2F268Dp-0292,  0x0.7949E568140758p-0344 },   // 2.0e-0088
        { 0x1.318E2E7635CEAp-0291,  0x0.1AF76C0E0F0580p-0343 },   // 3.0e-0088
        { 0x1.97683DF2F268Dp-0291,  0x0.7949E568140758p-0343 },   // 4.0e-0088
        { 0x1.FD424D6FAF030p-0291,  0x0.D79C5EC2190930p-0343 },   // 5.0e-0088
        { 0x1.318E2E7635CEAp-0290,  0x0.1AF76C0E0F0580p-0342 },   // 6.0e-0088
        { 0x1.647B3634941BBp-0290,  0x0.CA20A8BB118668p-0342 },   // 7.0e-0088
        { 0x1.97683DF2F268Dp-0290,  0x0.7949E568140758p-0342 },   // 8.0e-0088
        { 0x1.CA5545B150B5Fp-0290,  0x0.28732215168840p-0342 },   // 9.0e-0088
        { 0x1.FD424D6FAF030p-0290,  0x0.D79C5EC2190930p-0342 },   // 1.0e-0087
        { 0x1.FD424D6FAF030p-0289,  0x0.D79C5EC2190930p-0341 },   // 2.0e-0087
        { 0x1.7DF1BA13C3424p-0288,  0x0.A1B5471192C6E0p-0340 },   // 3.0e-0087
        { 0x1.FD424D6FAF030p-0288,  0x0.D79C5EC2190930p-0340 },   // 4.0e-0087
        { 0x1.3E497065CD61Ep-0287,  0x0.86C1BB394FA5B8p-0339 },   // 5.0e-0087
        { 0x1.7DF1BA13C3424p-0287,  0x0.A1B5471192C6E0p-0339 },   // 6.0e-0087
        { 0x1.BD9A03C1B922Ap-0287,  0x0.BCA8D2E9D5E808p-0339 },   // 7.0e-0087
        { 0x1.FD424D6FAF030p-0287,  0x0.D79C5EC2190930p-0339 },   // 8.0e-0087
        { 0x1.1E754B8ED271Bp-0286,  0x0.7947F54D2E1528p-0338 },   // 9.0e-0087
        { 0x1.3E497065CD61Ep-0286,  0x0.86C1BB394FA5B8p-0338 },   // 1.0e-0086
        { 0x1.3E497065CD61Ep-0285,  0x0.86C1BB394FA5B8p-0337 },   // 2.0e-0086
        { 0x1.DD6E2898B412Dp-0285,  0x0.CA2298D5F77898p-0337 },   // 3.0e-0086
        { 0x1.3E497065CD61Ep-0284,  0x0.86C1BB394FA5B8p-0336 },   // 4.0e-0086
        { 0x1.8DDBCC7F40BA6p-0284,  0x0.28722A07A38F28p-0336 },   // 5.0e-0086
        { 0x1.DD6E2898B412Dp-0284,  0x0.CA2298D5F77898p-0336 },   // 6.0e-0086
        { 0x1.1680425913B5Ap-0283,  0x0.B5E983D225B100p-0335 },   // 7.0e-0086
        { 0x1.3E497065CD61Ep-0283,  0x0.86C1BB394FA5B8p-0335 },   // 8.0e-0086
        { 0x1.66129E72870E2p-0283,  0x0.5799F2A0799A70p-0335 },   // 9.0e-0086
        { 0x1.8DDBCC7F40BA6p-0283,  0x0.28722A07A38F28p-0335 },   // 1.0e-0085
        { 0x1.8DDBCC7F40BA6p-0282,  0x0.28722A07A38F28p-0334 },   // 2.0e-0085
        { 0x1.2A64D95F708BCp-0281,  0x0.9E559F85BAAB60p-0333 },   // 3.0e-0085
        { 0x1.8DDBCC7F40BA6p-0281,  0x0.28722A07A38F28p-0333 },   // 4.0e-0085
        { 0x1.F152BF9F10E8Fp-0281,  0x0.B28EB4898C72F8p-0333 },   // 5.0e-0085
        { 0x1.2A64D95F708BCp-0280,  0x0.9E559F85BAAB60p-0332 },   // 6.0e-0085
        { 0x1.5C2052EF58A31p-0280,  0x0.6363E4C6AF1D48p-0332 },   // 7.0e-0085
        { 0x1.8DDBCC7F40BA6p-0280,  0x0.28722A07A38F28p-0332 },   // 8.0e-0085
        { 0x1.BF97460F28D1Ap-0280,  0x0.ED806F48980110p-0332 },   // 9.0e-0085
        { 0x1.F152BF9F10E8Fp-0280,  0x0.B28EB4898C72F8p-0332 },   // 1.0e-0084
        { 0x1.F152BF9F10E8Fp-0279,  0x0.B28EB4898C72F8p-0331 },   // 2.0e-0084
        { 0x1.74FE0FB74CAEBp-0278,  0x0.C5EB0767295638p-0330 },   // 3.0e-0084
        { 0x1.F152BF9F10E8Fp-0278,  0x0.B28EB4898C72F8p-0330 },   // 4.0e-0084
        { 0x1.36D3B7C36A919p-0277,  0x0.CF9930D5F7C7D8p-0329 },   // 5.0e-0084
        { 0x1.74FE0FB74CAEBp-0277,  0x0.C5EB0767295638p-0329 },   // 6.0e-0084
        { 0x1.B32867AB2ECBDp-0277,  0x0.BC3CDDF85AE498p-0329 },   // 7.0e-0084
        { 0x1.F152BF9F10E8Fp-0277,  0x0.B28EB4898C72F8p-0329 },   // 8.0e-0084
        { 0x1.17BE8BC979830p-0276,  0x0.D470458D5F00A8p-0328 },   // 9.0e-0084
        { 0x1.36D3B7C36A919p-0276,  0x0.CF9930D5F7C7D8p-0328 },   // 1.0e-0083
        { 0x1.36D3B7C36A919p-0275,  0x0.CF9930D5F7C7D8p-0327 },   // 2.0e-0083
        { 0x1.D23D93A51FDA6p-0275,  0x0.B765C940F3ABC8p-0327 },   // 3.0e-0083
        { 0x1.36D3B7C36A919p-0274,  0x0.CF9930D5F7C7D8p-0326 },   // 4.0e-0083
        { 0x1.8488A5B445360p-0274,  0x0.437F7D0B75B9D0p-0326 },   // 5.0e-0083
        { 0x1.D23D93A51FDA6p-0274,  0x0.B765C940F3ABC8p-0326 },   // 6.0e-0083
        { 0x1.0FF940CAFD3F6p-0273,  0x0.95A60ABB38CEE0p-0325 },   // 7.0e-0083
        { 0x1.36D3B7C36A919p-0273,  0x0.CF9930D5F7C7D8p-0325 },   // 8.0e-0083
        { 0x1.5DAE2EBBD7E3Dp-0273,  0x0.098C56F0B6C0D0p-0325 },   // 9.0e-0083
        { 0x1.8488A5B445360p-0273,  0x0.437F7D0B75B9D0p-0325 },   // 1.0e-0082
        { 0x1.8488A5B445360p-0272,  0x0.437F7D0B75B9D0p-0324 },   // 2.0e-0082
        { 0x1.23667C4733E88p-0271,  0x0.329F9DC8984B58p-0323 },   // 3.0e-0082
        { 0x1.8488A5B445360p-0271,  0x0.437F7D0B75B9D0p-0323 },   // 4.0e-0082
        { 0x1.E5AACF2156838p-0271,  0x0.545F5C4E532848p-0323 },   // 5.0e-0082
        { 0x1.23667C4733E88p-0270,  0x0.329F9DC8984B58p-0322 },   // 6.0e-0082
        { 0x1.53F790FDBC8F4p-0270,  0x0.3B0F8D6A070298p-0322 },   // 7.0e-0082
        { 0x1.8488A5B445360p-0270,  0x0.437F7D0B75B9D0p-0322 },   // 8.0e-0082
        { 0x1.B519BA6ACDDCCp-0270,  0x0.4BEF6CACE47108p-0322 },   // 9.0e-0082
        { 0x1.E5AACF2156838p-0270,  0x0.545F5C4E532840p-0322 },   // 1.0e-0081
        { 0x1.E5AACF2156838p-0269,  0x0.545F5C4E532840p-0321 },   // 2.0e-0081
        { 0x1.6C401B5900E2Ap-0268,  0x0.3F47853ABE5E30p-0320 },   // 3.0e-0081
        { 0x1.E5AACF2156838p-0268,  0x0.545F5C4E532840p-0320 },   // 4.0e-0081
        { 0x1.2F8AC174D6123p-0267,  0x0.34BB99B0F3F928p-0319 },   // 5.0e-0081
        { 0x1.6C401B5900E2Ap-0267,  0x0.3F47853ABE5E30p-0319 },   // 6.0e-0081
        { 0x1.A8F5753D2BB31p-0267,  0x0.49D370C488C338p-0319 },   // 7.0e-0081
        { 0x1.E5AACF2156838p-0267,  0x0.545F5C4E532840p-0319 },   // 8.0e-0081
        { 0x1.11301482C0A9Fp-0266,  0x0.AF75A3EC0EC6A8p-0318 },   // 9.0e-0081
        { 0x1.2F8AC174D6123p-0266,  0x0.34BB99B0F3F928p-0318 },   // 1.0e-0080
        { 0x1.2F8AC174D6123p-0265,  0x0.34BB99B0F3F928p-0317 },   // 2.0e-0080
        { 0x1.C750222F411B4p-0265,  0x0.CF1966896DF5C0p-0317 },   // 3.0e-0080
        { 0x1.2F8AC174D6123p-0264,  0x0.34BB99B0F3F928p-0316 },   // 4.0e-0080
        { 0x1.7B6D71D20B96Cp-0264,  0x0.01EA801D30F778p-0316 },   // 5.0e-0080
        { 0x1.C750222F411B4p-0264,  0x0.CF1966896DF5C0p-0316 },   // 6.0e-0080
        { 0x1.099969463B4FEp-0263,  0x0.CE24267AD57A00p-0315 },   // 7.0e-0080
        { 0x1.2F8AC174D6123p-0263,  0x0.34BB99B0F3F928p-0315 },   // 8.0e-0080
        { 0x1.557C19A370D47p-0263,  0x0.9B530CE7127850p-0315 },   // 9.0e-0080
        { 0x1.7B6D71D20B96Cp-0263,  0x0.01EA801D30F778p-0315 },   // 1.0e-0079
        { 0x1.7B6D71D20B96Cp-0262,  0x0.01EA801D30F778p-0314 },   // 2.0e-0079
        { 0x1.1C92155D88B11p-0261,  0x0.016FE015E4B998p-0313 },   // 3.0e-0079
        { 0x1.7B6D71D20B96Cp-0261,  0x0.01EA801D30F778p-0313 },   // 4.0e-0079
        { 0x1.DA48CE468E7C7p-0261,  0x0.026520247D3550p-0313 },   // 5.0e-0079
        { 0x1.1C92155D88B11p-0260,  0x0.016FE015E4B998p-0312 },   // 6.0e-0079
        { 0x1.4BFFC397CA23Ep-0260,  0x0.81AD30198AD888p-0312 },   // 7.0e-0079
        { 0x1.7B6D71D20B96Cp-0260,  0x0.01EA801D30F778p-0312 },   // 8.0e-0079
        { 0x1.AADB200C4D099p-0260,  0x0.8227D020D71660p-0312 },   // 9.0e-0079
        { 0x1.DA48CE468E7C7p-0260,  0x0.026520247D3550p-0312 },   // 1.0e-0078
        { 0x1.DA48CE468E7C7p-0259,  0x0.026520247D3550p-0311 },   // 2.0e-0078
        { 0x1.63B69AB4EADD5p-0258,  0x0.41CBD81B5DE800p-0310 },   // 3.0e-0078
        { 0x1.DA48CE468E7C7p-0258,  0x0.026520247D3550p-0310 },   // 4.0e-0078
        { 0x1.286D80EC190DCp-0257,  0x0.617F3416CE4150p-0309 },   // 5.0e-0078
        { 0x1.63B69AB4EADD5p-0257,  0x0.41CBD81B5DE800p-0309 },   // 6.0e-0078
        { 0x1.9EFFB47DBCACEp-0257,  0x0.22187C1FED8EA8p-0309 },   // 7.0e-0078
        { 0x1.DA48CE468E7C7p-0257,  0x0.026520247D3550p-0309 },   // 8.0e-0078
        { 0x1.0AC8F407B025Fp-0256,  0x0.F158E214866E00p-0308 },   // 9.0e-0078
        { 0x1.286D80EC190DCp-0256,  0x0.617F3416CE4150p-0308 },   // 1.0e-0077
        { 0x1.286D80EC190DCp-0255,  0x0.617F3416CE4150p-0307 },   // 2.0e-0077
        { 0x1.BCA441622594Ap-0255,  0x0.923ECE22356200p-0307 },   // 3.0e-0077
        { 0x1.286D80EC190DCp-0254,  0x0.617F3416CE4150p-0306 },   // 4.0e-0077
        { 0x1.7288E1271F513p-0254,  0x0.79DF011C81D1A8p-0306 },   // 5.0e-0077
        { 0x1.BCA441622594Ap-0254,  0x0.923ECE22356200p-0306 },   // 6.0e-0077
        { 0x1.035FD0CE95EC0p-0253,  0x0.D54F4D93F47928p-0305 },   // 7.0e-0077
        { 0x1.286D80EC190DCp-0253,  0x0.617F3416CE4150p-0305 },   // 8.0e-0077
        { 0x1.4D7B31099C2F7p-0253,  0x0.EDAF1A99A80980p-0305 },   // 9.0e-0077
        { 0x1.7288E1271F513p-0253,  0x0.79DF011C81D1A8p-0305 },   // 1.0e-0076
        { 0x1.7288E1271F513p-0252,  0x0.79DF011C81D1A8p-0304 },   // 2.0e-0076
        { 0x1.15E6A8DD577CEp-0251,  0x0.9B6740D5615D40p-0303 },   // 3.0e-0076
        { 0x1.7288E1271F513p-0251,  0x0.79DF011C81D1A8p-0303 },   // 4.0e-0076
        { 0x1.CF2B1970E7258p-0251,  0x0.5856C163A24610p-0303 },   // 5.0e-0076
        { 0x1.15E6A8DD577CEp-0250,  0x0.9B6740D5615D40p-0302 },   // 6.0e-0076
        { 0x1.4437C5023B671p-0250,  0x0.0AA320F8F19770p-0302 },   // 7.0e-0076
        { 0x1.7288E1271F513p-0250,  0x0.79DF011C81D1A8p-0302 },   // 8.0e-0076
        { 0x1.A0D9FD4C033B5p-0250,  0x0.E91AE140120BE0p-0302 },   // 9.0e-0076
        { 0x1.CF2B1970E7258p-0250,  0x0.5856C163A24610p-0302 },   // 1.0e-0075
        { 0x1.CF2B1970E7258p-0249,  0x0.5856C163A24610p-0301 },   // 2.0e-0075
        { 0x1.5B605314AD5C2p-0248,  0x0.4241110AB9B490p-0300 },   // 3.0e-0075
        { 0x1.CF2B1970E7258p-0248,  0x0.5856C163A24610p-0300 },   // 4.0e-0075
        { 0x1.217AEFE690777p-0247,  0x0.373638DE456BC8p-0299 },   // 5.0e-0075
        { 0x1.5B605314AD5C2p-0247,  0x0.4241110AB9B490p-0299 },   // 6.0e-0075
        { 0x1.9545B642CA40Dp-0247,  0x0.4D4BE9372DFD50p-0299 },   // 7.0e-0075
        { 0x1.CF2B1970E7258p-0247,  0x0.5856C163A24610p-0299 },   // 8.0e-0075
        { 0x1.04883E4F82051p-0246,  0x0.B1B0CCC80B4768p-0298 },   // 9.0e-0075
        { 0x1.217AEFE690777p-0246,  0x0.373638DE456BC8p-0298 },   // 1.0e-0074
        { 0x1.217AEFE690777p-0245,  0x0.373638DE456BC8p-0297 },   // 2.0e-0074
        { 0x1.B23867D9D8B32p-0245,  0x0.D2D1554D6821B0p-0297 },   // 3.0e-0074
        { 0x1.217AEFE690777p-0244,  0x0.373638DE456BC8p-0296 },   // 4.0e-0074
        { 0x1.69D9ABE034955p-0244,  0x0.0503C715D6C6C0p-0296 },   // 5.0e-0074
        { 0x1.B23867D9D8B32p-0244,  0x0.D2D1554D6821B0p-0296 },   // 6.0e-0074
        { 0x1.FA9723D37CD10p-0244,  0x0.A09EE384F97CA8p-0296 },   // 7.0e-0074
        { 0x1.217AEFE690777p-0243,  0x0.373638DE456BC8p-0295 },   // 8.0e-0074
        { 0x1.45AA4DE362866p-0243,  0x0.1E1CFFFA0E1940p-0295 },   // 9.0e-0074
        { 0x1.69D9ABE034955p-0243,  0x0.0503C715D6C6C0p-0295 },   // 1.0e-0073
        { 0x1.69D9ABE034955p-0242,  0x0.0503C715D6C6C0p-0294 },   // 2.0e-0073
        { 0x1.0F6340E8276FFp-0241,  0x0.C3C2D550611510p-0293 },   // 3.0e-0073
        { 0x1.69D9ABE034955p-0241,  0x0.0503C715D6C6C0p-0293 },   // 4.0e-0073
        { 0x1.C45016D841BAAp-0241,  0x0.4644B8DB4C7870p-0293 },   // 5.0e-0073
        { 0x1.0F6340E8276FFp-0240,  0x0.C3C2D550611510p-0292 },   // 6.0e-0073
        { 0x1.3C9E76642E02Ap-0240,  0x0.64634E331BEDE8p-0292 },   // 7.0e-0073
        { 0x1.69D9ABE034955p-0240,  0x0.0503C715D6C6C0p-0292 },   // 8.0e-0073
        { 0x1.9714E15C3B27Fp-0240,  0x0.A5A43FF8919F98p-0292 },   // 9.0e-0073
        { 0x1.C45016D841BAAp-0240,  0x0.4644B8DB4C7870p-0292 },   // 1.0e-0072
        { 0x1.C45016D841BAAp-0239,  0x0.4644B8DB4C7870p-0291 },   // 2.0e-0072
        { 0x1.533C1122314BFp-0238,  0x0.B4B38AA4795A50p-0290 },   // 3.0e-0072
        { 0x1.C45016D841BAAp-0238,  0x0.4644B8DB4C7870p-0290 },   // 4.0e-0072
        { 0x1.1AB20E472914Ap-0237,  0x0.6BEAF3890FCB40p-0289 },   // 5.0e-0072
        { 0x1.533C1122314BFp-0237,  0x0.B4B38AA4795A50p-0289 },   // 6.0e-0072
        { 0x1.8BC613FD39834p-0237,  0x0.FD7C21BFE2E960p-0289 },   // 7.0e-0072
        { 0x1.C45016D841BAAp-0237,  0x0.4644B8DB4C7870p-0289 },   // 8.0e-0072
        { 0x1.FCDA19B349F1Fp-0237,  0x0.8F0D4FF6B60780p-0289 },   // 9.0e-0072
        { 0x1.1AB20E472914Ap-0236,  0x0.6BEAF3890FCB40p-0288 },   // 1.0e-0071
        { 0x1.1AB20E472914Ap-0235,  0x0.6BEAF3890FCB40p-0287 },   // 2.0e-0071
        { 0x1.A80B156ABD9EFp-0235,  0x0.A1E06D4D97B0E8p-0287 },   // 3.0e-0071
        { 0x1.1AB20E472914Ap-0234,  0x0.6BEAF3890FCB40p-0286 },   // 4.0e-0071
        { 0x1.615E91D8F359Dp-0234,  0x0.06E5B06B53BE18p-0286 },   // 5.0e-0071
        { 0x1.A80B156ABD9EFp-0234,  0x0.A1E06D4D97B0E8p-0286 },   // 6.0e-0071
        { 0x1.EEB798FC87E42p-0234,  0x0.3CDB2A2FDBA3B8p-0286 },   // 7.0e-0071
        { 0x1.1AB20E472914Ap-0233,  0x0.6BEAF3890FCB40p-0285 },   // 8.0e-0071
        { 0x1.3E0850100E373p-0233,  0x0.B96851FA31C4A8p-0285 },   // 9.0e-0071
        { 0x1.615E91D8F359Dp-0233,  0x0.06E5B06B53BE18p-0285 },   // 1.0e-0070
        { 0x1.615E91D8F359Dp-0232,  0x0.06E5B06B53BE18p-0284 },   // 2.0e-0070
        { 0x1.0906ED62B6835p-0231,  0x0.C52C44507ECE90p-0283 },   // 3.0e-0070
        { 0x1.615E91D8F359Dp-0231,  0x0.06E5B06B53BE18p-0283 },   // 4.0e-0070
        { 0x1.B9B6364F30304p-0231,  0x0.489F1C8628AD98p-0283 },   // 5.0e-0070
        { 0x1.0906ED62B6835p-0230,  0x0.C52C44507ECE90p-0282 },   // 6.0e-0070
        { 0x1.3532BF9DD4EE9p-0230,  0x0.6608FA5DE94650p-0282 },   // 7.0e-0070
        { 0x1.615E91D8F359Dp-0230,  0x0.06E5B06B53BE18p-0282 },   // 8.0e-0070
        { 0x1.8D8A641411C50p-0230,  0x0.A7C26678BE35D8p-0282 },   // 9.0e-0070
        { 0x1.B9B6364F30304p-0230,  0x0.489F1C8628AD98p-0282 },   // 1.0e-0069
        { 0x1.B9B6364F30304p-0229,  0x0.489F1C8628AD98p-0281 },   // 2.0e-0069
        { 0x1.4B48A8BB64243p-0228,  0x0.367755649E8230p-0280 },   // 3.0e-0069
        { 0x1.B9B6364F30304p-0228,  0x0.489F1C8628AD98p-0280 },   // 4.0e-0069
        { 0x1.1411E1F17E1E2p-0227,  0x0.AD6371D3D96C80p-0279 },   // 5.0e-0069
        { 0x1.4B48A8BB64243p-0227,  0x0.367755649E8230p-0279 },   // 6.0e-0069
        { 0x1.827F6F854A2A3p-0227,  0x0.BF8B38F56397E8p-0279 },   // 7.0e-0069
        { 0x1.B9B6364F30304p-0227,  0x0.489F1C8628AD98p-0279 },   // 8.0e-0069
        { 0x1.F0ECFD1916364p-0227,  0x0.D1B30016EDC350p-0279 },   // 9.0e-0069
        { 0x1.1411E1F17E1E2p-0226,  0x0.AD6371D3D96C80p-0278 },   // 1.0e-0068
        { 0x1.1411E1F17E1E2p-0225,  0x0.AD6371D3D96C80p-0277 },   // 2.0e-0068
        { 0x1.9E1AD2EA3D2D4p-0225,  0x0.04152ABDC622C0p-0277 },   // 3.0e-0068
        { 0x1.1411E1F17E1E2p-0224,  0x0.AD6371D3D96C80p-0276 },   // 4.0e-0068
        { 0x1.59165A6DDDA5Bp-0224,  0x0.58BC4E48CFC7A0p-0276 },   // 5.0e-0068
        { 0x1.9E1AD2EA3D2D4p-0224,  0x0.04152ABDC622C0p-0276 },   // 6.0e-0068
        { 0x1.E31F4B669CB4Cp-0224,  0x0.AF6E0732BC7DE0p-0276 },   // 7.0e-0068
        { 0x1.1411E1F17E1E2p-0223,  0x0.AD6371D3D96C80p-0275 },   // 8.0e-0068
        { 0x1.36941E2FADE1Fp-0223,  0x0.030FE00E549A10p-0275 },   // 9.0e-0068
        { 0x1.59165A6DDDA5Bp-0223,  0x0.58BC4E48CFC7A0p-0275 },   // 1.0e-0067
        { 0x1.59165A6DDDA5Bp-0222,  0x0.58BC4E48CFC7A0p-0274 },   // 2.0e-0067
        { 0x1.02D0C3D2663C4p-0221,  0x0.828D3AB69BD5B8p-0273 },   // 3.0e-0067
        { 0x1.59165A6DDDA5Bp-0221,  0x0.58BC4E48CFC7A0p-0273 },   // 4.0e-0067
        { 0x1.AF5BF109550F2p-0221,  0x0.2EEB61DB03B988p-0273 },   // 5.0e-0067
        { 0x1.02D0C3D2663C4p-0220,  0x0.828D3AB69BD5B8p-0272 },   // 6.0e-0067
        { 0x1.2DF38F2021F0Fp-0220,  0x0.EDA4C47FB5CEA8p-0272 },   // 7.0e-0067
        { 0x1.59165A6DDDA5Bp-0220,  0x0.58BC4E48CFC7A0p-0272 },   // 8.0e-0067
        { 0x1.843925BB995A6p-0220,  0x0.C3D3D811E9C098p-0272 },   // 9.0e-0067
        { 0x1.AF5BF109550F2p-0220,  0x0.2EEB61DB03B988p-0272 },   // 1.0e-0066
        { 0x1.AF5BF109550F2p-0219,  0x0.2EEB61DB03B988p-0271 },   // 2.0e-0066
        { 0x1.4384F4C6FFCB5p-0218,  0x0.A330896442CB28p-0270 },   // 3.0e-0066
        { 0x1.AF5BF109550F2p-0218,  0x0.2EEB61DB03B988p-0270 },   // 4.0e-0066
        { 0x1.0D9976A5D5297p-0217,  0x0.5D531D28E253F8p-0269 },   // 5.0e-0066
        { 0x1.4384F4C6FFCB5p-0217,  0x0.A330896442CB28p-0269 },   // 6.0e-0066
        { 0x1.797072E82A6D3p-0217,  0x0.E90DF59FA34258p-0269 },   // 7.0e-0066
        { 0x1.AF5BF109550F2p-0217,  0x0.2EEB61DB03B988p-0269 },   // 8.0e-0066
        { 0x1.E5476F2A7FB10p-0217,  0x0.74C8CE166430B8p-0269 },   // 9.0e-0066
        { 0x1.0D9976A5D5297p-0216,  0x0.5D531D28E253F8p-0268 },   // 1.0e-0065
        { 0x1.0D9976A5D5297p-0215,  0x0.5D531D28E253F8p-0267 },   // 2.0e-0065
        { 0x1.946631F8BFBE3p-0215,  0x0.0BFCABBD537DF0p-0267 },   // 3.0e-0065
        { 0x1.0D9976A5D5297p-0214,  0x0.5D531D28E253F8p-0266 },   // 4.0e-0065
        { 0x1.50FFD44F4A73Dp-0214,  0x0.34A7E4731AE8F0p-0266 },   // 5.0e-0065
        { 0x1.946631F8BFBE3p-0214,  0x0.0BFCABBD537DF0p-0266 },   // 6.0e-0065
        { 0x1.D7CC8FA235088p-0214,  0x0.E35173078C12F0p-0266 },   // 7.0e-0065
        { 0x1.0D9976A5D5297p-0213,  0x0.5D531D28E253F8p-0265 },   // 8.0e-0065
        { 0x1.2F4CA57A8FCEAp-0213,  0x0.48FD80CDFE9E70p-0265 },   // 9.0e-0065
        { 0x1.50FFD44F4A73Dp-0213,  0x0.34A7E4731AE8F0p-0265 },   // 1.0e-0064
        { 0x1.50FFD44F4A73Dp-0212,  0x0.34A7E4731AE8F0p-0264 },   // 2.0e-0064
        { 0x1.F97FBE76EFADBp-0212,  0x0.CEFBD6ACA85D70p-0264 },   // 3.0e-0064
        { 0x1.50FFD44F4A73Dp-0211,  0x0.34A7E4731AE8F0p-0263 },   // 4.0e-0064
        { 0x1.A53FC9631D10Cp-0211,  0x0.81D1DD8FE1A330p-0263 },   // 5.0e-0064
        { 0x1.F97FBE76EFADBp-0211,  0x0.CEFBD6ACA85D70p-0263 },   // 6.0e-0064
        { 0x1.26DFD9C561255p-0210,  0x0.8E12E7E4B78BD0p-0262 },   // 7.0e-0064
        { 0x1.50FFD44F4A73Dp-0210,  0x0.34A7E4731AE8F0p-0262 },   // 8.0e-0064
        { 0x1.7B1FCED933C24p-0210,  0x0.DB3CE1017E4610p-0262 },   // 9.0e-0064
        { 0x1.A53FC9631D10Cp-0210,  0x0.81D1DD8FE1A330p-0262 },   // 1.0e-0063
        { 0x1.A53FC9631D10Cp-0209,  0x0.81D1DD8FE1A330p-0261 },   // 2.0e-0063
        { 0x1.3BEFD70A55CC9p-0208,  0x0.615D662BE93A60p-0260 },   // 3.0e-0063
        { 0x1.A53FC9631D10Cp-0208,  0x0.81D1DD8FE1A330p-0260 },   // 4.0e-0063
        { 0x1.0747DDDDF22A7p-0207,  0x0.D1232A79ED0600p-0259 },   // 5.0e-0063
        { 0x1.3BEFD70A55CC9p-0207,  0x0.615D662BE93A60p-0259 },   // 6.0e-0063
        { 0x1.7097D036B96EAp-0207,  0x0.F197A1DDE56EC8p-0259 },   // 7.0e-0063
        { 0x1.A53FC9631D10Cp-0207,  0x0.81D1DD8FE1A330p-0259 },   // 8.0e-0063
        { 0x1.D9E7C28F80B2Ep-0207,  0x0.120C1941DDD798p-0259 },   // 9.0e-0063
        { 0x1.0747DDDDF22A7p-0206,  0x0.D1232A79ED0600p-0258 },   // 1.0e-0062
        { 0x1.0747DDDDF22A7p-0205,  0x0.D1232A79ED0600p-0257 },   // 2.0e-0062
        { 0x1.8AEBCCCCEB3FBp-0205,  0x0.B9B4BFB6E38900p-0257 },   // 3.0e-0062
        { 0x1.0747DDDDF22A7p-0204,  0x0.D1232A79ED0600p-0256 },   // 4.0e-0062
        { 0x1.4919D5556EB51p-0204,  0x0.C56BF518684780p-0256 },   // 5.0e-0062
        { 0x1.8AEBCCCCEB3FBp-0204,  0x0.B9B4BFB6E38900p-0256 },   // 6.0e-0062
        { 0x1.CCBDC44467CA5p-0204,  0x0.ADFD8A555ECA80p-0256 },   // 7.0e-0062
        { 0x1.0747DDDDF22A7p-0203,  0x0.D1232A79ED0600p-0255 },   // 8.0e-0062
        { 0x1.2830D999B06FCp-0203,  0x0.CB478FC92AA6C0p-0255 },   // 9.0e-0062
        { 0x1.4919D5556EB51p-0203,  0x0.C56BF518684780p-0255 },   // 1.0e-0061
        { 0x1.4919D5556EB51p-0202,  0x0.C56BF518684780p-0254 },   // 2.0e-0061
        { 0x1.EDA6C000260FAp-0202,  0x0.A821EFA49C6B40p-0254 },   // 3.0e-0061
        { 0x1.4919D5556EB51p-0201,  0x0.C56BF518684780p-0253 },   // 4.0e-0061
        { 0x1.9B604AAACA626p-0201,  0x0.36C6F25E825960p-0253 },   // 5.0e-0061
        { 0x1.EDA6C000260FAp-0201,  0x0.A821EFA49C6B40p-0253 },   // 6.0e-0061
        { 0x1.1FF69AAAC0DE7p-0200,  0x0.8CBE76755B3E90p-0252 },   // 7.0e-0061
        { 0x1.4919D5556EB51p-0200,  0x0.C56BF518684780p-0252 },   // 8.0e-0061
        { 0x1.723D10001C8BBp-0200,  0x0.FE1973BB755070p-0252 },   // 9.0e-0061
        { 0x1.9B604AAACA626p-0200,  0x0.36C6F25E825960p-0252 },   // 1.0e-0060
        { 0x1.9B604AAACA626p-0199,  0x0.36C6F25E825960p-0251 },   // 2.0e-0060
        { 0x1.3488380017C9Cp-0198,  0x0.A91535C6E1C308p-0250 },   // 3.0e-0060
        { 0x1.9B604AAACA626p-0198,  0x0.36C6F25E825960p-0250 },   // 4.0e-0060
        { 0x1.011C2EAABE7D7p-0197,  0x0.E23C577B1177D8p-0249 },   // 5.0e-0060
        { 0x1.3488380017C9Cp-0197,  0x0.A91535C6E1C308p-0249 },   // 6.0e-0060
        { 0x1.67F4415571161p-0197,  0x0.6FEE1412B20E30p-0249 },   // 7.0e-0060
        { 0x1.9B604AAACA626p-0197,  0x0.36C6F25E825960p-0249 },   // 8.0e-0060
        { 0x1.CECC540023AEAp-0197,  0x0.FD9FD0AA52A488p-0249 },   // 9.0e-0060
        { 0x1.011C2EAABE7D7p-0196,  0x0.E23C577B1177D8p-0248 },   // 1.0e-0059
        { 0x1.011C2EAABE7D7p-0195,  0x0.E23C577B1177D8p-0247 },   // 2.0e-0059
        { 0x1.81AA46001DBC3p-0195,  0x0.D35A83389A33C8p-0247 },   // 3.0e-0059
        { 0x1.011C2EAABE7D7p-0194,  0x0.E23C577B1177D8p-0246 },   // 4.0e-0059
        { 0x1.41633A556E1CDp-0194,  0x0.DACB6D59D5D5D0p-0246 },   // 5.0e-0059
        { 0x1.81AA46001DBC3p-0194,  0x0.D35A83389A33C8p-0246 },   // 6.0e-0059
        { 0x1.C1F151AACD5B9p-0194,  0x0.CBE999175E91C0p-0246 },   // 7.0e-0059
        { 0x1.011C2EAABE7D7p-0193,  0x0.E23C577B1177D8p-0245 },   // 8.0e-0059
        { 0x1.213FB480164D2p-0193,  0x0.DE83E26A73A6D8p-0245 },   // 9.0e-0059
        { 0x1.41633A556E1CDp-0193,  0x0.DACB6D59D5D5D0p-0245 },   // 1.0e-0058
        { 0x1.41633A556E1CDp-0192,  0x0.DACB6D59D5D5D0p-0244 },   // 2.0e-0058
        { 0x1.E214D780252B4p-0192,  0x0.C8312406C0C0B8p-0244 },   // 3.0e-0058
        { 0x1.41633A556E1CDp-0191,  0x0.DACB6D59D5D5D0p-0243 },   // 4.0e-0058
        { 0x1.91BC08EAC9A41p-0191,  0x0.517E48B04B4B48p-0243 },   // 5.0e-0058
        { 0x1.E214D780252B4p-0191,  0x0.C8312406C0C0B8p-0243 },   // 6.0e-0058
        { 0x1.1936D30AC0594p-0190,  0x0.1F71FFAE9B1B18p-0242 },   // 7.0e-0058
        { 0x1.41633A556E1CDp-0190,  0x0.DACB6D59D5D5D0p-0242 },   // 8.0e-0058
        { 0x1.698FA1A01BE07p-0190,  0x0.9624DB05109088p-0242 },   // 9.0e-0058
        { 0x1.91BC08EAC9A41p-0190,  0x0.517E48B04B4B48p-0242 },   // 1.0e-0057
        { 0x1.91BC08EAC9A41p-0189,  0x0.517E48B04B4B48p-0241 },   // 2.0e-0057
        { 0x1.2D4D06B0173B0p-0188,  0x0.FD1EB684387870p-0240 },   // 3.0e-0057
        { 0x1.91BC08EAC9A41p-0188,  0x0.517E48B04B4B48p-0240 },   // 4.0e-0057
        { 0x1.F62B0B257C0D1p-0188,  0x0.A5DDDADC5E1E18p-0240 },   // 5.0e-0057
        { 0x1.2D4D06B0173B0p-0187,  0x0.FD1EB684387870p-0239 },   // 6.0e-0057
        { 0x1.5F8487CD706F9p-0187,  0x0.274E7F9A41E1D8p-0239 },   // 7.0e-0057
        { 0x1.91BC08EAC9A41p-0187,  0x0.517E48B04B4B48p-0239 },   // 8.0e-0057
        { 0x1.C3F38A0822D89p-0187,  0x0.7BAE11C654B4B0p-0239 },   // 9.0e-0057
        { 0x1.F62B0B257C0D1p-0187,  0x0.A5DDDADC5E1E18p-0239 },   // 1.0e-0056
        { 0x1.F62B0B257C0D1p-0186,  0x0.A5DDDADC5E1E18p-0238 },   // 2.0e-0056
        { 0x1.78A0485C1D09Dp-0185,  0x0.3C666425469690p-0237 },   // 3.0e-0056
        { 0x1.F62B0B257C0D1p-0185,  0x0.A5DDDADC5E1E18p-0237 },   // 4.0e-0056
        { 0x1.39DAE6F76D883p-0184,  0x0.07AAA8C9BAD2D0p-0236 },   // 5.0e-0056
        { 0x1.78A0485C1D09Dp-0184,  0x0.3C666425469690p-0236 },   // 6.0e-0056
        { 0x1.B765A9C0CC8B7p-0184,  0x0.71221F80D25A50p-0236 },   // 7.0e-0056
        { 0x1.F62B0B257C0D1p-0184,  0x0.A5DDDADC5E1E18p-0236 },   // 8.0e-0056
        { 0x1.1A78364515C75p-0183,  0x0.ED4CCB1BF4F0E8p-0235 },   // 9.0e-0056
        { 0x1.39DAE6F76D883p-0183,  0x0.07AAA8C9BAD2D0p-0235 },   // 1.0e-0055
        { 0x1.39DAE6F76D883p-0182,  0x0.07AAA8C9BAD2D0p-0234 },   // 2.0e-0055
        { 0x1.D6C85A73244C4p-0182,  0x0.8B7FFD2E983C38p-0234 },   // 3.0e-0055
        { 0x1.39DAE6F76D883p-0181,  0x0.07AAA8C9BAD2D0p-0233 },   // 4.0e-0055
        { 0x1.8851A0B548EA3p-0181,  0x0.C99552FC298780p-0233 },   // 5.0e-0055
        { 0x1.D6C85A73244C4p-0181,  0x0.8B7FFD2E983C38p-0233 },   // 6.0e-0055
        { 0x1.129F8A187FD72p-0180,  0x0.A6B553B0837870p-0232 },   // 7.0e-0055
        { 0x1.39DAE6F76D883p-0180,  0x0.07AAA8C9BAD2D0p-0232 },   // 8.0e-0055
        { 0x1.611643D65B393p-0180,  0x0.689FFDE2F22D28p-0232 },   // 9.0e-0055
        { 0x1.8851A0B548EA3p-0180,  0x0.C99552FC298780p-0232 },   // 1.0e-0054
        { 0x1.8851A0B548EA3p-0179,  0x0.C99552FC298780p-0231 },   // 2.0e-0054
        { 0x1.263D3887F6AFAp-0178,  0x0.D72FFE3D1F25A0p-0230 },   // 3.0e-0054
        { 0x1.8851A0B548EA3p-0178,  0x0.C99552FC298780p-0230 },   // 4.0e-0054
        { 0x1.EA6608E29B24Cp-0178,  0x0.BBFAA7BB33E960p-0230 },   // 5.0e-0054
        { 0x1.263D3887F6AFAp-0177,  0x0.D72FFE3D1F25A0p-0229 },   // 6.0e-0054
        { 0x1.57476C9E9FCCFp-0177,  0x0.5062A89CA45690p-0229 },   // 7.0e-0054
        { 0x1.8851A0B548EA3p-0177,  0x0.C99552FC298780p-0229 },   // 8.0e-0054
        { 0x1.B95BD4CBF2078p-0177,  0x0.42C7FD5BAEB870p-0229 },   // 9.0e-0054
        { 0x1.EA6608E29B24Cp-0177,  0x0.BBFAA7BB33E960p-0229 },   // 1.0e-0053
        { 0x1.EA6608E29B24Cp-0176,  0x0.BBFAA7BB33E960p-0228 },   // 2.0e-0053
        { 0x1.6FCC86A9F45B9p-0175,  0x0.8CFBFDCC66EF08p-0227 },   // 3.0e-0053
        { 0x1.EA6608E29B24Cp-0175,  0x0.BBFAA7BB33E960p-0227 },   // 4.0e-0053
        { 0x1.327FC58DA0F6Fp-0174,  0x0.F57CA8D50071D8p-0226 },   // 5.0e-0053
        { 0x1.6FCC86A9F45B9p-0174,  0x0.8CFBFDCC66EF08p-0226 },   // 6.0e-0053
        { 0x1.AD1947C647C03p-0174,  0x0.247B52C3CD6C38p-0226 },   // 7.0e-0053
        { 0x1.EA6608E29B24Cp-0174,  0x0.BBFAA7BB33E960p-0226 },   // 8.0e-0053
        { 0x1.13D964FF7744Bp-0173,  0x0.29BCFE594D3348p-0225 },   // 9.0e-0053
        { 0x1.327FC58DA0F6Fp-0173,  0x0.F57CA8D50071D8p-0225 },   // 1.0e-0052
        { 0x1.327FC58DA0F6Fp-0172,  0x0.F57CA8D50071D8p-0224 },   // 2.0e-0052
        { 0x1.CBBFA85471727p-0172,  0x0.F03AFD3F80AAC8p-0224 },   // 3.0e-0052
        { 0x1.327FC58DA0F6Fp-0171,  0x0.F57CA8D50071D8p-0223 },   // 4.0e-0052
        { 0x1.7F1FB6F10934Bp-0171,  0x0.F2DBD30A408E50p-0223 },   // 5.0e-0052
        { 0x1.CBBFA85471727p-0171,  0x0.F03AFD3F80AAC8p-0223 },   // 6.0e-0052
        { 0x1.0C2FCCDBECD81p-0170,  0x0.F6CD13BA6063A0p-0222 },   // 7.0e-0052
        { 0x1.327FC58DA0F6Fp-0170,  0x0.F57CA8D50071D8p-0222 },   // 8.0e-0052
        { 0x1.58CFBE3F5515Dp-0170,  0x0.F42C3DEFA08018p-0222 },   // 9.0e-0052
        { 0x1.7F1FB6F10934Bp-0170,  0x0.F2DBD30A408E50p-0222 },   // 1.0e-0051
        { 0x1.7F1FB6F10934Bp-0169,  0x0.F2DBD30A408E50p-0221 },   // 2.0e-0051
        { 0x1.1F57C934C6E78p-0168,  0x0.F624DE47B06AC0p-0220 },   // 3.0e-0051
        { 0x1.7F1FB6F10934Bp-0168,  0x0.F2DBD30A408E50p-0220 },   // 4.0e-0051
        { 0x1.DEE7A4AD4B81Ep-0168,  0x0.EF92C7CCD0B1E8p-0220 },   // 5.0e-0051
        { 0x1.1F57C934C6E78p-0167,  0x0.F624DE47B06AC0p-0219 },   // 6.0e-0051
        { 0x1.4F3BC012E80E2p-0167,  0x0.748058A8F87C88p-0219 },   // 7.0e-0051
        { 0x1.7F1FB6F10934Bp-0167,  0x0.F2DBD30A408E50p-0219 },   // 8.0e-0051
        { 0x1.AF03ADCF2A5B5p-0167,  0x0.71374D6B88A020p-0219 },   // 9.0e-0051
        { 0x1.DEE7A4AD4B81Ep-0167,  0x0.EF92C7CCD0B1E8p-0219 },   // 1.0e-0050
        { 0x1.DEE7A4AD4B81Ep-0166,  0x0.EF92C7CCD0B1E8p-0218 },   // 2.0e-0050
        { 0x1.672DBB81F8A17p-0165,  0x0.33AE15D99C8570p-0217 },   // 3.0e-0050
        { 0x1.DEE7A4AD4B81Ep-0165,  0x0.EF92C7CCD0B1E8p-0217 },   // 4.0e-0050
        { 0x1.2B50C6EC4F313p-0164,  0x0.55BBBCE0026F30p-0216 },   // 5.0e-0050
        { 0x1.672DBB81F8A17p-0164,  0x0.33AE15D99C8570p-0216 },   // 6.0e-0050
        { 0x1.A30AB017A211Bp-0164,  0x0.11A06ED3369BA8p-0216 },   // 7.0e-0050
        { 0x1.DEE7A4AD4B81Ep-0164,  0x0.EF92C7CCD0B1E8p-0216 },   // 8.0e-0050
        { 0x1.0D624CA17A791p-0163,  0x0.66C29063356410p-0215 },   // 9.0e-0050
        { 0x1.2B50C6EC4F313p-0163,  0x0.55BBBCE0026F30p-0215 },   // 1.0e-0049
        { 0x1.2B50C6EC4F313p-0162,  0x0.55BBBCE0026F30p-0214 },   // 2.0e-0049
        { 0x1.C0F92A6276C9Dp-0162,  0x0.00999B5003A6C8p-0214 },   // 3.0e-0049
        { 0x1.2B50C6EC4F313p-0161,  0x0.55BBBCE0026F30p-0213 },   // 4.0e-0049
        { 0x1.7624F8A762FD8p-0161,  0x0.2B2AAC18030B00p-0213 },   // 5.0e-0049
        { 0x1.C0F92A6276C9Dp-0161,  0x0.00999B5003A6C8p-0213 },   // 6.0e-0049
        { 0x1.05E6AE0EC54B0p-0160,  0x0.EB044544022148p-0212 },   // 7.0e-0049
        { 0x1.2B50C6EC4F313p-0160,  0x0.55BBBCE0026F30p-0212 },   // 8.0e-0049
        { 0x1.50BADFC9D9175p-0160,  0x0.C073347C02BD18p-0212 },   // 9.0e-0049
        { 0x1.7624F8A762FD8p-0160,  0x0.2B2AAC18030B00p-0212 },   // 1.0e-0048
        { 0x1.7624F8A762FD8p-0159,  0x0.2B2AAC18030B00p-0211 },   // 2.0e-0048
        { 0x1.189BBA7D8A3E2p-0158,  0x0.20600112024840p-0210 },   // 3.0e-0048
        { 0x1.7624F8A762FD8p-0158,  0x0.2B2AAC18030B00p-0210 },   // 4.0e-0048
        { 0x1.D3AE36D13BBCEp-0158,  0x0.35F5571E03CDC0p-0210 },   // 5.0e-0048
        { 0x1.189BBA7D8A3E2p-0157,  0x0.20600112024840p-0209 },   // 6.0e-0048
        { 0x1.47605992769DDp-0157,  0x0.25C5569502A9A0p-0209 },   // 7.0e-0048
        { 0x1.7624F8A762FD8p-0157,  0x0.2B2AAC18030B00p-0209 },   // 8.0e-0048
        { 0x1.A4E997BC4F5D3p-0157,  0x0.3090019B036C60p-0209 },   // 9.0e-0048
        { 0x1.D3AE36D13BBCEp-0157,  0x0.35F5571E03CDC0p-0209 },   // 1.0e-0047
        { 0x1.D3AE36D13BBCEp-0156,  0x0.35F5571E03CDC0p-0208 },   // 2.0e-0047
        { 0x1.5EC2A91CECCDAp-0155,  0x0.A878015682DA50p-0207 },   // 3.0e-0047
        { 0x1.D3AE36D13BBCEp-0155,  0x0.35F5571E03CDC0p-0207 },   // 4.0e-0047
        { 0x1.244CE242C5560p-0154,  0x0.E1B95672C26098p-0206 },   // 5.0e-0047
        { 0x1.5EC2A91CECCDAp-0154,  0x0.A878015682DA50p-0206 },   // 6.0e-0047
        { 0x1.99386FF714454p-0154,  0x0.6F36AC3A435408p-0206 },   // 7.0e-0047
        { 0x1.D3AE36D13BBCEp-0154,  0x0.35F5571E03CDC0p-0206 },   // 8.0e-0047
        { 0x1.0711FED5B19A3p-0153,  0x0.FE5A0100E223B8p-0205 },   // 9.0e-0047
        { 0x1.244CE242C5560p-0153,  0x0.E1B95672C26098p-0205 },   // 1.0e-0046
        { 0x1.244CE242C5560p-0152,  0x0.E1B95672C26098p-0204 },   // 2.0e-0046
        { 0x1.B673536428011p-0152,  0x0.529601AC2390E0p-0204 },   // 3.0e-0046
        { 0x1.244CE242C5560p-0151,  0x0.E1B95672C26098p-0203 },   // 4.0e-0046
        { 0x1.6D601AD376AB9p-0151,  0x0.1A27AC0F72F8B8p-0203 },   // 5.0e-0046
        { 0x1.B673536428011p-0151,  0x0.529601AC2390E0p-0203 },   // 6.0e-0046
        { 0x1.FF868BF4D9569p-0151,  0x0.8B045748D42908p-0203 },   // 7.0e-0046
        { 0x1.244CE242C5560p-0150,  0x0.E1B95672C26098p-0202 },   // 8.0e-0046
        { 0x1.48D67E8B1E00Cp-0150,  0x0.FDF081411AACA8p-0202 },   // 9.0e-0046
        { 0x1.6D601AD376AB9p-0150,  0x0.1A27AC0F72F8B8p-0202 },   // 1.0e-0045
        { 0x1.6D601AD376AB9p-0149,  0x0.1A27AC0F72F8B8p-0201 },   // 2.0e-0045
        { 0x1.1208141E9900Ap-0148,  0x0.D39DC10B963A88p-0200 },   // 3.0e-0045
        { 0x1.6D601AD376AB9p-0148,  0x0.1A27AC0F72F8B8p-0200 },   // 4.0e-0045
        { 0x1.C8B8218854567p-0148,  0x0.60B197134FB6E8p-0200 },   // 5.0e-0045
        { 0x1.1208141E9900Ap-0147,  0x0.D39DC10B963A88p-0199 },   // 6.0e-0045
        { 0x1.3FB4177907D61p-0147,  0x0.F6E2B68D8499A0p-0199 },   // 7.0e-0045
        { 0x1.6D601AD376AB9p-0147,  0x0.1A27AC0F72F8B8p-0199 },   // 8.0e-0045
        { 0x1.9B0C1E2DE5810p-0147,  0x0.3D6CA1916157D0p-0199 },   // 9.0e-0045
        { 0x1.C8B8218854567p-0147,  0x0.60B197134FB6E8p-0199 },   // 1.0e-0044
        { 0x1.C8B8218854567p-0146,  0x0.60B197134FB6E8p-0198 },   // 2.0e-0044
        { 0x1.568A19263F40Dp-0145,  0x0.8885314E7BC930p-0197 },   // 3.0e-0044
        { 0x1.C8B8218854567p-0145,  0x0.60B197134FB6E8p-0197 },   // 4.0e-0044
        { 0x1.1D7314F534B60p-0144,  0x0.9C6EFE6C11D250p-0196 },   // 5.0e-0044
        { 0x1.568A19263F40Dp-0144,  0x0.8885314E7BC930p-0196 },   // 6.0e-0044
        { 0x1.8FA11D5749CBAp-0144,  0x0.749B6430E5C010p-0196 },   // 7.0e-0044
        { 0x1.C8B8218854567p-0144,  0x0.60B197134FB6E8p-0196 },   // 8.0e-0044
        { 0x1.00E792DCAF70Ap-0143,  0x0.2663E4FADCD6E0p-0195 },   // 9.0e-0044
        { 0x1.1D7314F534B60p-0143,  0x0.9C6EFE6C11D250p-0195 },   // 1.0e-0043
        { 0x1.1D7314F534B60p-0142,  0x0.9C6EFE6C11D250p-0194 },   // 2.0e-0043
        { 0x1.AC2C9F6FCF110p-0142,  0x0.EAA67DA21ABB80p-0194 },   // 3.0e-0043
        { 0x1.1D7314F534B60p-0141,  0x0.9C6EFE6C11D250p-0193 },   // 4.0e-0043
        { 0x1.64CFDA3281E38p-0141,  0x0.C38ABE071646E8p-0193 },   // 5.0e-0043
        { 0x1.AC2C9F6FCF110p-0141,  0x0.EAA67DA21ABB80p-0193 },   // 6.0e-0043
        { 0x1.F38964AD1C3E9p-0141,  0x0.11C23D3D1F3010p-0193 },   // 7.0e-0043
        { 0x1.1D7314F534B60p-0140,  0x0.9C6EFE6C11D250p-0192 },   // 8.0e-0043
        { 0x1.41217793DB4CCp-0140,  0x0.AFFCDE39940CA0p-0192 },   // 9.0e-0043
        { 0x1.64CFDA3281E38p-0140,  0x0.C38ABE071646E8p-0192 },   // 1.0e-0042
        { 0x1.64CFDA3281E38p-0139,  0x0.C38ABE071646E8p-0191 },   // 2.0e-0042
        { 0x1.0B9BE3A5E16AAp-0138,  0x0.92A80E8550B530p-0190 },   // 3.0e-0042
        { 0x1.64CFDA3281E38p-0138,  0x0.C38ABE071646E8p-0190 },   // 4.0e-0042
        { 0x1.BE03D0BF225C6p-0138,  0x0.F46D6D88DBD8A0p-0190 },   // 5.0e-0042
        { 0x1.0B9BE3A5E16AAp-0137,  0x0.92A80E8550B530p-0189 },   // 6.0e-0042
        { 0x1.3835DEEC31A71p-0137,  0x0.AB196646337E08p-0189 },   // 7.0e-0042
        { 0x1.64CFDA3281E38p-0137,  0x0.C38ABE071646E8p-0189 },   // 8.0e-0042
        { 0x1.9169D578D21FFp-0137,  0x0.DBFC15C7F90FC8p-0189 },   // 9.0e-0042
        { 0x1.BE03D0BF225C6p-0137,  0x0.F46D6D88DBD8A0p-0189 },   // 1.0e-0041
        { 0x1.BE03D0BF225C6p-0136,  0x0.F46D6D88DBD8A0p-0188 },   // 2.0e-0041
        { 0x1.4E82DC8F59C55p-0135,  0x0.37521226A4E278p-0187 },   // 3.0e-0041
        { 0x1.BE03D0BF225C6p-0135,  0x0.F46D6D88DBD8A0p-0187 },   // 4.0e-0041
        { 0x1.16C262777579Cp-0134,  0x0.58C46475896760p-0186 },   // 5.0e-0041
        { 0x1.4E82DC8F59C55p-0134,  0x0.37521226A4E278p-0186 },   // 6.0e-0041
        { 0x1.864356A73E10Ep-0134,  0x0.15DFBFD7C05D90p-0186 },   // 7.0e-0041
        { 0x1.BE03D0BF225C6p-0134,  0x0.F46D6D88DBD8A0p-0186 },   // 8.0e-0041
        { 0x1.F5C44AD706A7Fp-0134,  0x0.D2FB1B39F753B8p-0186 },   // 9.0e-0041
        { 0x1.16C262777579Cp-0133,  0x0.58C46475896760p-0185 },   // 1.0e-0040
        { 0x1.16C262777579Cp-0132,  0x0.58C46475896760p-0184 },   // 2.0e-0040
        { 0x1.A22393B33036Ap-0132,  0x0.852696B04E1B18p-0184 },   // 3.0e-0040
        { 0x1.16C262777579Cp-0131,  0x0.58C46475896760p-0183 },   // 4.0e-0040
        { 0x1.5C72FB1552D83p-0131,  0x0.6EF57D92EBC140p-0183 },   // 5.0e-0040
        { 0x1.A22393B33036Ap-0131,  0x0.852696B04E1B18p-0183 },   // 6.0e-0040
        { 0x1.E7D42C510D951p-0131,  0x0.9B57AFCDB074F0p-0183 },   // 7.0e-0040
        { 0x1.16C262777579Cp-0130,  0x0.58C46475896760p-0182 },   // 8.0e-0040
        { 0x1.399AAEC66428Fp-0130,  0x0.E3DCF1043A9450p-0182 },   // 9.0e-0040
        { 0x1.5C72FB1552D83p-0130,  0x0.6EF57D92EBC140p-0182 },   // 1.0e-0039
        { 0x1.5C72FB1552D83p-0129,  0x0.6EF57D92EBC140p-0181 },   // 2.0e-0039
        { 0x1.05563C4FFE222p-0128,  0x0.93381E2E30D0F0p-0180 },   // 3.0e-0039
        { 0x1.5C72FB1552D83p-0128,  0x0.6EF57D92EBC140p-0180 },   // 4.0e-0039
        { 0x1.B38FB9DAA78E4p-0128,  0x0.4AB2DCF7A6B190p-0180 },   // 5.0e-0039
        { 0x1.05563C4FFE222p-0127,  0x0.93381E2E30D0F0p-0179 },   // 6.0e-0039
        { 0x1.30E49BB2A87D3p-0127,  0x0.0116CDE08E4918p-0179 },   // 7.0e-0039
        { 0x1.5C72FB1552D83p-0127,  0x0.6EF57D92EBC140p-0179 },   // 8.0e-0039
        { 0x1.88015A77FD333p-0127,  0x0.DCD42D45493968p-0179 },   // 9.0e-0039
        { 0x1.B38FB9DAA78E4p-0127,  0x0.4AB2DCF7A6B190p-0179 },   // 1.0e-0038
        { 0x1.B38FB9DAA78E4p-0126,  0x0.4AB2DCF7A6B190p-0178 },   // 2.0e-0038
        { 0x1.46ABCB63FDAABp-0125,  0x0.380625B9BD0528p-0177 },   // 3.0e-0038
        { 0x1.B38FB9DAA78E4p-0125,  0x0.4AB2DCF7A6B190p-0177 },   // 4.0e-0038
        { 0x1.1039D428A8B8Ep-0124,  0x0.AEAFCA1AC82EF8p-0176 },   // 5.0e-0038
        { 0x1.46ABCB63FDAABp-0124,  0x0.380625B9BD0528p-0176 },   // 6.0e-0038
        { 0x1.7D1DC29F529C7p-0124,  0x0.C15C8158B1DB58p-0176 },   // 7.0e-0038
        { 0x1.B38FB9DAA78E4p-0124,  0x0.4AB2DCF7A6B190p-0176 },   // 8.0e-0038
        { 0x1.EA01B115FC800p-0124,  0x0.D40938969B87C0p-0176 },   // 9.0e-0038
        { 0x1.1039D428A8B8Ep-0123,  0x0.AEAFCA1AC82EF8p-0175 },   // 1.0e-0037
        { 0x1.1039D428A8B8Ep-0122,  0x0.AEAFCA1AC82EF8p-0174 },   // 2.0e-0037
        { 0x1.9856BE3CFD156p-0122,  0x0.0607AF282C4678p-0174 },   // 3.0e-0037
        { 0x1.1039D428A8B8Ep-0121,  0x0.AEAFCA1AC82EF8p-0173 },   // 4.0e-0037
        { 0x1.54484932D2E72p-0121,  0x0.5A5BBCA17A3AB8p-0173 },   // 5.0e-0037
        { 0x1.9856BE3CFD156p-0121,  0x0.0607AF282C4678p-0173 },   // 6.0e-0037
        { 0x1.DC65334727439p-0121,  0x0.B1B3A1AEDE5230p-0173 },   // 7.0e-0037
        { 0x1.1039D428A8B8Ep-0120,  0x0.AEAFCA1AC82EF8p-0172 },   // 8.0e-0037
        { 0x1.32410EADBDD00p-0120,  0x0.8485C35E2134D8p-0172 },   // 9.0e-0037
        { 0x1.54484932D2E72p-0120,  0x0.5A5BBCA17A3AB8p-0172 },   // 1.0e-0036
        { 0x1.54484932D2E72p-0119,  0x0.5A5BBCA17A3AB8p-0171 },   // 2.0e-0036
        { 0x1.FE6C6DCC3C5ABp-0119,  0x0.87899AF2375810p-0171 },   // 3.0e-0036
        { 0x1.54484932D2E72p-0118,  0x0.5A5BBCA17A3AB8p-0170 },   // 4.0e-0036
        { 0x1.A95A5B7F87A0Ep-0118,  0x0.F0F2ABC9D8C968p-0170 },   // 5.0e-0036
        { 0x1.FE6C6DCC3C5ABp-0118,  0x0.87899AF2375810p-0170 },   // 6.0e-0036
        { 0x1.29BF400C788A4p-0117,  0x0.0F10450D4AF360p-0169 },   // 7.0e-0036
        { 0x1.54484932D2E72p-0117,  0x0.5A5BBCA17A3AB8p-0169 },   // 8.0e-0036
        { 0x1.7ED152592D440p-0117,  0x0.A5A73435A98210p-0169 },   // 9.0e-0036
        { 0x1.A95A5B7F87A0Ep-0117,  0x0.F0F2ABC9D8C968p-0169 },   // 1.0e-0035
        { 0x1.A95A5B7F87A0Ep-0116,  0x0.F0F2ABC9D8C968p-0168 },   // 2.0e-0035
        { 0x1.3F03C49FA5B8Bp-0115,  0x0.34B600D7629708p-0167 },   // 3.0e-0035
        { 0x1.A95A5B7F87A0Ep-0115,  0x0.F0F2ABC9D8C968p-0167 },   // 4.0e-0035
        { 0x1.09D8792FB4C49p-0114,  0x0.5697AB5E277DE0p-0166 },   // 5.0e-0035
        { 0x1.3F03C49FA5B8Bp-0114,  0x0.34B600D7629708p-0166 },   // 6.0e-0035
        { 0x1.742F100F96ACDp-0114,  0x0.12D456509DB038p-0166 },   // 7.0e-0035
        { 0x1.A95A5B7F87A0Ep-0114,  0x0.F0F2ABC9D8C968p-0166 },   // 8.0e-0035
        { 0x1.DE85A6EF78950p-0114,  0x0.CF11014313E290p-0166 },   // 9.0e-0035
        { 0x1.09D8792FB4C49p-0113,  0x0.5697AB5E277DE0p-0165 },   // 1.0e-0034
        { 0x1.09D8792FB4C49p-0112,  0x0.5697AB5E277DE0p-0164 },   // 2.0e-0034
        { 0x1.8EC4B5C78F26Ep-0112,  0x0.01E3810D3B3CD0p-0164 },   // 3.0e-0034
        { 0x1.09D8792FB4C49p-0111,  0x0.5697AB5E277DE0p-0163 },   // 4.0e-0034
        { 0x1.4C4E977BA1F5Bp-0111,  0x0.AC3D9635B15D58p-0163 },   // 5.0e-0034
        { 0x1.8EC4B5C78F26Ep-0111,  0x0.01E3810D3B3CD0p-0163 },   // 6.0e-0034
        { 0x1.D13AD4137C580p-0111,  0x0.57896BE4C51C48p-0163 },   // 7.0e-0034
        { 0x1.09D8792FB4C49p-0110,  0x0.5697AB5E277DE0p-0162 },   // 8.0e-0034
        { 0x1.2B138855AB5D2p-0110,  0x0.816AA0C9EC6D98p-0162 },   // 9.0e-0034
        { 0x1.4C4E977BA1F5Bp-0110,  0x0.AC3D9635B15D58p-0162 },   // 1.0e-0033
        { 0x1.4C4E977BA1F5Bp-0109,  0x0.AC3D9635B15D58p-0161 },   // 2.0e-0033
        { 0x1.F275E33972F09p-0109,  0x0.825C61508A0C00p-0161 },   // 3.0e-0033
        { 0x1.4C4E977BA1F5Bp-0108,  0x0.AC3D9635B15D58p-0160 },   // 4.0e-0033
        { 0x1.9F623D5A8A732p-0108,  0x0.974CFBC31DB4B0p-0160 },   // 5.0e-0033
        { 0x1.F275E33972F09p-0108,  0x0.825C61508A0C00p-0160 },   // 6.0e-0033
        { 0x1.22C4C48C2DB70p-0107,  0x0.36B5E36EFB31A8p-0159 },   // 7.0e-0033
        { 0x1.4C4E977BA1F5Bp-0107,  0x0.AC3D9635B15D58p-0159 },   // 8.0e-0033
        { 0x1.75D86A6B16347p-0107,  0x0.21C548FC678900p-0159 },   // 9.0e-0033
        { 0x1.9F623D5A8A732p-0107,  0x0.974CFBC31DB4B0p-0159 },   // 1.0e-0032
        { 0x1.9F623D5A8A732p-0106,  0x0.974CFBC31DB4B0p-0158 },   // 2.0e-0032
        { 0x1.3789AE03E7D65p-0105,  0x0.F179BCD2564780p-0157 },   // 3.0e-0032
        { 0x1.9F623D5A8A732p-0105,  0x0.974CFBC31DB4B0p-0157 },   // 4.0e-0032
        { 0x1.039D66589687Fp-0104,  0x0.9E901D59F290E8p-0156 },   // 5.0e-0032
        { 0x1.3789AE03E7D65p-0104,  0x0.F179BCD2564780p-0156 },   // 6.0e-0032
        { 0x1.6B75F5AF3924Cp-0104,  0x0.44635C4AB9FE18p-0156 },   // 7.0e-0032
        { 0x1.9F623D5A8A732p-0104,  0x0.974CFBC31DB4B0p-0156 },   // 8.0e-0032
        { 0x1.D34E8505DBC18p-0104,  0x0.EA369B3B816B40p-0156 },   // 9.0e-0032
        { 0x1.039D66589687Fp-0103,  0x0.9E901D59F290E8p-0155 },   // 1.0e-0031
        { 0x1.039D66589687Fp-0102,  0x0.9E901D59F290E8p-0154 },   // 2.0e-0031
        { 0x1.856C1984E1CBFp-0102,  0x0.6DD82C06EBD960p-0154 },   // 3.0e-0031
        { 0x1.039D66589687Fp-0101,  0x0.9E901D59F290E8p-0153 },   // 4.0e-0031
        { 0x1.4484BFEEBC29Fp-0101,  0x0.863424B06F3528p-0153 },   // 5.0e-0031
        { 0x1.856C1984E1CBFp-0101,  0x0.6DD82C06EBD960p-0153 },   // 6.0e-0031
        { 0x1.C653731B076DFp-0101,  0x0.557C335D687DA0p-0153 },   // 7.0e-0031
        { 0x1.039D66589687Fp-0100,  0x0.9E901D59F290E8p-0152 },   // 8.0e-0031
        { 0x1.24111323A958Fp-0100,  0x0.9262210530E308p-0152 },   // 9.0e-0031
        { 0x1.4484BFEEBC29Fp-0100,  0x0.863424B06F3528p-0152 },   // 1.0e-0030
        { 0x1.4484BFEEBC29Fp-0099,  0x0.863424B06F3528p-0151 },   // 2.0e-0030
        { 0x1.E6C71FE61A3EFp-0099,  0x0.494E3708A6CFB8p-0151 },   // 3.0e-0030
        { 0x1.4484BFEEBC29Fp-0098,  0x0.863424B06F3528p-0150 },   // 4.0e-0030
        { 0x1.95A5EFEA6B347p-0098,  0x0.67C12DDC8B0270p-0150 },   // 5.0e-0030
        { 0x1.E6C71FE61A3EFp-0098,  0x0.494E3708A6CFB8p-0150 },   // 6.0e-0030
        { 0x1.1BF427F0E4A4Bp-0097,  0x0.956DA01A614E80p-0149 },   // 7.0e-0030
        { 0x1.4484BFEEBC29Fp-0097,  0x0.863424B06F3528p-0149 },   // 8.0e-0030
        { 0x1.6D1557EC93AF3p-0097,  0x0.76FAA9467D1BC8p-0149 },   // 9.0e-0030
        { 0x1.95A5EFEA6B347p-0097,  0x0.67C12DDC8B0270p-0149 },   // 1.0e-0029
        { 0x1.95A5EFEA6B347p-0096,  0x0.67C12DDC8B0270p-0148 },   // 2.0e-0029
        { 0x1.303C73EFD0675p-0095,  0x0.8DD0E2656841D0p-0147 },   // 3.0e-0029
        { 0x1.95A5EFEA6B347p-0095,  0x0.67C12DDC8B0270p-0147 },   // 4.0e-0029
        { 0x1.FB0F6BE506019p-0095,  0x0.41B17953ADC310p-0147 },   // 5.0e-0029
        { 0x1.303C73EFD0675p-0094,  0x0.8DD0E2656841D0p-0146 },   // 6.0e-0029
        { 0x1.62F131ED1DCDEp-0094,  0x0.7AC90820F9A220p-0146 },   // 7.0e-0029
        { 0x1.95A5EFEA6B347p-0094,  0x0.67C12DDC8B0270p-0146 },   // 8.0e-0029
        { 0x1.C85AADE7B89B0p-0094,  0x0.54B953981C62C0p-0146 },   // 9.0e-0029
        { 0x1.FB0F6BE506019p-0094,  0x0.41B17953ADC310p-0146 },   // 1.0e-0028
        { 0x1.FB0F6BE506019p-0093,  0x0.41B17953ADC310p-0145 },   // 2.0e-0028
        { 0x1.7C4B90EBC4812p-0092,  0x0.F1451AFEC25248p-0144 },   // 3.0e-0028
        { 0x1.FB0F6BE506019p-0092,  0x0.41B17953ADC310p-0144 },   // 4.0e-0028
        { 0x1.3CE9A36F23C0Fp-0091,  0x0.C90EEBD44C99E8p-0143 },   // 5.0e-0028
        { 0x1.7C4B90EBC4812p-0091,  0x0.F1451AFEC25248p-0143 },   // 6.0e-0028
        { 0x1.BBAD7E6865416p-0091,  0x0.197B4A29380AA8p-0143 },   // 7.0e-0028
        { 0x1.FB0F6BE506019p-0091,  0x0.41B17953ADC310p-0143 },   // 8.0e-0028
        { 0x1.1D38ACB0D360Ep-0090,  0x0.34F3D43F11BDB8p-0142 },   // 9.0e-0028
        { 0x1.3CE9A36F23C0Fp-0090,  0x0.C90EEBD44C99E8p-0142 },   // 1.0e-0027
        { 0x1.3CE9A36F23C0Fp-0089,  0x0.C90EEBD44C99E8p-0141 },   // 2.0e-0027
        { 0x1.DB5E7526B5A17p-0089,  0x0.AD9661BE72E6D8p-0141 },   // 3.0e-0027
        { 0x1.3CE9A36F23C0Fp-0088,  0x0.C90EEBD44C99E8p-0140 },   // 4.0e-0027
        { 0x1.8C240C4AECB13p-0088,  0x0.BB52A6C95FC060p-0140 },   // 5.0e-0027
        { 0x1.DB5E7526B5A17p-0088,  0x0.AD9661BE72E6D8p-0140 },   // 6.0e-0027
        { 0x1.154C6F013F48Dp-0087,  0x0.CFED0E59C306A8p-0139 },   // 7.0e-0027
        { 0x1.3CE9A36F23C0Fp-0087,  0x0.C90EEBD44C99E8p-0139 },   // 8.0e-0027
        { 0x1.6486D7DD08391p-0087,  0x0.C230C94ED62D20p-0139 },   // 9.0e-0027
        { 0x1.8C240C4AECB13p-0087,  0x0.BB52A6C95FC060p-0139 },   // 1.0e-0026
        { 0x1.8C240C4AECB13p-0086,  0x0.BB52A6C95FC060p-0138 },   // 2.0e-0026
        { 0x1.291B09383184Ep-0085,  0x0.CC7DFD1707D048p-0137 },   // 3.0e-0026
        { 0x1.8C240C4AECB13p-0085,  0x0.BB52A6C95FC060p-0137 },   // 4.0e-0026
        { 0x1.EF2D0F5DA7DD8p-0085,  0x0.AA27507BB7B078p-0137 },   // 5.0e-0026
        { 0x1.291B09383184Ep-0084,  0x0.CC7DFD1707D048p-0136 },   // 6.0e-0026
        { 0x1.5A9F8AC18F1B1p-0084,  0x0.43E851F033C858p-0136 },   // 7.0e-0026
        { 0x1.8C240C4AECB13p-0084,  0x0.BB52A6C95FC060p-0136 },   // 8.0e-0026
        { 0x1.BDA88DD44A476p-0084,  0x0.32BCFBA28BB870p-0136 },   // 9.0e-0026
        { 0x1.EF2D0F5DA7DD8p-0084,  0x0.AA27507BB7B078p-0136 },   // 1.0e-0025
        { 0x1.EF2D0F5DA7DD8p-0083,  0x0.AA27507BB7B078p-0135 },   // 2.0e-0025
        { 0x1.7361CB863DE62p-0082,  0x0.7F9D7C5CC9C458p-0134 },   // 3.0e-0025
        { 0x1.EF2D0F5DA7DD8p-0082,  0x0.AA27507BB7B078p-0134 },   // 4.0e-0025
        { 0x1.357C299A88EA7p-0081,  0x0.6A58924D52CE48p-0133 },   // 5.0e-0025
        { 0x1.7361CB863DE62p-0081,  0x0.7F9D7C5CC9C458p-0133 },   // 6.0e-0025
        { 0x1.B1476D71F2E1Dp-0081,  0x0.94E2666C40BA68p-0133 },   // 7.0e-0025
        { 0x1.EF2D0F5DA7DD8p-0081,  0x0.AA27507BB7B078p-0133 },   // 8.0e-0025
        { 0x1.168958A4AE6C9p-0080,  0x0.DFB61D45975340p-0132 },   // 9.0e-0025
        { 0x1.357C299A88EA7p-0080,  0x0.6A58924D52CE48p-0132 },   // 1.0e-0024
        { 0x1.357C299A88EA7p-0079,  0x0.6A58924D52CE48p-0131 },   // 2.0e-0024
        { 0x1.D03A3E67CD5FBp-0079,  0x0.1F84DB73FC3570p-0131 },   // 3.0e-0024
        { 0x1.357C299A88EA7p-0078,  0x0.6A58924D52CE48p-0130 },   // 4.0e-0024
        { 0x1.82DB34012B251p-0078,  0x0.44EEB6E0A781E0p-0130 },   // 5.0e-0024
        { 0x1.D03A3E67CD5FBp-0078,  0x0.1F84DB73FC3570p-0130 },   // 6.0e-0024
        { 0x1.0ECCA46737CD2p-0077,  0x0.7D0D8003A87480p-0129 },   // 7.0e-0024
        { 0x1.357C299A88EA7p-0077,  0x0.6A58924D52CE48p-0129 },   // 8.0e-0024
        { 0x1.5C2BAECDDA07Cp-0077,  0x0.57A3A496FD2818p-0129 },   // 9.0e-0024
        { 0x1.82DB34012B251p-0077,  0x0.44EEB6E0A781E0p-0129 },   // 1.0e-0023
        { 0x1.82DB34012B251p-0076,  0x0.44EEB6E0A781E0p-0128 },   // 2.0e-0023
        { 0x1.22246700E05BCp-0075,  0x0.F3B309287DA168p-0127 },   // 3.0e-0023
        { 0x1.82DB34012B251p-0075,  0x0.44EEB6E0A781E0p-0127 },   // 4.0e-0023
        { 0x1.E392010175EE5p-0075,  0x0.962A6498D16258p-0127 },   // 5.0e-0023
        { 0x1.22246700E05BCp-0074,  0x0.F3B309287DA168p-0126 },   // 6.0e-0023
        { 0x1.527FCD8105C07p-0074,  0x0.1C50E0049291A0p-0126 },   // 7.0e-0023
        { 0x1.82DB34012B251p-0074,  0x0.44EEB6E0A781E0p-0126 },   // 8.0e-0023
        { 0x1.B3369A815089Bp-0074,  0x0.6D8C8DBCBC7218p-0126 },   // 9.0e-0023
        { 0x1.E392010175EE5p-0074,  0x0.962A6498D16258p-0126 },   // 1.0e-0022
        { 0x1.E392010175EE5p-0073,  0x0.962A6498D16258p-0125 },   // 2.0e-0022
        { 0x1.6AAD80C11872Cp-0072,  0x0.309FCB729D09C0p-0124 },   // 3.0e-0022
        { 0x1.E392010175EE5p-0072,  0x0.962A6498D16258p-0124 },   // 4.0e-0022
        { 0x1.2E3B40A0E9B4Fp-0071,  0x0.7DDA7EDF82DD78p-0123 },   // 5.0e-0022
        { 0x1.6AAD80C11872Cp-0071,  0x0.309FCB729D09C0p-0123 },   // 6.0e-0022
        { 0x1.A71FC0E147308p-0071,  0x0.E3651805B73610p-0123 },   // 7.0e-0022
        { 0x1.E392010175EE5p-0071,  0x0.962A6498D16258p-0123 },   // 8.0e-0022
        { 0x1.10022090D2561p-0070,  0x0.2477D895F5C750p-0122 },   // 9.0e-0022
        { 0x1.2E3B40A0E9B4Fp-0070,  0x0.7DDA7EDF82DD78p-0122 },   // 1.0e-0021
        { 0x1.2E3B40A0E9B4Fp-0069,  0x0.7DDA7EDF82DD78p-0121 },   // 2.0e-0021
        { 0x1.C558E0F15E8F7p-0069,  0x0.3CC7BE4F444C30p-0121 },   // 3.0e-0021
        { 0x1.2E3B40A0E9B4Fp-0068,  0x0.7DDA7EDF82DD78p-0120 },   // 4.0e-0021
        { 0x1.79CA10C924223p-0068,  0x0.5D511E976394D0p-0120 },   // 5.0e-0021
        { 0x1.C558E0F15E8F7p-0068,  0x0.3CC7BE4F444C30p-0120 },   // 6.0e-0021
        { 0x1.0873D88CCC7E5p-0067,  0x0.8E1F2F039281C8p-0119 },   // 7.0e-0021
        { 0x1.2E3B40A0E9B4Fp-0067,  0x0.7DDA7EDF82DD78p-0119 },   // 8.0e-0021
        { 0x1.5402A8B506EB9p-0067,  0x0.6D95CEBB733928p-0119 },   // 9.0e-0021
        { 0x1.79CA10C924223p-0067,  0x0.5D511E976394D0p-0119 },   // 1.0e-0020
        { 0x1.79CA10C924223p-0066,  0x0.5D511E976394D0p-0118 },   // 2.0e-0020
        { 0x1.1B578C96DB19Ap-0065,  0x0.85FCD6F18AAFA0p-0117 },   // 3.0e-0020
        { 0x1.79CA10C924223p-0065,  0x0.5D511E976394D0p-0117 },   // 4.0e-0020
        { 0x1.D83C94FB6D2ACp-0065,  0x0.34A5663D3C7A08p-0117 },   // 5.0e-0020
        { 0x1.1B578C96DB19Ap-0064,  0x0.85FCD6F18AAFA0p-0116 },   // 6.0e-0020
        { 0x1.4A90CEAFFF9DEp-0064,  0x0.F1A6FAC4772238p-0116 },   // 7.0e-0020
        { 0x1.79CA10C924223p-0064,  0x0.5D511E976394D0p-0116 },   // 8.0e-0020
        { 0x1.A90352E248A67p-0064,  0x0.C8FB426A500770p-0116 },   // 9.0e-0020
        { 0x1.D83C94FB6D2ACp-0064,  0x0.34A5663D3C7A08p-0116 },   // 1.0e-0019
        { 0x1.D83C94FB6D2ACp-0063,  0x0.34A5663D3C7A08p-0115 },   // 2.0e-0019
        { 0x1.622D6FBC91E01p-0062,  0x0.277C0CADED5B88p-0114 },   // 3.0e-0019
        { 0x1.D83C94FB6D2ACp-0062,  0x0.34A5663D3C7A08p-0114 },   // 4.0e-0019
        { 0x1.2725DD1D243ABp-0061,  0x0.A0E75FE645CC48p-0113 },   // 5.0e-0019
        { 0x1.622D6FBC91E01p-0061,  0x0.277C0CADED5B88p-0113 },   // 6.0e-0019
        { 0x1.9D35025BFF856p-0061,  0x0.AE10B97594EAC8p-0113 },   // 7.0e-0019
        { 0x1.D83C94FB6D2ACp-0061,  0x0.34A5663D3C7A08p-0113 },   // 8.0e-0019
        { 0x1.09A213CD6D680p-0060,  0x0.DD9D09827204A0p-0112 },   // 9.0e-0019
        { 0x1.2725DD1D243ABp-0060,  0x0.A0E75FE645CC48p-0112 },   // 1.0e-0018
        { 0x1.2725DD1D243ABp-0059,  0x0.A0E75FE645CC48p-0111 },   // 2.0e-0018
        { 0x1.BAB8CBABB6581p-0059,  0x0.715B0FD968B268p-0111 },   // 3.0e-0018
        { 0x1.2725DD1D243ABp-0058,  0x0.A0E75FE645CC48p-0110 },   // 4.0e-0018
        { 0x1.70EF54646D496p-0058,  0x0.892137DFD73F58p-0110 },   // 5.0e-0018
        { 0x1.BAB8CBABB6581p-0058,  0x0.715B0FD968B268p-0110 },   // 6.0e-0018
        { 0x1.024121797FB36p-0057,  0x0.2CCA73E97D12B8p-0109 },   // 7.0e-0018
        { 0x1.2725DD1D243ABp-0057,  0x0.A0E75FE645CC48p-0109 },   // 8.0e-0018
        { 0x1.4C0A98C0C8C21p-0057,  0x0.15044BE30E85D0p-0109 },   // 9.0e-0018
        { 0x1.70EF54646D496p-0057,  0x0.892137DFD73F58p-0109 },   // 1.0e-0017
        { 0x1.70EF54646D496p-0056,  0x0.892137DFD73F58p-0108 },   // 2.0e-0017
        { 0x1.14B37F4B51F70p-0055,  0x0.E6D8E9E7E16F80p-0107 },   // 3.0e-0017
        { 0x1.70EF54646D496p-0055,  0x0.892137DFD73F58p-0107 },   // 4.0e-0017
        { 0x1.CD2B297D889BCp-0055,  0x0.2B6985D7CD0F30p-0107 },   // 5.0e-0017
        { 0x1.14B37F4B51F70p-0054,  0x0.E6D8E9E7E16F80p-0106 },   // 6.0e-0017
        { 0x1.42D169D7DFA03p-0054,  0x0.B7FD10E3DC5768p-0106 },   // 7.0e-0017
        { 0x1.70EF54646D496p-0054,  0x0.892137DFD73F58p-0106 },   // 8.0e-0017
        { 0x1.9F0D3EF0FAF29p-0054,  0x0.5A455EDBD22740p-0106 },   // 9.0e-0017
        { 0x1.CD2B297D889BCp-0054,  0x0.2B6985D7CD0F30p-0106 },   // 1.0e-0016
        { 0x1.CD2B297D889BCp-0053,  0x0.2B6985D7CD0F30p-0105 },   // 2.0e-0016
        { 0x1.59E05F1E2674Dp-0052,  0x0.208F2461D9CB60p-0104 },   // 3.0e-0016
        { 0x1.CD2B297D889BCp-0052,  0x0.2B6985D7CD0F30p-0104 },   // 4.0e-0016
        { 0x1.203AF9EE75615p-0051,  0x0.9B21F3A6E02978p-0103 },   // 5.0e-0016
        { 0x1.59E05F1E2674Dp-0051,  0x0.208F2461D9CB60p-0103 },   // 6.0e-0016
        { 0x1.9385C44DD7884p-0051,  0x0.A5FC551CD36D48p-0103 },   // 7.0e-0016
        { 0x1.CD2B297D889BCp-0051,  0x0.2B6985D7CD0F30p-0103 },   // 8.0e-0016
        { 0x1.036847569CD79p-0050,  0x0.D86B5B49635888p-0102 },   // 9.0e-0016
        { 0x1.203AF9EE75615p-0050,  0x0.9B21F3A6E02978p-0102 },   // 1.0e-0015
        { 0x1.203AF9EE75615p-0049,  0x0.9B21F3A6E02978p-0101 },   // 2.0e-0015
        { 0x1.B05876E5B0120p-0049,  0x0.68B2ED7A503E38p-0101 },   // 3.0e-0015
        { 0x1.203AF9EE75615p-0048,  0x0.9B21F3A6E02978p-0100 },   // 4.0e-0015
        { 0x1.6849B86A12B9Bp-0048,  0x0.01EA70909833D8p-0100 },   // 5.0e-0015
        { 0x1.B05876E5B0120p-0048,  0x0.68B2ED7A503E38p-0100 },   // 6.0e-0015
        { 0x1.F86735614D6A5p-0048,  0x0.CF7B6A64084898p-0100 },   // 7.0e-0015
        { 0x1.203AF9EE75615p-0047,  0x0.9B21F3A6E02978p-0099 },   // 8.0e-0015
        { 0x1.4442592C440D8p-0047,  0x0.4E86321BBC2EA8p-0099 },   // 9.0e-0015
        { 0x1.6849B86A12B9Bp-0047,  0x0.01EA70909833D8p-0099 },   // 1.0e-0014
        { 0x1.6849B86A12B9Bp-0046,  0x0.01EA70909833D8p-0098 },   // 2.0e-0014
        { 0x1.0E374A4F8E0B4p-0045,  0x0.416FD46C7226E0p-0097 },   // 3.0e-0014
        { 0x1.6849B86A12B9Bp-0045,  0x0.01EA70909833D8p-0097 },   // 4.0e-0014
        { 0x1.C25C268497681p-0045,  0x0.C2650CB4BE40D0p-0097 },   // 5.0e-0014
        { 0x1.0E374A4F8E0B4p-0044,  0x0.416FD46C7226E0p-0096 },   // 6.0e-0014
        { 0x1.3B40815CD0627p-0044,  0x0.A1AD227E852D60p-0096 },   // 7.0e-0014
        { 0x1.6849B86A12B9Bp-0044,  0x0.01EA70909833D8p-0096 },   // 8.0e-0014
        { 0x1.9552EF775510Ep-0044,  0x0.6227BEA2AB3A58p-0096 },   // 9.0e-0014
        { 0x1.C25C268497681p-0044,  0x0.C2650CB4BE40D0p-0096 },   // 1.0e-0013
        { 0x1.C25C268497681p-0043,  0x0.C2650CB4BE40D0p-0095 },   // 2.0e-0013
        { 0x1.51C51CE3718E1p-0042,  0x0.51CBC9878EB0A0p-0094 },   // 3.0e-0013
        { 0x1.C25C268497681p-0042,  0x0.C2650CB4BE40D0p-0094 },   // 4.0e-0013
        { 0x1.19799812DEA11p-0041,  0x0.197F27F0F6E880p-0093 },   // 5.0e-0013
        { 0x1.51C51CE3718E1p-0041,  0x0.51CBC9878EB0A0p-0093 },   // 6.0e-0013
        { 0x1.8A10A1B4047B1p-0041,  0x0.8A186B1E2678B8p-0093 },   // 7.0e-0013
        { 0x1.C25C268497681p-0041,  0x0.C2650CB4BE40D0p-0093 },   // 8.0e-0013
        { 0x1.FAA7AB552A551p-0041,  0x0.FAB1AE4B5608F0p-0093 },   // 9.0e-0013
        { 0x1.19799812DEA11p-0040,  0x0.197F27F0F6E880p-0092 },   // 1.0e-0012
        { 0x1.19799812DEA11p-0039,  0x0.197F27F0F6E880p-0091 },   // 2.0e-0012
        { 0x1.A636641C4DF19p-0039,  0x0.A63EBBE9725CC8p-0091 },   // 3.0e-0012
        { 0x1.19799812DEA11p-0038,  0x0.197F27F0F6E880p-0090 },   // 4.0e-0012
        { 0x1.5FD7FE1796495p-0038,  0x0.5FDEF1ED34A2A0p-0090 },   // 5.0e-0012
        { 0x1.A636641C4DF19p-0038,  0x0.A63EBBE9725CC8p-0090 },   // 6.0e-0012
        { 0x1.EC94CA210599Dp-0038,  0x0.EC9E85E5B016E8p-0090 },   // 7.0e-0012
        { 0x1.19799812DEA11p-0037,  0x0.197F27F0F6E880p-0089 },   // 8.0e-0012
        { 0x1.3CA8CB153A753p-0037,  0x0.3CAF0CEF15C590p-0089 },   // 9.0e-0012
        { 0x1.5FD7FE1796495p-0037,  0x0.5FDEF1ED34A2A0p-0089 },   // 1.0e-0011
        { 0x1.5FD7FE1796495p-0036,  0x0.5FDEF1ED34A2A0p-0088 },   // 2.0e-0011
        { 0x1.07E1FE91B0B70p-0035,  0x0.07E73571E779F8p-0087 },   // 3.0e-0011
        { 0x1.5FD7FE1796495p-0035,  0x0.5FDEF1ED34A2A0p-0087 },   // 4.0e-0011
        { 0x1.B7CDFD9D7BDBAp-0035,  0x0.B7D6AE6881CB50p-0087 },   // 5.0e-0011
        { 0x1.07E1FE91B0B70p-0034,  0x0.07E73571E779F8p-0086 },   // 6.0e-0011
        { 0x1.33DCFE54A3802p-0034,  0x0.B3E313AF8E0E50p-0086 },   // 7.0e-0011
        { 0x1.5FD7FE1796495p-0034,  0x0.5FDEF1ED34A2A0p-0086 },   // 8.0e-0011
        { 0x1.8BD2FDDA89128p-0034,  0x0.0BDAD02ADB36F8p-0086 },   // 9.0e-0011
        { 0x1.B7CDFD9D7BDBAp-0034,  0x0.B7D6AE6881CB50p-0086 },   // 1.0e-0010
        { 0x1.B7CDFD9D7BDBAp-0033,  0x0.B7D6AE6881CB50p-0085 },   // 2.0e-0010
        { 0x1.49DA7E361CE4Cp-0032,  0x0.09E102CE615878p-0084 },   // 3.0e-0010
        { 0x1.B7CDFD9D7BDBAp-0032,  0x0.B7D6AE6881CB50p-0084 },   // 4.0e-0010
        { 0x1.12E0BE826D694p-0031,  0x0.B2E62D01511F10p-0083 },   // 5.0e-0010
        { 0x1.49DA7E361CE4Cp-0031,  0x0.09E102CE615878p-0083 },   // 6.0e-0010
        { 0x1.80D43DE9CC603p-0031,  0x0.60DBD89B7191E0p-0083 },   // 7.0e-0010
        { 0x1.B7CDFD9D7BDBAp-0031,  0x0.B7D6AE6881CB50p-0083 },   // 8.0e-0010
        { 0x1.EEC7BD512B572p-0031,  0x0.0ED184359204B8p-0083 },   // 9.0e-0010
        { 0x1.12E0BE826D694p-0030,  0x0.B2E62D01511F10p-0082 },   // 1.0e-0009
        { 0x1.12E0BE826D694p-0029,  0x0.B2E62D01511F10p-0081 },   // 2.0e-0009
        { 0x1.9C511DC3A41DFp-0029,  0x0.0C594381F9AE98p-0081 },   // 3.0e-0009
        { 0x1.12E0BE826D694p-0028,  0x0.B2E62D01511F10p-0080 },   // 4.0e-0009
        { 0x1.5798EE2308C39p-0028,  0x0.DF9FB841A566D0p-0080 },   // 5.0e-0009
        { 0x1.9C511DC3A41DFp-0028,  0x0.0C594381F9AE98p-0080 },   // 6.0e-0009
        { 0x1.E1094D643F784p-0028,  0x0.3912CEC24DF660p-0080 },   // 7.0e-0009
        { 0x1.12E0BE826D694p-0027,  0x0.B2E62D01511F10p-0079 },   // 8.0e-0009
        { 0x1.353CD652BB167p-0027,  0x0.4942F2A17B42F0p-0079 },   // 9.0e-0009
        { 0x1.5798EE2308C39p-0027,  0x0.DF9FB841A566D0p-0079 },   // 1.0e-0008
        { 0x1.5798EE2308C39p-0026,  0x0.DF9FB841A566D0p-0078 },   // 2.0e-0008
        { 0x1.01B2B29A4692Bp-0025,  0x0.67B7CA313C0D20p-0077 },   // 3.0e-0008
        { 0x1.5798EE2308C39p-0025,  0x0.DF9FB841A566D0p-0077 },   // 4.0e-0008
        { 0x1.AD7F29ABCAF48p-0025,  0x0.5787A6520EC088p-0077 },   // 5.0e-0008
        { 0x1.01B2B29A4692Bp-0024,  0x0.67B7CA313C0D20p-0076 },   // 6.0e-0008
        { 0x1.2CA5D05EA7AB2p-0024,  0x0.A3ABC13970B9F8p-0076 },   // 7.0e-0008
        { 0x1.5798EE2308C39p-0024,  0x0.DF9FB841A566D0p-0076 },   // 8.0e-0008
        { 0x1.828C0BE769DC1p-0024,  0x0.1B93AF49DA13B0p-0076 },   // 9.0e-0008
        { 0x1.AD7F29ABCAF48p-0024,  0x0.5787A6520EC088p-0076 },   // 1.0e-0007
        { 0x1.AD7F29ABCAF48p-0023,  0x0.5787A6520EC088p-0075 },   // 2.0e-0007
        { 0x1.421F5F40D8376p-0022,  0x0.41A5BCBD8B1068p-0074 },   // 3.0e-0007
        { 0x1.AD7F29ABCAF48p-0022,  0x0.5787A6520EC088p-0074 },   // 4.0e-0007
        { 0x1.0C6F7A0B5ED8Dp-0021,  0x0.36B4C7F3493858p-0073 },   // 5.0e-0007
        { 0x1.421F5F40D8376p-0021,  0x0.41A5BCBD8B1068p-0073 },   // 6.0e-0007
        { 0x1.77CF44765195Fp-0021,  0x0.4C96B187CCE878p-0073 },   // 7.0e-0007
        { 0x1.AD7F29ABCAF48p-0021,  0x0.5787A6520EC088p-0073 },   // 8.0e-0007
        { 0x1.E32F0EE144531p-0021,  0x0.62789B1C509898p-0073 },   // 9.0e-0007
        { 0x1.0C6F7A0B5ED8Dp-0020,  0x0.36B4C7F3493858p-0072 },   // 1.0e-0006
        { 0x1.0C6F7A0B5ED8Dp-0019,  0x0.36B4C7F3493858p-0071 },   // 2.0e-0006
        { 0x1.92A737110E453p-0019,  0x0.D20F2BECEDD480p-0071 },   // 3.0e-0006
        { 0x1.0C6F7A0B5ED8Dp-0018,  0x0.36B4C7F3493858p-0070 },   // 4.0e-0006
        { 0x1.4F8B588E368F0p-0018,  0x0.8461F9F01B8668p-0070 },   // 5.0e-0006
        { 0x1.92A737110E453p-0018,  0x0.D20F2BECEDD480p-0070 },   // 6.0e-0006
        { 0x1.D5C31593E5FB7p-0018,  0x0.1FBC5DE9C02298p-0070 },   // 7.0e-0006
        { 0x1.0C6F7A0B5ED8Dp-0017,  0x0.36B4C7F3493858p-0069 },   // 8.0e-0006
        { 0x1.2DFD694CCAB3Ep-0017,  0x0.DD8B60F1B25F60p-0069 },   // 9.0e-0006
        { 0x1.4F8B588E368F0p-0017,  0x0.8461F9F01B8668p-0069 },   // 1.0e-0005
        { 0x1.4F8B588E368F0p-0016,  0x0.8461F9F01B8668p-0068 },   // 2.0e-0005
        { 0x1.F75104D551D68p-0016,  0x0.C692F6E82949A0p-0068 },   // 3.0e-0005
        { 0x1.4F8B588E368F0p-0015,  0x0.8461F9F01B8668p-0067 },   // 4.0e-0005
        { 0x1.A36E2EB1C432Cp-0015,  0x0.A57A786C226808p-0067 },   // 5.0e-0005
        { 0x1.F75104D551D68p-0015,  0x0.C692F6E82949A0p-0067 },   // 6.0e-0005
        { 0x1.2599ED7C6FBD2p-0014,  0x0.73D5BAB21815A0p-0066 },   // 7.0e-0005
        { 0x1.4F8B588E368F0p-0014,  0x0.8461F9F01B8668p-0066 },   // 8.0e-0005
        { 0x1.797CC39FFD60Ep-0014,  0x0.94EE392E1EF738p-0066 },   // 9.0e-0005
        { 0x1.A36E2EB1C432Cp-0014,  0x0.A57A786C226808p-0066 },   // 1.0e-0004
        { 0x1.A36E2EB1C432Cp-0013,  0x0.A57A786C226808p-0065 },   // 2.0e-0004
        { 0x1.3A92A30553261p-0012,  0x0.7C1BDA5119CE00p-0064 },   // 3.0e-0004
        { 0x1.A36E2EB1C432Cp-0012,  0x0.A57A786C226808p-0064 },   // 4.0e-0004
        { 0x1.0624DD2F1A9FBp-0011,  0x0.E76C8B43958100p-0063 },   // 5.0e-0004
        { 0x1.3A92A30553261p-0011,  0x0.7C1BDA5119CE00p-0063 },   // 6.0e-0004
        { 0x1.6F0068DB8BAC7p-0011,  0x0.10CB295E9E1B08p-0063 },   // 7.0e-0004
        { 0x1.A36E2EB1C432Cp-0011,  0x0.A57A786C226808p-0063 },   // 8.0e-0004
        { 0x1.D7DBF487FCB92p-0011,  0x0.3A29C779A6B508p-0063 },   // 9.0e-0004
        { 0x1.0624DD2F1A9FBp-0010,  0x0.E76C8B43958100p-0062 },   // 1.0e-0003
        { 0x1.0624DD2F1A9FBp-0009,  0x0.E76C8B43958100p-0061 },   // 2.0e-0003
        { 0x1.89374BC6A7EF9p-0009,  0x0.DB22D0E5604188p-0061 },   // 3.0e-0003
        { 0x1.0624DD2F1A9FBp-0008,  0x0.E76C8B43958100p-0060 },   // 4.0e-0003
        { 0x1.47AE147AE147Ap-0008,  0x0.E147AE147AE140p-0060 },   // 5.0e-0003
        { 0x1.89374BC6A7EF9p-0008,  0x0.DB22D0E5604188p-0060 },   // 6.0e-0003
        { 0x1.CAC083126E978p-0008,  0x0.D4FDF3B645A1C8p-0060 },   // 7.0e-0003
        { 0x1.0624DD2F1A9FBp-0007,  0x0.E76C8B43958100p-0059 },   // 8.0e-0003
        { 0x1.26E978D4FDF3Bp-0007,  0x0.645A1CAC083120p-0059 },   // 9.0e-0003
        { 0x1.47AE147AE147Ap-0007,  0x0.E147AE147AE140p-0059 },   // 1.0e-0002
        { 0x1.47AE147AE147Ap-0006,  0x0.E147AE147AE140p-0058 },   // 2.0e-0002
        { 0x1.EB851EB851EB8p-0006,  0x0.51EB851EB851E8p-0058 },   // 3.0e-0002
        { 0x1.47AE147AE147Ap-0005,  0x0.E147AE147AE140p-0057 },   // 4.0e-0002
        { 0x1.9999999999999p-0005,  0x0.99999999999998p-0057 },   // 5.0e-0002
        { 0x1.EB851EB851EB8p-0005,  0x0.51EB851EB851E8p-0057 },   // 6.0e-0002
        { 0x1.1EB851EB851EBp-0004,  0x0.851EB851EB8518p-0056 },   // 7.0e-0002
        { 0x1.47AE147AE147Ap-0004,  0x0.E147AE147AE140p-0056 },   // 8.0e-0002
        { 0x1.70A3D70A3D70Ap-0004,  0x0.3D70A3D70A3D70p-0056 },   // 9.0e-0002
        { 0x1.9999999999999p-0004,  0x0.99999999999998p-0056 },   // 1.0e-0001
        { 0x1.9999999999999p-0003,  0x0.99999999999998p-0055 },   // 2.0e-0001
        { 0x1.3333333333333p-0002,  0x0.33333333333330p-0054 },   // 3.0e-0001
        { 0x1.9999999999999p-0002,  0x0.99999999999998p-0054 },   // 4.0e-0001
        { 0x1.0000000000000p-0001,  0x0.00000000000000p-0053 },   // 5.0e-0001
        { 0x1.3333333333333p-0001,  0x0.33333333333330p-0053 },   // 6.0e-0001
        { 0x1.6666666666666p-0001,  0x0.66666666666660p-0053 },   // 7.0e-0001
        { 0x1.9999999999999p-0001,  0x0.99999999999998p-0053 },   // 8.0e-0001
        { 0x1.CCCCCCCCCCCCCp-0001,  0x0.CCCCCCCCCCCCC8p-0053 },   // 9.0e-0001
        { 0x1.0000000000000p+0000,  0x0.00000000000000p-0052 },   // 1.0e+0000
        { 0x1.0000000000000p+0001,  0x0.00000000000000p-0051 },   // 2.0e+0000
        { 0x1.8000000000000p+0001,  0x0.00000000000000p-0051 },   // 3.0e+0000
        { 0x1.0000000000000p+0002,  0x0.00000000000000p-0050 },   // 4.0e+0000
        { 0x1.4000000000000p+0002,  0x0.00000000000000p-0050 },   // 5.0e+0000
        { 0x1.8000000000000p+0002,  0x0.00000000000000p-0050 },   // 6.0e+0000
        { 0x1.C000000000000p+0002,  0x0.00000000000000p-0050 },   // 7.0e+0000
        { 0x1.0000000000000p+0003,  0x0.00000000000000p-0049 },   // 8.0e+0000
        { 0x1.2000000000000p+0003,  0x0.00000000000000p-0049 },   // 9.0e+0000
        { 0x1.4000000000000p+0003,  0x0.00000000000000p-0049 },   // 1.0e+0001
        { 0x1.4000000000000p+0004,  0x0.00000000000000p-0048 },   // 2.0e+0001
        { 0x1.E000000000000p+0004,  0x0.00000000000000p-0048 },   // 3.0e+0001
        { 0x1.4000000000000p+0005,  0x0.00000000000000p-0047 },   // 4.0e+0001
        { 0x1.9000000000000p+0005,  0x0.00000000000000p-0047 },   // 5.0e+0001
        { 0x1.E000000000000p+0005,  0x0.00000000000000p-0047 },   // 6.0e+0001
        { 0x1.1800000000000p+0006,  0x0.00000000000000p-0046 },   // 7.0e+0001
        { 0x1.4000000000000p+0006,  0x0.00000000000000p-0046 },   // 8.0e+0001
        { 0x1.6800000000000p+0006,  0x0.00000000000000p-0046 },   // 9.0e+0001
        { 0x1.9000000000000p+0006,  0x0.00000000000000p-0046 },   // 1.0e+0002
        { 0x1.9000000000000p+0007,  0x0.00000000000000p-0045 },   // 2.0e+0002
        { 0x1.2C00000000000p+0008,  0x0.00000000000000p-0044 },   // 3.0e+0002
        { 0x1.9000000000000p+0008,  0x0.00000000000000p-0044 },   // 4.0e+0002
        { 0x1.F400000000000p+0008,  0x0.00000000000000p-0044 },   // 5.0e+0002
        { 0x1.2C00000000000p+0009,  0x0.00000000000000p-0043 },   // 6.0e+0002
        { 0x1.5E00000000000p+0009,  0x0.00000000000000p-0043 },   // 7.0e+0002
        { 0x1.9000000000000p+0009,  0x0.00000000000000p-0043 },   // 8.0e+0002
        { 0x1.C200000000000p+0009,  0x0.00000000000000p-0043 },   // 9.0e+0002
        { 0x1.F400000000000p+0009,  0x0.00000000000000p-0043 },   // 1.0e+0003
        { 0x1.F400000000000p+0010,  0x0.00000000000000p-0042 },   // 2.0e+0003
        { 0x1.7700000000000p+0011,  0x0.00000000000000p-0041 },   // 3.0e+0003
        { 0x1.F400000000000p+0011,  0x0.00000000000000p-0041 },   // 4.0e+0003
        { 0x1.3880000000000p+0012,  0x0.00000000000000p-0040 },   // 5.0e+0003
        { 0x1.7700000000000p+0012,  0x0.00000000000000p-0040 },   // 6.0e+0003
        { 0x1.B580000000000p+0012,  0x0.00000000000000p-0040 },   // 7.0e+0003
        { 0x1.F400000000000p+0012,  0x0.00000000000000p-0040 },   // 8.0e+0003
        { 0x1.1940000000000p+0013,  0x0.00000000000000p-0039 },   // 9.0e+0003
        { 0x1.3880000000000p+0013,  0x0.00000000000000p-0039 },   // 1.0e+0004
        { 0x1.3880000000000p+0014,  0x0.00000000000000p-0038 },   // 2.0e+0004
        { 0x1.D4C0000000000p+0014,  0x0.00000000000000p-0038 },   // 3.0e+0004
        { 0x1.3880000000000p+0015,  0x0.00000000000000p-0037 },   // 4.0e+0004
        { 0x1.86A0000000000p+0015,  0x0.00000000000000p-0037 },   // 5.0e+0004
        { 0x1.D4C0000000000p+0015,  0x0.00000000000000p-0037 },   // 6.0e+0004
        { 0x1.1170000000000p+0016,  0x0.00000000000000p-0036 },   // 7.0e+0004
        { 0x1.3880000000000p+0016,  0x0.00000000000000p-0036 },   // 8.0e+0004
        { 0x1.5F90000000000p+0016,  0x0.00000000000000p-0036 },   // 9.0e+0004
        { 0x1.86A0000000000p+0016,  0x0.00000000000000p-0036 },   // 1.0e+0005
        { 0x1.86A0000000000p+0017,  0x0.00000000000000p-0035 },   // 2.0e+0005
        { 0x1.24F8000000000p+0018,  0x0.00000000000000p-0034 },   // 3.0e+0005
        { 0x1.86A0000000000p+0018,  0x0.00000000000000p-0034 },   // 4.0e+0005
        { 0x1.E848000000000p+0018,  0x0.00000000000000p-0034 },   // 5.0e+0005
        { 0x1.24F8000000000p+0019,  0x0.00000000000000p-0033 },   // 6.0e+0005
        { 0x1.55CC000000000p+0019,  0x0.00000000000000p-0033 },   // 7.0e+0005
        { 0x1.86A0000000000p+0019,  0x0.00000000000000p-0033 },   // 8.0e+0005
        { 0x1.B774000000000p+0019,  0x0.00000000000000p-0033 },   // 9.0e+0005
        { 0x1.E848000000000p+0019,  0x0.00000000000000p-0033 },   // 1.0e+0006
        { 0x1.E848000000000p+0020,  0x0.00000000000000p-0032 },   // 2.0e+0006
        { 0x1.6E36000000000p+0021,  0x0.00000000000000p-0031 },   // 3.0e+0006
        { 0x1.E848000000000p+0021,  0x0.00000000000000p-0031 },   // 4.0e+0006
        { 0x1.312D000000000p+0022,  0x0.00000000000000p-0030 },   // 5.0e+0006
        { 0x1.6E36000000000p+0022,  0x0.00000000000000p-0030 },   // 6.0e+0006
        { 0x1.AB3F000000000p+0022,  0x0.00000000000000p-0030 },   // 7.0e+0006
        { 0x1.E848000000000p+0022,  0x0.00000000000000p-0030 },   // 8.0e+0006
        { 0x1.12A8800000000p+0023,  0x0.00000000000000p-0029 },   // 9.0e+0006
        { 0x1.312D000000000p+0023,  0x0.00000000000000p-0029 },   // 1.0e+0007
        { 0x1.312D000000000p+0024,  0x0.00000000000000p-0028 },   // 2.0e+0007
        { 0x1.C9C3800000000p+0024,  0x0.00000000000000p-0028 },   // 3.0e+0007
        { 0x1.312D000000000p+0025,  0x0.00000000000000p-0027 },   // 4.0e+0007
        { 0x1.7D78400000000p+0025,  0x0.00000000000000p-0027 },   // 5.0e+0007
        { 0x1.C9C3800000000p+0025,  0x0.00000000000000p-0027 },   // 6.0e+0007
        { 0x1.0B07600000000p+0026,  0x0.00000000000000p-0026 },   // 7.0e+0007
        { 0x1.312D000000000p+0026,  0x0.00000000000000p-0026 },   // 8.0e+0007
        { 0x1.5752A00000000p+0026,  0x0.00000000000000p-0026 },   // 9.0e+0007
        { 0x1.7D78400000000p+0026,  0x0.00000000000000p-0026 },   // 1.0e+0008
        { 0x1.7D78400000000p+0027,  0x0.00000000000000p-0025 },   // 2.0e+0008
        { 0x1.1E1A300000000p+0028,  0x0.00000000000000p-0024 },   // 3.0e+0008
        { 0x1.7D78400000000p+0028,  0x0.00000000000000p-0024 },   // 4.0e+0008
        { 0x1.DCD6500000000p+0028,  0x0.00000000000000p-0024 },   // 5.0e+0008
        { 0x1.1E1A300000000p+0029,  0x0.00000000000000p-0023 },   // 6.0e+0008
        { 0x1.4DC9380000000p+0029,  0x0.00000000000000p-0023 },   // 7.0e+0008
        { 0x1.7D78400000000p+0029,  0x0.00000000000000p-0023 },   // 8.0e+0008
        { 0x1.AD27480000000p+0029,  0x0.00000000000000p-0023 },   // 9.0e+0008
        { 0x1.DCD6500000000p+0029,  0x0.00000000000000p-0023 },   // 1.0e+0009
        { 0x1.DCD6500000000p+0030,  0x0.00000000000000p-0022 },   // 2.0e+0009
        { 0x1.65A0BC0000000p+0031,  0x0.00000000000000p-0021 },   // 3.0e+0009
        { 0x1.DCD6500000000p+0031,  0x0.00000000000000p-0021 },   // 4.0e+0009
        { 0x1.2A05F20000000p+0032,  0x0.00000000000000p-0020 },   // 5.0e+0009
        { 0x1.65A0BC0000000p+0032,  0x0.00000000000000p-0020 },   // 6.0e+0009
        { 0x1.A13B860000000p+0032,  0x0.00000000000000p-0020 },   // 7.0e+0009
        { 0x1.DCD6500000000p+0032,  0x0.00000000000000p-0020 },   // 8.0e+0009
        { 0x1.0C388D0000000p+0033,  0x0.00000000000000p-0019 },   // 9.0e+0009
        { 0x1.2A05F20000000p+0033,  0x0.00000000000000p-0019 },   // 1.0e+0010
        { 0x1.2A05F20000000p+0034,  0x0.00000000000000p-0018 },   // 2.0e+0010
        { 0x1.BF08EB0000000p+0034,  0x0.00000000000000p-0018 },   // 3.0e+0010
        { 0x1.2A05F20000000p+0035,  0x0.00000000000000p-0017 },   // 4.0e+0010
        { 0x1.74876E8000000p+0035,  0x0.00000000000000p-0017 },   // 5.0e+0010
        { 0x1.BF08EB0000000p+0035,  0x0.00000000000000p-0017 },   // 6.0e+0010
        { 0x1.04C533C000000p+0036,  0x0.00000000000000p-0016 },   // 7.0e+0010
        { 0x1.2A05F20000000p+0036,  0x0.00000000000000p-0016 },   // 8.0e+0010
        { 0x1.4F46B04000000p+0036,  0x0.00000000000000p-0016 },   // 9.0e+0010
        { 0x1.74876E8000000p+0036,  0x0.00000000000000p-0016 },   // 1.0e+0011
        { 0x1.74876E8000000p+0037,  0x0.00000000000000p-0015 },   // 2.0e+0011
        { 0x1.176592E000000p+0038,  0x0.00000000000000p-0014 },   // 3.0e+0011
        { 0x1.74876E8000000p+0038,  0x0.00000000000000p-0014 },   // 4.0e+0011
        { 0x1.D1A94A2000000p+0038,  0x0.00000000000000p-0014 },   // 5.0e+0011
        { 0x1.176592E000000p+0039,  0x0.00000000000000p-0013 },   // 6.0e+0011
        { 0x1.45F680B000000p+0039,  0x0.00000000000000p-0013 },   // 7.0e+0011
        { 0x1.74876E8000000p+0039,  0x0.00000000000000p-0013 },   // 8.0e+0011
        { 0x1.A3185C5000000p+0039,  0x0.00000000000000p-0013 },   // 9.0e+0011
        { 0x1.D1A94A2000000p+0039,  0x0.00000000000000p-0013 },   // 1.0e+0012
        { 0x1.D1A94A2000000p+0040,  0x0.00000000000000p-0012 },   // 2.0e+0012
        { 0x1.5D3EF79800000p+0041,  0x0.00000000000000p-0011 },   // 3.0e+0012
        { 0x1.D1A94A2000000p+0041,  0x0.00000000000000p-0011 },   // 4.0e+0012
        { 0x1.2309CE5400000p+0042,  0x0.00000000000000p-0010 },   // 5.0e+0012
        { 0x1.5D3EF79800000p+0042,  0x0.00000000000000p-0010 },   // 6.0e+0012
        { 0x1.977420DC00000p+0042,  0x0.00000000000000p-0010 },   // 7.0e+0012
        { 0x1.D1A94A2000000p+0042,  0x0.00000000000000p-0010 },   // 8.0e+0012
        { 0x1.05EF39B200000p+0043,  0x0.00000000000000p-0009 },   // 9.0e+0012
        { 0x1.2309CE5400000p+0043,  0x0.00000000000000p-0009 },   // 1.0e+0013
        { 0x1.2309CE5400000p+0044,  0x0.00000000000000p-0008 },   // 2.0e+0013
        { 0x1.B48EB57E00000p+0044,  0x0.00000000000000p-0008 },   // 3.0e+0013
        { 0x1.2309CE5400000p+0045,  0x0.00000000000000p-0007 },   // 4.0e+0013
        { 0x1.6BCC41E900000p+0045,  0x0.00000000000000p-0007 },   // 5.0e+0013
        { 0x1.B48EB57E00000p+0045,  0x0.00000000000000p-0007 },   // 6.0e+0013
        { 0x1.FD51291300000p+0045,  0x0.00000000000000p-0007 },   // 7.0e+0013
        { 0x1.2309CE5400000p+0046,  0x0.00000000000000p-0006 },   // 8.0e+0013
        { 0x1.476B081E80000p+0046,  0x0.00000000000000p-0006 },   // 9.0e+0013
        { 0x1.6BCC41E900000p+0046,  0x0.00000000000000p-0006 },   // 1.0e+0014
        { 0x1.6BCC41E900000p+0047,  0x0.00000000000000p-0005 },   // 2.0e+0014
        { 0x1.10D9316EC0000p+0048,  0x0.00000000000000p-0004 },   // 3.0e+0014
        { 0x1.6BCC41E900000p+0048,  0x0.00000000000000p-0004 },   // 4.0e+0014
        { 0x1.C6BF526340000p+0048,  0x0.00000000000000p-0004 },   // 5.0e+0014
        { 0x1.10D9316EC0000p+0049,  0x0.00000000000000p-0003 },   // 6.0e+0014
        { 0x1.3E52B9ABE0000p+0049,  0x0.00000000000000p-0003 },   // 7.0e+0014
        { 0x1.6BCC41E900000p+0049,  0x0.00000000000000p-0003 },   // 8.0e+0014
        { 0x1.9945CA2620000p+0049,  0x0.00000000000000p-0003 },   // 9.0e+0014
        { 0x1.C6BF526340000p+0049,  0x0.00000000000000p-0003 },   // 1.0e+0015
        { 0x1.C6BF526340000p+0050,  0x0.00000000000000p-0002 },   // 2.0e+0015
        { 0x1.550F7DCA70000p+0051,  0x0.00000000000000p-0001 },   // 3.0e+0015
        { 0x1.C6BF526340000p+0051,  0x0.00000000000000p-0001 },   // 4.0e+0015
        { 0x1.1C37937E08000p+0052,  0x0.00000000000000p+0000 },   // 5.0e+0015
        { 0x1.550F7DCA70000p+0052,  0x0.00000000000000p+0000 },   // 6.0e+0015
        { 0x1.8DE76816D8000p+0052,  0x0.00000000000000p+0000 },   // 7.0e+0015
        { 0x1.C6BF526340000p+0052,  0x0.00000000000000p+0000 },   // 8.0e+0015
        { 0x1.FF973CAFA8000p+0052,  0x0.00000000000000p+0000 },   // 9.0e+0015
        { 0x1.1C37937E08000p+0053,  0x0.00000000000000p+0001 },   // 1.0e+0016
        { 0x1.1C37937E08000p+0054,  0x0.00000000000000p+0002 },   // 2.0e+0016
        { 0x1.AA535D3D0C000p+0054,  0x0.00000000000000p+0002 },   // 3.0e+0016
        { 0x1.1C37937E08000p+0055,  0x0.00000000000000p+0003 },   // 4.0e+0016
        { 0x1.6345785D8A000p+0055,  0x0.00000000000000p+0003 },   // 5.0e+0016
        { 0x1.AA535D3D0C000p+0055,  0x0.00000000000000p+0003 },   // 6.0e+0016
        { 0x1.F161421C8E000p+0055,  0x0.00000000000000p+0003 },   // 7.0e+0016
        { 0x1.1C37937E08000p+0056,  0x0.00000000000000p+0004 },   // 8.0e+0016
        { 0x1.3FBE85EDC9000p+0056,  0x0.00000000000000p+0004 },   // 9.0e+0016
        { 0x1.6345785D8A000p+0056,  0x0.00000000000000p+0004 },   // 1.0e+0017
        { 0x1.6345785D8A000p+0057,  0x0.00000000000000p+0005 },   // 2.0e+0017
        { 0x1.0A741A4627800p+0058,  0x0.00000000000000p+0006 },   // 3.0e+0017
        { 0x1.6345785D8A000p+0058,  0x0.00000000000000p+0006 },   // 4.0e+0017
        { 0x1.BC16D674EC800p+0058,  0x0.00000000000000p+0006 },   // 5.0e+0017
        { 0x1.0A741A4627800p+0059,  0x0.00000000000000p+0007 },   // 6.0e+0017
        { 0x1.36DCC951D8C00p+0059,  0x0.00000000000000p+0007 },   // 7.0e+0017
        { 0x1.6345785D8A000p+0059,  0x0.00000000000000p+0007 },   // 8.0e+0017
        { 0x1.8FAE27693B400p+0059,  0x0.00000000000000p+0007 },   // 9.0e+0017
        { 0x1.BC16D674EC800p+0059,  0x0.00000000000000p+0007 },   // 1.0e+0018
        { 0x1.BC16D674EC800p+0060,  0x0.00000000000000p+0008 },   // 2.0e+0018
        { 0x1.4D1120D7B1600p+0061,  0x0.00000000000000p+0009 },   // 3.0e+0018
        { 0x1.BC16D674EC800p+0061,  0x0.00000000000000p+0009 },   // 4.0e+0018
        { 0x1.158E460913D00p+0062,  0x0.00000000000000p+0010 },   // 5.0e+0018
        { 0x1.4D1120D7B1600p+0062,  0x0.00000000000000p+0010 },   // 6.0e+0018
        { 0x1.8493FBA64EF00p+0062,  0x0.00000000000000p+0010 },   // 7.0e+0018
        { 0x1.BC16D674EC800p+0062,  0x0.00000000000000p+0010 },   // 8.0e+0018
        { 0x1.F399B1438A100p+0062,  0x0.00000000000000p+0010 },   // 9.0e+0018
        { 0x1.158E460913D00p+0063,  0x0.00000000000000p+0011 },   // 1.0e+0019
        { 0x1.158E460913D00p+0064,  0x0.00000000000000p+0012 },   // 2.0e+0019
        { 0x1.A055690D9DB80p+0064,  0x0.00000000000000p+0012 },   // 3.0e+0019
        { 0x1.158E460913D00p+0065,  0x0.00000000000000p+0013 },   // 4.0e+0019
        { 0x1.5AF1D78B58C40p+0065,  0x0.00000000000000p+0013 },   // 5.0e+0019
        { 0x1.A055690D9DB80p+0065,  0x0.00000000000000p+0013 },   // 6.0e+0019
        { 0x1.E5B8FA8FE2AC0p+0065,  0x0.00000000000000p+0013 },   // 7.0e+0019
        { 0x1.158E460913D00p+0066,  0x0.00000000000000p+0014 },   // 8.0e+0019
        { 0x1.38400ECA364A0p+0066,  0x0.00000000000000p+0014 },   // 9.0e+0019
        { 0x1.5AF1D78B58C40p+0066,  0x0.00000000000000p+0014 },   // 1.0e+0020
        { 0x1.5AF1D78B58C40p+0067,  0x0.00000000000000p+0015 },   // 2.0e+0020
        { 0x1.043561A882930p+0068,  0x0.00000000000000p+0016 },   // 3.0e+0020
        { 0x1.5AF1D78B58C40p+0068,  0x0.00000000000000p+0016 },   // 4.0e+0020
        { 0x1.B1AE4D6E2EF50p+0068,  0x0.00000000000000p+0016 },   // 5.0e+0020
        { 0x1.043561A882930p+0069,  0x0.00000000000000p+0017 },   // 6.0e+0020
        { 0x1.2F939C99EDAB8p+0069,  0x0.00000000000000p+0017 },   // 7.0e+0020
        { 0x1.5AF1D78B58C40p+0069,  0x0.00000000000000p+0017 },   // 8.0e+0020
        { 0x1.8650127CC3DC8p+0069,  0x0.00000000000000p+0017 },   // 9.0e+0020
        { 0x1.B1AE4D6E2EF50p+0069,  0x0.00000000000000p+0017 },   // 1.0e+0021
        { 0x1.B1AE4D6E2EF50p+0070,  0x0.00000000000000p+0018 },   // 2.0e+0021
        { 0x1.4542BA12A337Cp+0071,  0x0.00000000000000p+0019 },   // 3.0e+0021
        { 0x1.B1AE4D6E2EF50p+0071,  0x0.00000000000000p+0019 },   // 4.0e+0021
        { 0x1.0F0CF064DD592p+0072,  0x0.00000000000000p+0020 },   // 5.0e+0021
        { 0x1.4542BA12A337Cp+0072,  0x0.00000000000000p+0020 },   // 6.0e+0021
        { 0x1.7B7883C069166p+0072,  0x0.00000000000000p+0020 },   // 7.0e+0021
        { 0x1.B1AE4D6E2EF50p+0072,  0x0.00000000000000p+0020 },   // 8.0e+0021
        { 0x1.E7E4171BF4D3Ap+0072,  0x0.00000000000000p+0020 },   // 9.0e+0021
        { 0x1.0F0CF064DD592p+0073,  0x0.00000000000000p+0021 },   // 1.0e+0022
        { 0x1.0F0CF064DD592p+0074,  0x0.00000000000000p+0022 },   // 2.0e+0022
        { 0x1.969368974C05Bp+0074,  0x0.00000000000000p+0022 },   // 3.0e+0022
        { 0x1.0F0CF064DD592p+0075,  0x0.00000000000000p+0023 },   // 4.0e+0022
        { 0x1.52D02C7E14AF6p+0075,  0x0.80000000000000p+0023 },   // 5.0e+0022
        { 0x1.969368974C05Bp+0075,  0x0.00000000000000p+0023 },   // 6.0e+0022
        { 0x1.DA56A4B0835BFp+0075,  0x0.80000000000000p+0023 },   // 7.0e+0022
        { 0x1.0F0CF064DD592p+0076,  0x0.00000000000000p+0024 },   // 8.0e+0022
        { 0x1.30EE8E7179044p+0076,  0x0.40000000000000p+0024 },   // 9.0e+0022
        { 0x1.52D02C7E14AF6p+0076,  0x0.80000000000000p+0024 },   // 1.0e+0023
        { 0x1.52D02C7E14AF6p+0077,  0x0.80000000000000p+0025 },   // 2.0e+0023
        { 0x1.FC3842BD1F071p+0077,  0x0.C0000000000000p+0025 },   // 3.0e+0023
        { 0x1.52D02C7E14AF6p+0078,  0x0.80000000000000p+0026 },   // 4.0e+0023
        { 0x1.A784379D99DB4p+0078,  0x0.20000000000000p+0026 },   // 5.0e+0023
        { 0x1.FC3842BD1F071p+0078,  0x0.C0000000000000p+0026 },   // 6.0e+0023
        { 0x1.287626EE52197p+0079,  0x0.B0000000000000p+0027 },   // 7.0e+0023
        { 0x1.52D02C7E14AF6p+0079,  0x0.80000000000000p+0027 },   // 8.0e+0023
        { 0x1.7D2A320DD7455p+0079,  0x0.50000000000000p+0027 },   // 9.0e+0023
        { 0x1.A784379D99DB4p+0079,  0x0.20000000000000p+0027 },   // 1.0e+0024
        { 0x1.A784379D99DB4p+0080,  0x0.20000000000000p+0028 },   // 2.0e+0024
        { 0x1.3DA329B633647p+0081,  0x0.18000000000000p+0029 },   // 3.0e+0024
        { 0x1.A784379D99DB4p+0081,  0x0.20000000000000p+0029 },   // 4.0e+0024
        { 0x1.08B2A2C280290p+0082,  0x0.94000000000000p+0030 },   // 5.0e+0024
        { 0x1.3DA329B633647p+0082,  0x0.18000000000000p+0030 },   // 6.0e+0024
        { 0x1.7293B0A9E69FDp+0082,  0x0.9C000000000000p+0030 },   // 7.0e+0024
        { 0x1.A784379D99DB4p+0082,  0x0.20000000000000p+0030 },   // 8.0e+0024
        { 0x1.DC74BE914D16Ap+0082,  0x0.A4000000000000p+0030 },   // 9.0e+0024
        { 0x1.08B2A2C280290p+0083,  0x0.94000000000000p+0031 },   // 1.0e+0025
        { 0x1.08B2A2C280290p+0084,  0x0.94000000000000p+0032 },   // 2.0e+0025
        { 0x1.8D0BF423C03D8p+0084,  0x0.DE000000000000p+0032 },   // 3.0e+0025
        { 0x1.08B2A2C280290p+0085,  0x0.94000000000000p+0033 },   // 4.0e+0025
        { 0x1.4ADF4B7320334p+0085,  0x0.B9000000000000p+0033 },   // 5.0e+0025
        { 0x1.8D0BF423C03D8p+0085,  0x0.DE000000000000p+0033 },   // 6.0e+0025
        { 0x1.CF389CD46047Dp+0085,  0x0.03000000000000p+0033 },   // 7.0e+0025
        { 0x1.08B2A2C280290p+0086,  0x0.94000000000000p+0034 },   // 8.0e+0025
        { 0x1.29C8F71AD02E2p+0086,  0x0.A6800000000000p+0034 },   // 9.0e+0025
        { 0x1.4ADF4B7320334p+0086,  0x0.B9000000000000p+0034 },   // 1.0e+0026
        { 0x1.4ADF4B7320334p+0087,  0x0.B9000000000000p+0035 },   // 2.0e+0026
        { 0x1.F04EF12CB04CFp+0087,  0x0.15800000000000p+0035 },   // 3.0e+0026
        { 0x1.4ADF4B7320334p+0088,  0x0.B9000000000000p+0036 },   // 4.0e+0026
        { 0x1.9D971E4FE8401p+0088,  0x0.E7400000000000p+0036 },   // 5.0e+0026
        { 0x1.F04EF12CB04CFp+0088,  0x0.15800000000000p+0036 },   // 6.0e+0026
        { 0x1.21836204BC2CEp+0089,  0x0.21E00000000000p+0037 },   // 7.0e+0026
        { 0x1.4ADF4B7320334p+0089,  0x0.B9000000000000p+0037 },   // 8.0e+0026
        { 0x1.743B34E18439Bp+0089,  0x0.50200000000000p+0037 },   // 9.0e+0026
        { 0x1.9D971E4FE8401p+0089,  0x0.E7400000000000p+0037 },   // 1.0e+0027
        { 0x1.9D971E4FE8401p+0090,  0x0.E7400000000000p+0038 },   // 2.0e+0027
        { 0x1.363156BBEE301p+0091,  0x0.6D700000000000p+0039 },   // 3.0e+0027
        { 0x1.9D971E4FE8401p+0091,  0x0.E7400000000000p+0039 },   // 4.0e+0027
        { 0x1.027E72F1F1281p+0092,  0x0.30880000000000p+0040 },   // 5.0e+0027
        { 0x1.363156BBEE301p+0092,  0x0.6D700000000000p+0040 },   // 6.0e+0027
        { 0x1.69E43A85EB381p+0092,  0x0.AA580000000000p+0040 },   // 7.0e+0027
        { 0x1.9D971E4FE8401p+0092,  0x0.E7400000000000p+0040 },   // 8.0e+0027
        { 0x1.D14A0219E5482p+0092,  0x0.24280000000000p+0040 },   // 9.0e+0027
        { 0x1.027E72F1F1281p+0093,  0x0.30880000000000p+0041 },   // 1.0e+0028
        { 0x1.027E72F1F1281p+0094,  0x0.30880000000000p+0042 },   // 2.0e+0028
        { 0x1.83BDAC6AE9BC1p+0094,  0x0.C8CC0000000000p+0042 },   // 3.0e+0028
        { 0x1.027E72F1F1281p+0095,  0x0.30880000000000p+0043 },   // 4.0e+0028
        { 0x1.431E0FAE6D721p+0095,  0x0.7CAA0000000000p+0043 },   // 5.0e+0028
        { 0x1.83BDAC6AE9BC1p+0095,  0x0.C8CC0000000000p+0043 },   // 6.0e+0028
        { 0x1.C45D492766062p+0095,  0x0.14EE0000000000p+0043 },   // 7.0e+0028
        { 0x1.027E72F1F1281p+0096,  0x0.30880000000000p+0044 },   // 8.0e+0028
        { 0x1.22CE41502F4D1p+0096,  0x0.56990000000000p+0044 },   // 9.0e+0028
        { 0x1.431E0FAE6D721p+0096,  0x0.7CAA0000000000p+0044 },   // 1.0e+0029
        { 0x1.431E0FAE6D721p+0097,  0x0.7CAA0000000000p+0045 },   // 2.0e+0029
        { 0x1.E4AD1785A42B2p+0097,  0x0.3AFF0000000000p+0045 },   // 3.0e+0029
        { 0x1.431E0FAE6D721p+0098,  0x0.7CAA0000000000p+0046 },   // 4.0e+0029
        { 0x1.93E5939A08CE9p+0098,  0x0.DBD48000000000p+0046 },   // 5.0e+0029
        { 0x1.E4AD1785A42B2p+0098,  0x0.3AFF0000000000p+0046 },   // 6.0e+0029
        { 0x1.1ABA4DB89FC3Dp+0099,  0x0.4D14C000000000p+0047 },   // 7.0e+0029
        { 0x1.431E0FAE6D721p+0099,  0x0.7CAA0000000000p+0047 },   // 8.0e+0029
        { 0x1.6B81D1A43B205p+0099,  0x0.AC3F4000000000p+0047 },   // 9.0e+0029
        { 0x1.93E5939A08CE9p+0099,  0x0.DBD48000000000p+0047 },   // 1.0e+0030
        { 0x1.93E5939A08CE9p+0100,  0x0.DBD48000000000p+0048 },   // 2.0e+0030
        { 0x1.2EEC2EB3869AFp+0101,  0x0.64DF6000000000p+0049 },   // 3.0e+0030
        { 0x1.93E5939A08CE9p+0101,  0x0.DBD48000000000p+0049 },   // 4.0e+0030
        { 0x1.F8DEF8808B024p+0101,  0x0.52C9A000000000p+0049 },   // 5.0e+0030
        { 0x1.2EEC2EB3869AFp+0102,  0x0.64DF6000000000p+0050 },   // 6.0e+0030
        { 0x1.6168E126C7B4Cp+0102,  0x0.A059F000000000p+0050 },   // 7.0e+0030
        { 0x1.93E5939A08CE9p+0102,  0x0.DBD48000000000p+0050 },   // 8.0e+0030
        { 0x1.C662460D49E87p+0102,  0x0.174F1000000000p+0050 },   // 9.0e+0030
        { 0x1.F8DEF8808B024p+0102,  0x0.52C9A000000000p+0050 },   // 1.0e+0031
        { 0x1.F8DEF8808B024p+0103,  0x0.52C9A000000000p+0051 },   // 2.0e+0031
        { 0x1.7AA73A606841Bp+0104,  0x0.3E173800000000p+0052 },   // 3.0e+0031
        { 0x1.F8DEF8808B024p+0104,  0x0.52C9A000000000p+0052 },   // 4.0e+0031
        { 0x1.3B8B5B5056E16p+0105,  0x0.B3BE0400000000p+0053 },   // 5.0e+0031
        { 0x1.7AA73A606841Bp+0105,  0x0.3E173800000000p+0053 },   // 6.0e+0031
        { 0x1.B9C3197079A1Fp+0105,  0x0.C8706C00000000p+0053 },   // 7.0e+0031
        { 0x1.F8DEF8808B024p+0105,  0x0.52C9A000000000p+0053 },   // 8.0e+0031
        { 0x1.1BFD6BC84E314p+0106,  0x0.6E916A00000000p+0054 },   // 9.0e+0031
        { 0x1.3B8B5B5056E16p+0106,  0x0.B3BE0400000000p+0054 },   // 1.0e+0032
        { 0x1.3B8B5B5056E16p+0107,  0x0.B3BE0400000000p+0055 },   // 2.0e+0032
        { 0x1.D95108F882522p+0107,  0x0.0D9D0600000000p+0055 },   // 3.0e+0032
        { 0x1.3B8B5B5056E16p+0108,  0x0.B3BE0400000000p+0056 },   // 4.0e+0032
        { 0x1.8A6E32246C99Cp+0108,  0x0.60AD8500000000p+0056 },   // 5.0e+0032
        { 0x1.D95108F882522p+0108,  0x0.0D9D0600000000p+0056 },   // 6.0e+0032
        { 0x1.1419EFE64C053p+0109,  0x0.DD464380000000p+0057 },   // 7.0e+0032
        { 0x1.3B8B5B5056E16p+0109,  0x0.B3BE0400000000p+0057 },   // 8.0e+0032
        { 0x1.62FCC6BA61BD9p+0109,  0x0.8A35C480000000p+0057 },   // 9.0e+0032
        { 0x1.8A6E32246C99Cp+0109,  0x0.60AD8500000000p+0057 },   // 1.0e+0033
        { 0x1.8A6E32246C99Cp+0110,  0x0.60AD8500000000p+0058 },   // 2.0e+0033
        { 0x1.27D2A59B51735p+0111,  0x0.488223C0000000p+0059 },   // 3.0e+0033
        { 0x1.8A6E32246C99Cp+0111,  0x0.60AD8500000000p+0059 },   // 4.0e+0033
        { 0x1.ED09BEAD87C03p+0111,  0x0.78D8E640000000p+0059 },   // 5.0e+0033
        { 0x1.27D2A59B51735p+0112,  0x0.488223C0000000p+0060 },   // 6.0e+0033
        { 0x1.59206BDFDF068p+0112,  0x0.D497D460000000p+0060 },   // 7.0e+0033
        { 0x1.8A6E32246C99Cp+0112,  0x0.60AD8500000000p+0060 },   // 8.0e+0033
        { 0x1.BBBBF868FA2CFp+0112,  0x0.ECC335A0000000p+0060 },   // 9.0e+0033
        { 0x1.ED09BEAD87C03p+0112,  0x0.78D8E640000000p+0060 },   // 1.0e+0034
        { 0x1.ED09BEAD87C03p+0113,  0x0.78D8E640000000p+0061 },   // 2.0e+0034
        { 0x1.71C74F0225D02p+0114,  0x0.9AA2ACB0000000p+0062 },   // 3.0e+0034
        { 0x1.ED09BEAD87C03p+0114,  0x0.78D8E640000000p+0062 },   // 4.0e+0034
        { 0x1.3426172C74D82p+0115,  0x0.2B878FE8000000p+0063 },   // 5.0e+0034
        { 0x1.71C74F0225D02p+0115,  0x0.9AA2ACB0000000p+0063 },   // 6.0e+0034
        { 0x1.AF6886D7D6C83p+0115,  0x0.09BDC978000000p+0063 },   // 7.0e+0034
        { 0x1.ED09BEAD87C03p+0115,  0x0.78D8E640000000p+0063 },   // 8.0e+0034
        { 0x1.15557B419C5C1p+0116,  0x0.F3FA0184000000p+0064 },   // 9.0e+0034
        { 0x1.3426172C74D82p+0116,  0x0.2B878FE8000000p+0064 },   // 1.0e+0035
        { 0x1.3426172C74D82p+0117,  0x0.2B878FE8000000p+0065 },   // 2.0e+0035
        { 0x1.CE3922C2AF443p+0117,  0x0.414B57DC000000p+0065 },   // 3.0e+0035
        { 0x1.3426172C74D82p+0118,  0x0.2B878FE8000000p+0066 },   // 4.0e+0035
        { 0x1.812F9CF7920E2p+0118,  0x0.B66973E2000000p+0066 },   // 5.0e+0035
        { 0x1.CE3922C2AF443p+0118,  0x0.414B57DC000000p+0066 },   // 6.0e+0035
        { 0x1.0DA15446E63D1p+0119,  0x0.E6169DEB000000p+0067 },   // 7.0e+0035
        { 0x1.3426172C74D82p+0119,  0x0.2B878FE8000000p+0067 },   // 8.0e+0035
        { 0x1.5AAADA1203732p+0119,  0x0.70F881E5000000p+0067 },   // 9.0e+0035
        { 0x1.812F9CF7920E2p+0119,  0x0.B66973E2000000p+0067 },   // 1.0e+0036
        { 0x1.812F9CF7920E2p+0120,  0x0.B66973E2000000p+0068 },   // 2.0e+0036
        { 0x1.20E3B5B9AD8AAp+0121,  0x0.08CF16E9800000p+0069 },   // 3.0e+0036
        { 0x1.812F9CF7920E2p+0121,  0x0.B66973E2000000p+0069 },   // 4.0e+0036
        { 0x1.E17B84357691Bp+0121,  0x0.6403D0DA800000p+0069 },   // 5.0e+0036
        { 0x1.20E3B5B9AD8AAp+0122,  0x0.08CF16E9800000p+0070 },   // 6.0e+0036
        { 0x1.5109A9589FCC6p+0122,  0x0.5F9C4565C00000p+0070 },   // 7.0e+0036
        { 0x1.812F9CF7920E2p+0122,  0x0.B66973E2000000p+0070 },   // 8.0e+0036
        { 0x1.B1559096844FFp+0122,  0x0.0D36A25E400000p+0070 },   // 9.0e+0036
        { 0x1.E17B84357691Bp+0122,  0x0.6403D0DA800000p+0070 },   // 1.0e+0037
        { 0x1.E17B84357691Bp+0123,  0x0.6403D0DA800000p+0071 },   // 2.0e+0037
        { 0x1.691CA32818ED4p+0124,  0x0.8B02DCA3E00000p+0072 },   // 3.0e+0037
        { 0x1.E17B84357691Bp+0124,  0x0.6403D0DA800000p+0072 },   // 4.0e+0037
        { 0x1.2CED32A16A1B1p+0125,  0x0.1E826288900000p+0073 },   // 5.0e+0037
        { 0x1.691CA32818ED4p+0125,  0x0.8B02DCA3E00000p+0073 },   // 6.0e+0037
        { 0x1.A54C13AEC7BF7p+0125,  0x0.F78356BF300000p+0073 },   // 7.0e+0037
        { 0x1.E17B84357691Bp+0125,  0x0.6403D0DA800000p+0073 },   // 8.0e+0037
        { 0x1.0ED57A5E12B1Fp+0126,  0x0.6842257AE80000p+0074 },   // 9.0e+0037
        { 0x1.2CED32A16A1B1p+0126,  0x0.1E826288900000p+0074 },   // 1.0e+0038
        { 0x1.2CED32A16A1B1p+0127,  0x0.1E826288900000p+0075 },   // 2.0e+0038
        { 0x1.C363CBF21F289p+0127,  0x0.ADC393CCD80000p+0075 },   // 3.0e+0038
        { 0x1.2CED32A16A1B1p+0128,  0x0.1E826288900000p+0076 },   // 4.0e+0038
        { 0x1.78287F49C4A1Dp+0128,  0x0.6622FB2AB40000p+0076 },   // 5.0e+0038
        { 0x1.C363CBF21F289p+0128,  0x0.ADC393CCD80000p+0076 },   // 6.0e+0038
        { 0x1.074F8C4D3CD7Ap+0129,  0x0.FAB216377E0000p+0077 },   // 7.0e+0038
        { 0x1.2CED32A16A1B1p+0129,  0x0.1E826288900000p+0077 },   // 8.0e+0038
        { 0x1.528AD8F5975E7p+0129,  0x0.4252AED9A20000p+0077 },   // 9.0e+0038
        { 0x1.78287F49C4A1Dp+0129,  0x0.6622FB2AB40000p+0077 },   // 1.0e+0039
        { 0x1.78287F49C4A1Dp+0130,  0x0.6622FB2AB40000p+0078 },   // 2.0e+0039
        { 0x1.1A1E5F7753796p+0131,  0x0.0C9A3C60070000p+0079 },   // 3.0e+0039
        { 0x1.78287F49C4A1Dp+0131,  0x0.6622FB2AB40000p+0079 },   // 4.0e+0039
        { 0x1.D6329F1C35CA4p+0131,  0x0.BFABB9F5610000p+0079 },   // 5.0e+0039
        { 0x1.1A1E5F7753796p+0132,  0x0.0C9A3C60070000p+0080 },   // 6.0e+0039
        { 0x1.49236F608C0D9p+0132,  0x0.B95E9BC55D8000p+0080 },   // 7.0e+0039
        { 0x1.78287F49C4A1Dp+0132,  0x0.6622FB2AB40000p+0080 },   // 8.0e+0039
        { 0x1.A72D8F32FD361p+0132,  0x0.12E75A900A8000p+0080 },   // 9.0e+0039
        { 0x1.D6329F1C35CA4p+0132,  0x0.BFABB9F5610000p+0080 },   // 1.0e+0040
        { 0x1.D6329F1C35CA4p+0133,  0x0.BFABB9F5610000p+0081 },   // 2.0e+0040
        { 0x1.60A5F7552857Bp+0134,  0x0.8FC0CB7808C000p+0082 },   // 3.0e+0040
        { 0x1.D6329F1C35CA4p+0134,  0x0.BFABB9F5610000p+0082 },   // 4.0e+0040
        { 0x1.25DFA371A19E6p+0135,  0x0.F7CB54395CA000p+0083 },   // 5.0e+0040
        { 0x1.60A5F7552857Bp+0135,  0x0.8FC0CB7808C000p+0083 },   // 6.0e+0040
        { 0x1.9B6C4B38AF110p+0135,  0x0.27B642B6B4E000p+0083 },   // 7.0e+0040
        { 0x1.D6329F1C35CA4p+0135,  0x0.BFABB9F5610000p+0083 },   // 8.0e+0040
        { 0x1.087C797FDE41Cp+0136,  0x0.ABD0989A069000p+0084 },   // 9.0e+0040
        { 0x1.25DFA371A19E6p+0136,  0x0.F7CB54395CA000p+0084 },   // 1.0e+0041
        { 0x1.25DFA371A19E6p+0137,  0x0.F7CB54395CA000p+0085 },   // 2.0e+0041
        { 0x1.B8CF752A726DAp+0137,  0x0.73B0FE560AF000p+0085 },   // 3.0e+0041
        { 0x1.25DFA371A19E6p+0138,  0x0.F7CB54395CA000p+0086 },   // 4.0e+0041
        { 0x1.6F578C4E0A060p+0138,  0x0.B5BE2947B3C800p+0086 },   // 5.0e+0041
        { 0x1.B8CF752A726DAp+0138,  0x0.73B0FE560AF000p+0086 },   // 6.0e+0041
        { 0x1.0123AF036D6AAp+0139,  0x0.18D1E9B2310C00p+0087 },   // 7.0e+0041
        { 0x1.25DFA371A19E6p+0139,  0x0.F7CB54395CA000p+0087 },   // 8.0e+0041
        { 0x1.4A9B97DFD5D23p+0139,  0x0.D6C4BEC0883400p+0087 },   // 9.0e+0041
        { 0x1.6F578C4E0A060p+0139,  0x0.B5BE2947B3C800p+0087 },   // 1.0e+0042
        { 0x1.6F578C4E0A060p+0140,  0x0.B5BE2947B3C800p+0088 },   // 2.0e+0042
        { 0x1.1381A93A87848p+0141,  0x0.884E9EF5C6D600p+0089 },   // 3.0e+0042
        { 0x1.6F578C4E0A060p+0141,  0x0.B5BE2947B3C800p+0089 },   // 4.0e+0042
        { 0x1.CB2D6F618C878p+0141,  0x0.E32DB399A0BA00p+0089 },   // 5.0e+0042
        { 0x1.1381A93A87848p+0142,  0x0.884E9EF5C6D600p+0090 },   // 6.0e+0042
        { 0x1.416C9AC448C54p+0142,  0x0.9F06641EBD4F00p+0090 },   // 7.0e+0042
        { 0x1.6F578C4E0A060p+0142,  0x0.B5BE2947B3C800p+0090 },   // 8.0e+0042
        { 0x1.9D427DD7CB46Cp+0142,  0x0.CC75EE70AA4100p+0090 },   // 9.0e+0042
        { 0x1.CB2D6F618C878p+0142,  0x0.E32DB399A0BA00p+0090 },   // 1.0e+0043
        { 0x1.CB2D6F618C878p+0143,  0x0.E32DB399A0BA00p+0091 },   // 2.0e+0043
        { 0x1.586213892965Ap+0144,  0x0.AA6246B3388B80p+0092 },   // 3.0e+0043
        { 0x1.CB2D6F618C878p+0144,  0x0.E32DB399A0BA00p+0092 },   // 4.0e+0043
        { 0x1.1EFC659CF7D4Bp+0145,  0x0.8DFC9040047440p+0093 },   // 5.0e+0043
        { 0x1.586213892965Ap+0145,  0x0.AA6246B3388B80p+0093 },   // 6.0e+0043
        { 0x1.91C7C1755AF69p+0145,  0x0.C6C7FD266CA2C0p+0093 },   // 7.0e+0043
        { 0x1.CB2D6F618C878p+0145,  0x0.E32DB399A0BA00p+0093 },   // 8.0e+0043
        { 0x1.02498EA6DF0C3p+0146,  0x0.FFC9B5066A68A0p+0094 },   // 9.0e+0043
        { 0x1.1EFC659CF7D4Bp+0146,  0x0.8DFC9040047440p+0094 },   // 1.0e+0044
        { 0x1.1EFC659CF7D4Bp+0147,  0x0.8DFC9040047440p+0095 },   // 2.0e+0044
        { 0x1.AE7A986B73BF1p+0147,  0x0.54FAD86006AE60p+0095 },   // 3.0e+0044
        { 0x1.1EFC659CF7D4Bp+0148,  0x0.8DFC9040047440p+0096 },   // 4.0e+0044
        { 0x1.66BB7F0435C9Ep+0148,  0x0.717BB450059150p+0096 },   // 5.0e+0044
        { 0x1.AE7A986B73BF1p+0148,  0x0.54FAD86006AE60p+0096 },   // 6.0e+0044
        { 0x1.F639B1D2B1B44p+0148,  0x0.3879FC7007CB70p+0096 },   // 7.0e+0044
        { 0x1.1EFC659CF7D4Bp+0149,  0x0.8DFC9040047440p+0097 },   // 8.0e+0044
        { 0x1.42DBF25096CF4p+0149,  0x0.FFBC22480502C8p+0097 },   // 9.0e+0044
        { 0x1.66BB7F0435C9Ep+0149,  0x0.717BB450059150p+0097 },   // 1.0e+0045
        { 0x1.66BB7F0435C9Ep+0150,  0x0.717BB450059150p+0098 },   // 2.0e+0045
        { 0x1.0D0C9F4328576p+0151,  0x0.D51CC73C042CF8p+0099 },   // 3.0e+0045
        { 0x1.66BB7F0435C9Ep+0151,  0x0.717BB450059150p+0099 },   // 4.0e+0045
        { 0x1.C06A5EC5433C6p+0151,  0x0.0DDAA16406F5A0p+0099 },   // 5.0e+0045
        { 0x1.0D0C9F4328576p+0152,  0x0.D51CC73C042CF8p+0100 },   // 6.0e+0045
        { 0x1.39E40F23AF10Ap+0152,  0x0.A34C3DC604DF20p+0100 },   // 7.0e+0045
        { 0x1.66BB7F0435C9Ep+0152,  0x0.717BB450059150p+0100 },   // 8.0e+0045
        { 0x1.9392EEE4BC832p+0152,  0x0.3FAB2ADA064378p+0100 },   // 9.0e+0045
        { 0x1.C06A5EC5433C6p+0152,  0x0.0DDAA16406F5A0p+0100 },   // 1.0e+0046
        { 0x1.C06A5EC5433C6p+0153,  0x0.0DDAA16406F5A0p+0101 },   // 2.0e+0046
        { 0x1.504FC713F26D4p+0154,  0x0.8A63F90B053838p+0102 },   // 3.0e+0046
        { 0x1.C06A5EC5433C6p+0154,  0x0.0DDAA16406F5A0p+0102 },   // 4.0e+0046
        { 0x1.18427B3B4A05Bp+0155,  0x0.C8A8A4DE845980p+0103 },   // 5.0e+0046
        { 0x1.504FC713F26D4p+0155,  0x0.8A63F90B053838p+0103 },   // 6.0e+0046
        { 0x1.885D12EC9AD4Dp+0155,  0x0.4C1F4D378616E8p+0103 },   // 7.0e+0046
        { 0x1.C06A5EC5433C6p+0155,  0x0.0DDAA16406F5A0p+0103 },   // 8.0e+0046
        { 0x1.F877AA9DEBA3Ep+0155,  0x0.CF95F59087D458p+0103 },   // 9.0e+0046
        { 0x1.18427B3B4A05Bp+0156,  0x0.C8A8A4DE845980p+0104 },   // 1.0e+0047
        { 0x1.18427B3B4A05Bp+0157,  0x0.C8A8A4DE845980p+0105 },   // 2.0e+0047
        { 0x1.A463B8D8EF089p+0157,  0x0.ACFCF74DC68648p+0105 },   // 3.0e+0047
        { 0x1.18427B3B4A05Bp+0158,  0x0.C8A8A4DE845980p+0106 },   // 4.0e+0047
        { 0x1.5E531A0A1C872p+0158,  0x0.BAD2CE16256FE8p+0106 },   // 5.0e+0047
        { 0x1.A463B8D8EF089p+0158,  0x0.ACFCF74DC68648p+0106 },   // 6.0e+0047
        { 0x1.EA7457A7C18A0p+0158,  0x0.9F272085679CA8p+0106 },   // 7.0e+0047
        { 0x1.18427B3B4A05Bp+0159,  0x0.C8A8A4DE845980p+0107 },   // 8.0e+0047
        { 0x1.3B4ACAA2B3467p+0159,  0x0.41BDB97A54E4B0p+0107 },   // 9.0e+0047
        { 0x1.5E531A0A1C872p+0159,  0x0.BAD2CE16256FE8p+0107 },   // 1.0e+0048
        { 0x1.5E531A0A1C872p+0160,  0x0.BAD2CE16256FE8p+0108 },   // 2.0e+0048
        { 0x1.06BE538795656p+0161,  0x0.0C1E1A909C13E8p+0109 },   // 3.0e+0048
        { 0x1.5E531A0A1C872p+0161,  0x0.BAD2CE16256FE8p+0109 },   // 4.0e+0048
        { 0x1.B5E7E08CA3A8Fp+0161,  0x0.6987819BAECBE0p+0109 },   // 5.0e+0048
        { 0x1.06BE538795656p+0162,  0x0.0C1E1A909C13E8p+0110 },   // 6.0e+0048
        { 0x1.3288B6C8D8F64p+0162,  0x0.6378745360C1E8p+0110 },   // 7.0e+0048
        { 0x1.5E531A0A1C872p+0162,  0x0.BAD2CE16256FE8p+0110 },   // 8.0e+0048
        { 0x1.8A1D7D4B60181p+0162,  0x0.122D27D8EA1DE0p+0110 },   // 9.0e+0048
        { 0x1.B5E7E08CA3A8Fp+0162,  0x0.6987819BAECBE0p+0110 },   // 1.0e+0049
        { 0x1.B5E7E08CA3A8Fp+0163,  0x0.6987819BAECBE0p+0111 },   // 2.0e+0049
        { 0x1.486DE8697ABEBp+0164,  0x0.8F25A134C318E8p+0112 },   // 3.0e+0049
        { 0x1.B5E7E08CA3A8Fp+0164,  0x0.6987819BAECBE0p+0112 },   // 4.0e+0049
        { 0x1.11B0EC57E6499p+0165,  0x0.A1F4B1014D3F68p+0113 },   // 5.0e+0049
        { 0x1.486DE8697ABEBp+0165,  0x0.8F25A134C318E8p+0113 },   // 6.0e+0049
        { 0x1.7F2AE47B0F33Dp+0165,  0x0.7C56916838F260p+0113 },   // 7.0e+0049
        { 0x1.B5E7E08CA3A8Fp+0165,  0x0.6987819BAECBE0p+0113 },   // 8.0e+0049
        { 0x1.ECA4DC9E381E1p+0165,  0x0.56B871CF24A558p+0113 },   // 9.0e+0049
        { 0x1.11B0EC57E6499p+0166,  0x0.A1F4B1014D3F68p+0114 },   // 1.0e+0050
        { 0x1.11B0EC57E6499p+0167,  0x0.A1F4B1014D3F68p+0115 },   // 2.0e+0050
        { 0x1.9A896283D96E6p+0167,  0x0.72EF0981F3DF20p+0115 },   // 3.0e+0050
        { 0x1.11B0EC57E6499p+0168,  0x0.A1F4B1014D3F68p+0116 },   // 4.0e+0050
        { 0x1.561D276DDFDC0p+0168,  0x0.0A71DD41A08F48p+0116 },   // 5.0e+0050
        { 0x1.9A896283D96E6p+0168,  0x0.72EF0981F3DF20p+0116 },   // 6.0e+0050
        { 0x1.DEF59D99D300Cp+0168,  0x0.DB6C35C2472EF8p+0116 },   // 7.0e+0050
        { 0x1.11B0EC57E6499p+0169,  0x0.A1F4B1014D3F68p+0117 },   // 8.0e+0050
        { 0x1.33E709E2E312Cp+0169,  0x0.D633472176E758p+0117 },   // 9.0e+0050
        { 0x1.561D276DDFDC0p+0169,  0x0.0A71DD41A08F48p+0117 },   // 1.0e+0051
        { 0x1.561D276DDFDC0p+0170,  0x0.0A71DD41A08F48p+0118 },   // 2.0e+0051
        { 0x1.0095DD9267E50p+0171,  0x0.07D565F1386B70p+0119 },   // 3.0e+0051
        { 0x1.561D276DDFDC0p+0171,  0x0.0A71DD41A08F48p+0119 },   // 4.0e+0051
        { 0x1.ABA4714957D30p+0171,  0x0.0D0E549208B318p+0119 },   // 5.0e+0051
        { 0x1.0095DD9267E50p+0172,  0x0.07D565F1386B70p+0120 },   // 6.0e+0051
        { 0x1.2B59828023E08p+0172,  0x0.0923A1996C7D58p+0120 },   // 7.0e+0051
        { 0x1.561D276DDFDC0p+0172,  0x0.0A71DD41A08F48p+0120 },   // 8.0e+0051
        { 0x1.80E0CC5B9BD78p+0172,  0x0.0BC018E9D4A130p+0120 },   // 9.0e+0051
        { 0x1.ABA4714957D30p+0172,  0x0.0D0E549208B318p+0120 },   // 1.0e+0052
        { 0x1.ABA4714957D30p+0173,  0x0.0D0E549208B318p+0121 },   // 2.0e+0052
        { 0x1.40BB54F701DE4p+0174,  0x0.09CABF6D868650p+0122 },   // 3.0e+0052
        { 0x1.ABA4714957D30p+0174,  0x0.0D0E549208B318p+0122 },   // 4.0e+0052
        { 0x1.0B46C6CDD6E3Ep+0175,  0x0.0828F4DB456FF0p+0123 },   // 5.0e+0052
        { 0x1.40BB54F701DE4p+0175,  0x0.09CABF6D868650p+0123 },   // 6.0e+0052
        { 0x1.762FE3202CD8Ap+0175,  0x0.0B6C89FFC79CB0p+0123 },   // 7.0e+0052
        { 0x1.ABA4714957D30p+0175,  0x0.0D0E549208B318p+0123 },   // 8.0e+0052
        { 0x1.E118FF7282CD6p+0175,  0x0.0EB01F2449C978p+0123 },   // 9.0e+0052
        { 0x1.0B46C6CDD6E3Ep+0176,  0x0.0828F4DB456FF0p+0124 },   // 1.0e+0053
        { 0x1.0B46C6CDD6E3Ep+0177,  0x0.0828F4DB456FF0p+0125 },   // 2.0e+0053
        { 0x1.90EA2A34C255Dp+0177,  0x0.0C3D6F48E827E8p+0125 },   // 3.0e+0053
        { 0x1.0B46C6CDD6E3Ep+0178,  0x0.0828F4DB456FF0p+0126 },   // 4.0e+0053
        { 0x1.4E1878814C9CDp+0178,  0x0.8A33321216CBE8p+0126 },   // 5.0e+0053
        { 0x1.90EA2A34C255Dp+0178,  0x0.0C3D6F48E827E8p+0126 },   // 6.0e+0053
        { 0x1.D3BBDBE8380ECp+0178,  0x0.8E47AC7FB983E0p+0126 },   // 7.0e+0053
        { 0x1.0B46C6CDD6E3Ep+0179,  0x0.0828F4DB456FF0p+0127 },   // 8.0e+0053
        { 0x1.2CAF9FA791C05p+0179,  0x0.C92E1376AE1DE8p+0127 },   // 9.0e+0053
        { 0x1.4E1878814C9CDp+0179,  0x0.8A33321216CBE8p+0127 },   // 1.0e+0054
        { 0x1.4E1878814C9CDp+0180,  0x0.8A33321216CBE8p+0128 },   // 2.0e+0054
        { 0x1.F524B4C1F2EB4p+0180,  0x0.4F4CCB1B2231E0p+0128 },   // 3.0e+0054
        { 0x1.4E1878814C9CDp+0181,  0x0.8A33321216CBE8p+0129 },   // 4.0e+0054
        { 0x1.A19E96A19FC40p+0181,  0x0.ECBFFE969C7EE8p+0129 },   // 5.0e+0054
        { 0x1.F524B4C1F2EB4p+0181,  0x0.4F4CCB1B2231E0p+0129 },   // 6.0e+0054
        { 0x1.2455697123093p+0182,  0x0.D8ECCBCFD3F268p+0130 },   // 7.0e+0054
        { 0x1.4E1878814C9CDp+0182,  0x0.8A33321216CBE8p+0130 },   // 8.0e+0054
        { 0x1.77DB879176307p+0182,  0x0.3B79985459A568p+0130 },   // 9.0e+0054
        { 0x1.A19E96A19FC40p+0182,  0x0.ECBFFE969C7EE8p+0130 },   // 1.0e+0055
        { 0x1.A19E96A19FC40p+0183,  0x0.ECBFFE969C7EE8p+0131 },   // 2.0e+0055
        { 0x1.3936F0F937D30p+0184,  0x0.B18FFEF0F55F28p+0132 },   // 3.0e+0055
        { 0x1.A19E96A19FC40p+0184,  0x0.ECBFFE969C7EE8p+0132 },   // 4.0e+0055
        { 0x1.05031E2503DA8p+0185,  0x0.93F7FF1E21CF50p+0133 },   // 5.0e+0055
        { 0x1.3936F0F937D30p+0185,  0x0.B18FFEF0F55F28p+0133 },   // 6.0e+0055
        { 0x1.6D6AC3CD6BCB8p+0185,  0x0.CF27FEC3C8EF08p+0133 },   // 7.0e+0055
        { 0x1.A19E96A19FC40p+0185,  0x0.ECBFFE969C7EE8p+0133 },   // 8.0e+0055
        { 0x1.D5D26975D3BC9p+0185,  0x0.0A57FE69700EC0p+0133 },   // 9.0e+0055
        { 0x1.05031E2503DA8p+0186,  0x0.93F7FF1E21CF50p+0134 },   // 1.0e+0056
        { 0x1.05031E2503DA8p+0187,  0x0.93F7FF1E21CF50p+0135 },   // 2.0e+0056
        { 0x1.8784AD3785C7Cp+0187,  0x0.DDF3FEAD32B6F8p+0135 },   // 3.0e+0056
        { 0x1.05031E2503DA8p+0188,  0x0.93F7FF1E21CF50p+0136 },   // 4.0e+0056
        { 0x1.4643E5AE44D12p+0188,  0x0.B8F5FEE5AA4320p+0136 },   // 5.0e+0056
        { 0x1.8784AD3785C7Cp+0188,  0x0.DDF3FEAD32B6F8p+0136 },   // 6.0e+0056
        { 0x1.C8C574C0C6BE7p+0188,  0x0.02F1FE74BB2AC8p+0136 },   // 7.0e+0056
        { 0x1.05031E2503DA8p+0189,  0x0.93F7FF1E21CF50p+0137 },   // 8.0e+0056
        { 0x1.25A381E9A455Dp+0189,  0x0.A676FF01E60938p+0137 },   // 9.0e+0056
        { 0x1.4643E5AE44D12p+0189,  0x0.B8F5FEE5AA4320p+0137 },   // 1.0e+0057
        { 0x1.4643E5AE44D12p+0190,  0x0.B8F5FEE5AA4320p+0138 },   // 2.0e+0057
        { 0x1.E965D8856739Cp+0190,  0x0.1570FE587F64B8p+0138 },   // 3.0e+0057
        { 0x1.4643E5AE44D12p+0191,  0x0.B8F5FEE5AA4320p+0139 },   // 4.0e+0057
        { 0x1.97D4DF19D6057p+0191,  0x0.67337E9F14D3E8p+0139 },   // 5.0e+0057
        { 0x1.E965D8856739Cp+0191,  0x0.1570FE587F64B8p+0139 },   // 6.0e+0057
        { 0x1.1D7B68F87C370p+0192,  0x0.61D73F08F4FAC0p+0140 },   // 7.0e+0057
        { 0x1.4643E5AE44D12p+0192,  0x0.B8F5FEE5AA4320p+0140 },   // 8.0e+0057
        { 0x1.6F0C62640D6B5p+0192,  0x0.1014BEC25F8B88p+0140 },   // 9.0e+0057
        { 0x1.97D4DF19D6057p+0192,  0x0.67337E9F14D3E8p+0140 },   // 1.0e+0058
        { 0x1.97D4DF19D6057p+0193,  0x0.67337E9F14D3E8p+0141 },   // 2.0e+0058
        { 0x1.31DFA75360841p+0194,  0x0.8D669EF74F9EF0p+0142 },   // 3.0e+0058
        { 0x1.97D4DF19D6057p+0194,  0x0.67337E9F14D3E8p+0142 },   // 4.0e+0058
        { 0x1.FDCA16E04B86Dp+0194,  0x0.41005E46DA08E8p+0142 },   // 5.0e+0058
        { 0x1.31DFA75360841p+0195,  0x0.8D669EF74F9EF0p+0143 },   // 6.0e+0058
        { 0x1.64DA43369B44Cp+0195,  0x0.7A4D0ECB323970p+0143 },   // 7.0e+0058
        { 0x1.97D4DF19D6057p+0195,  0x0.67337E9F14D3E8p+0143 },   // 8.0e+0058
        { 0x1.CACF7AFD10C62p+0195,  0x0.5419EE72F76E68p+0143 },   // 9.0e+0058
        { 0x1.FDCA16E04B86Dp+0195,  0x0.41005E46DA08E8p+0143 },   // 1.0e+0059
        { 0x1.FDCA16E04B86Dp+0196,  0x0.41005E46DA08E8p+0144 },   // 2.0e+0059
        { 0x1.7E57912838A51p+0197,  0x0.F0C046B52386A8p+0145 },   // 3.0e+0059
        { 0x1.FDCA16E04B86Dp+0197,  0x0.41005E46DA08E8p+0145 },   // 4.0e+0059
        { 0x1.3E9E4E4C2F344p+0198,  0x0.48A03AEC484590p+0146 },   // 5.0e+0059
        { 0x1.7E57912838A51p+0198,  0x0.F0C046B52386A8p+0146 },   // 6.0e+0059
        { 0x1.BE10D4044215Fp+0198,  0x0.98E0527DFEC7C8p+0146 },   // 7.0e+0059
        { 0x1.FDCA16E04B86Dp+0198,  0x0.41005E46DA08E8p+0146 },   // 8.0e+0059
        { 0x1.1EC1ACDE2A7BDp+0199,  0x0.74903507DAA500p+0147 },   // 9.0e+0059
        { 0x1.3E9E4E4C2F344p+0199,  0x0.48A03AEC484590p+0147 },   // 1.0e+0060
        { 0x1.3E9E4E4C2F344p+0200,  0x0.48A03AEC484590p+0148 },   // 2.0e+0060
        { 0x1.DDED757246CE6p+0200,  0x0.6CF058626C6858p+0148 },   // 3.0e+0060
        { 0x1.3E9E4E4C2F344p+0201,  0x0.48A03AEC484590p+0149 },   // 4.0e+0060
        { 0x1.8E45E1DF3B015p+0201,  0x0.5AC849A75A56F0p+0149 },   // 5.0e+0060
        { 0x1.DDED757246CE6p+0201,  0x0.6CF058626C6858p+0149 },   // 6.0e+0060
        { 0x1.16CA8482A94DBp+0202,  0x0.BF8C338EBF3CE0p+0150 },   // 7.0e+0060
        { 0x1.3E9E4E4C2F344p+0202,  0x0.48A03AEC484590p+0150 },   // 8.0e+0060
        { 0x1.66721815B51ACp+0202,  0x0.D1B44249D14E40p+0150 },   // 9.0e+0060
        { 0x1.8E45E1DF3B015p+0202,  0x0.5AC849A75A56F0p+0150 },   // 1.0e+0061
        { 0x1.8E45E1DF3B015p+0203,  0x0.5AC849A75A56F0p+0151 },   // 2.0e+0061
        { 0x1.2AB469676C410p+0204,  0x0.0416373D83C138p+0152 },   // 3.0e+0061
        { 0x1.8E45E1DF3B015p+0204,  0x0.5AC849A75A56F0p+0152 },   // 4.0e+0061
        { 0x1.F1D75A5709C1Ap+0204,  0x0.B17A5C1130ECB0p+0152 },   // 5.0e+0061
        { 0x1.2AB469676C410p+0205,  0x0.0416373D83C138p+0153 },   // 6.0e+0061
        { 0x1.5C7D25A353A12p+0205,  0x0.AF6F40726F0C18p+0153 },   // 7.0e+0061
        { 0x1.8E45E1DF3B015p+0205,  0x0.5AC849A75A56F0p+0153 },   // 8.0e+0061
        { 0x1.C00E9E1B22618p+0205,  0x0.062152DC45A1D0p+0153 },   // 9.0e+0061
        { 0x1.F1D75A5709C1Ap+0205,  0x0.B17A5C1130ECB0p+0153 },   // 1.0e+0062
        { 0x1.F1D75A5709C1Ap+0206,  0x0.B17A5C1130ECB0p+0154 },   // 2.0e+0062
        { 0x1.756183C147514p+0207,  0x0.051BC50CE4B180p+0155 },   // 3.0e+0062
        { 0x1.F1D75A5709C1Ap+0207,  0x0.B17A5C1130ECB0p+0155 },   // 4.0e+0062
        { 0x1.3726987666190p+0208,  0x0.AEEC798ABE93F0p+0156 },   // 5.0e+0062
        { 0x1.756183C147514p+0208,  0x0.051BC50CE4B180p+0156 },   // 6.0e+0062
        { 0x1.B39C6F0C28897p+0208,  0x0.5B4B108F0ACF18p+0156 },   // 7.0e+0062
        { 0x1.F1D75A5709C1Ap+0208,  0x0.B17A5C1130ECB0p+0156 },   // 8.0e+0062
        { 0x1.180922D0F57CFp+0209,  0x0.03D4D3C9AB8520p+0157 },   // 9.0e+0062
        { 0x1.3726987666190p+0209,  0x0.AEEC798ABE93F0p+0157 },   // 1.0e+0063
        { 0x1.3726987666190p+0210,  0x0.AEEC798ABE93F0p+0158 },   // 2.0e+0063
        { 0x1.D2B9E4B199259p+0210,  0x0.0662B6501DDDE8p+0158 },   // 3.0e+0063
        { 0x1.3726987666190p+0211,  0x0.AEEC798ABE93F0p+0159 },   // 4.0e+0063
        { 0x1.84F03E93FF9F4p+0211,  0x0.DAA797ED6E38E8p+0159 },   // 5.0e+0063
        { 0x1.D2B9E4B199259p+0211,  0x0.0662B6501DDDE8p+0159 },   // 6.0e+0063
        { 0x1.1041C5679955Ep+0212,  0x0.990EEA5966C170p+0160 },   // 7.0e+0063
        { 0x1.3726987666190p+0212,  0x0.AEEC798ABE93F0p+0160 },   // 8.0e+0063
        { 0x1.5E0B6B8532DC2p+0212,  0x0.C4CA08BC166668p+0160 },   // 9.0e+0063
        { 0x1.84F03E93FF9F4p+0212,  0x0.DAA797ED6E38E8p+0160 },   // 1.0e+0064
        { 0x1.84F03E93FF9F4p+0213,  0x0.DAA797ED6E38E8p+0161 },   // 2.0e+0064
        { 0x1.23B42EEEFFB77p+0214,  0x0.A3FDB1F212AAB0p+0162 },   // 3.0e+0064
        { 0x1.84F03E93FF9F4p+0214,  0x0.DAA797ED6E38E8p+0162 },   // 4.0e+0064
        { 0x1.E62C4E38FF872p+0214,  0x0.11517DE8C9C728p+0162 },   // 5.0e+0064
        { 0x1.23B42EEEFFB77p+0215,  0x0.A3FDB1F212AAB0p+0163 },   // 6.0e+0064
        { 0x1.545236C17FAB6p+0215,  0x0.3F52A4EFC071C8p+0163 },   // 7.0e+0064
        { 0x1.84F03E93FF9F4p+0215,  0x0.DAA797ED6E38E8p+0163 },   // 8.0e+0064
        { 0x1.B58E46667F933p+0215,  0x0.75FC8AEB1C0008p+0163 },   // 9.0e+0064
        { 0x1.E62C4E38FF872p+0215,  0x0.11517DE8C9C728p+0163 },   // 1.0e+0065
        { 0x1.E62C4E38FF872p+0216,  0x0.11517DE8C9C728p+0164 },   // 2.0e+0065
        { 0x1.6CA13AAABFA55p+0217,  0x0.8CFD1E6E975558p+0165 },   // 3.0e+0065
        { 0x1.E62C4E38FF872p+0217,  0x0.11517DE8C9C728p+0165 },   // 4.0e+0065
        { 0x1.2FDBB0E39FB47p+0218,  0x0.4AD2EEB17E1C78p+0166 },   // 5.0e+0065
        { 0x1.6CA13AAABFA55p+0218,  0x0.8CFD1E6E975558p+0166 },   // 6.0e+0065
        { 0x1.A966C471DF963p+0218,  0x0.CF274E2BB08E40p+0166 },   // 7.0e+0065
        { 0x1.E62C4E38FF872p+0218,  0x0.11517DE8C9C728p+0166 },   // 8.0e+0065
        { 0x1.1178EC000FBC0p+0219,  0x0.29BDD6D2F18000p+0167 },   // 9.0e+0065
        { 0x1.2FDBB0E39FB47p+0219,  0x0.4AD2EEB17E1C78p+0167 },   // 1.0e+0066
        { 0x1.2FDBB0E39FB47p+0220,  0x0.4AD2EEB17E1C78p+0168 },   // 2.0e+0066
        { 0x1.C7C989556F8EAp+0220,  0x0.F03C660A3D2AB0p+0168 },   // 3.0e+0066
        { 0x1.2FDBB0E39FB47p+0221,  0x0.4AD2EEB17E1C78p+0169 },   // 4.0e+0066
        { 0x1.7BD29D1C87A19p+0221,  0x0.1D87AA5DDDA390p+0169 },   // 5.0e+0066
        { 0x1.C7C989556F8EAp+0221,  0x0.F03C660A3D2AB0p+0169 },   // 6.0e+0066
        { 0x1.09E03AC72BBDEp+0222,  0x0.617890DB4E58E8p+0170 },   // 7.0e+0066
        { 0x1.2FDBB0E39FB47p+0222,  0x0.4AD2EEB17E1C78p+0170 },   // 8.0e+0066
        { 0x1.55D7270013AB0p+0222,  0x0.342D4C87ADE008p+0170 },   // 9.0e+0066
        { 0x1.7BD29D1C87A19p+0222,  0x0.1D87AA5DDDA390p+0170 },   // 1.0e+0067
        { 0x1.7BD29D1C87A19p+0223,  0x0.1D87AA5DDDA390p+0171 },   // 2.0e+0067
        { 0x1.1CDDF5D565B92p+0224,  0x0.D625BFC6663AB0p+0172 },   // 3.0e+0067
        { 0x1.7BD29D1C87A19p+0224,  0x0.1D87AA5DDDA390p+0172 },   // 4.0e+0067
        { 0x1.DAC74463A989Fp+0224,  0x0.64E994F5550C78p+0172 },   // 5.0e+0067
        { 0x1.1CDDF5D565B92p+0225,  0x0.D625BFC6663AB0p+0173 },   // 6.0e+0067
        { 0x1.4C584978F6AD5p+0225,  0x0.F9D6B51221EF20p+0173 },   // 7.0e+0067
        { 0x1.7BD29D1C87A19p+0225,  0x0.1D87AA5DDDA390p+0173 },   // 8.0e+0067
        { 0x1.AB4CF0C01895Cp+0225,  0x0.41389FA9995808p+0173 },   // 9.0e+0067
        { 0x1.DAC74463A989Fp+0225,  0x0.64E994F5550C78p+0173 },   // 1.0e+0068
        { 0x1.DAC74463A989Fp+0226,  0x0.64E994F5550C78p+0174 },   // 2.0e+0068
        { 0x1.6415734ABF277p+0227,  0x0.8BAF2FB7FFC958p+0175 },   // 3.0e+0068
        { 0x1.DAC74463A989Fp+0227,  0x0.64E994F5550C78p+0175 },   // 4.0e+0068
        { 0x1.28BC8ABE49F63p+0228,  0x0.9F11FD195527C8p+0176 },   // 5.0e+0068
        { 0x1.6415734ABF277p+0228,  0x0.8BAF2FB7FFC958p+0176 },   // 6.0e+0068
        { 0x1.9F6E5BD73458Bp+0228,  0x0.784C6256AA6AE8p+0176 },   // 7.0e+0068
        { 0x1.DAC74463A989Fp+0228,  0x0.64E994F5550C78p+0176 },   // 8.0e+0068
        { 0x1.0B1016780F5D9p+0229,  0x0.A8C363C9FFD700p+0177 },   // 9.0e+0068
        { 0x1.28BC8ABE49F63p+0229,  0x0.9F11FD195527C8p+0177 },   // 1.0e+0069
        { 0x1.28BC8ABE49F63p+0230,  0x0.9F11FD195527C8p+0178 },   // 2.0e+0069
        { 0x1.BD1AD01D6EF15p+0230,  0x0.6E9AFBA5FFBBB0p+0178 },   // 3.0e+0069
        { 0x1.28BC8ABE49F63p+0231,  0x0.9F11FD195527C8p+0179 },   // 4.0e+0069
        { 0x1.72EBAD6DDC73Cp+0231,  0x0.86D67C5FAA71C0p+0179 },   // 5.0e+0069
        { 0x1.BD1AD01D6EF15p+0231,  0x0.6E9AFBA5FFBBB0p+0179 },   // 6.0e+0069
        { 0x1.03A4F96680B77p+0232,  0x0.2B2FBD762A82D0p+0180 },   // 7.0e+0069
        { 0x1.28BC8ABE49F63p+0232,  0x0.9F11FD195527C8p+0180 },   // 8.0e+0069
        { 0x1.4DD41C1613350p+0232,  0x0.12F43CBC7FCCC8p+0180 },   // 9.0e+0069
        { 0x1.72EBAD6DDC73Cp+0232,  0x0.86D67C5FAA71C0p+0180 },   // 1.0e+0070
        { 0x1.72EBAD6DDC73Cp+0233,  0x0.86D67C5FAA71C0p+0181 },   // 2.0e+0070
        { 0x1.1630C2126556Dp+0234,  0x0.6520DD47BFD550p+0182 },   // 3.0e+0070
        { 0x1.72EBAD6DDC73Cp+0234,  0x0.86D67C5FAA71C0p+0182 },   // 4.0e+0070
        { 0x1.CFA698C95390Bp+0234,  0x0.A88C1B77950E30p+0182 },   // 5.0e+0070
        { 0x1.1630C2126556Dp+0235,  0x0.6520DD47BFD550p+0183 },   // 6.0e+0070
        { 0x1.448E37C020E54p+0235,  0x0.F5FBACD3B52388p+0183 },   // 7.0e+0070
        { 0x1.72EBAD6DDC73Cp+0235,  0x0.86D67C5FAA71C0p+0183 },   // 8.0e+0070
        { 0x1.A149231B98024p+0235,  0x0.17B14BEB9FBFF8p+0183 },   // 9.0e+0070
        { 0x1.CFA698C95390Bp+0235,  0x0.A88C1B77950E30p+0183 },   // 1.0e+0071
        { 0x1.CFA698C95390Bp+0236,  0x0.A88C1B77950E30p+0184 },   // 2.0e+0071
        { 0x1.5BBCF296FEAC8p+0237,  0x0.BE691499AFCAA0p+0185 },   // 3.0e+0071
        { 0x1.CFA698C95390Bp+0237,  0x0.A88C1B77950E30p+0185 },   // 4.0e+0071
        { 0x1.21C81F7DD43A7p+0238,  0x0.4957912ABD28D8p+0186 },   // 5.0e+0071
        { 0x1.5BBCF296FEAC8p+0238,  0x0.BE691499AFCAA0p+0186 },   // 6.0e+0071
        { 0x1.95B1C5B0291EAp+0238,  0x0.337A9808A26C68p+0186 },   // 7.0e+0071
        { 0x1.CFA698C95390Bp+0238,  0x0.A88C1B77950E30p+0186 },   // 8.0e+0071
        { 0x1.04CDB5F13F016p+0239,  0x0.8ECECF7343D7F8p+0187 },   // 9.0e+0071
        { 0x1.21C81F7DD43A7p+0239,  0x0.4957912ABD28D8p+0187 },   // 1.0e+0072
        { 0x1.21C81F7DD43A7p+0240,  0x0.4957912ABD28D8p+0188 },   // 2.0e+0072
        { 0x1.B2AC2F3CBE57Ap+0240,  0x0.EE0359C01BBD48p+0188 },   // 3.0e+0072
        { 0x1.21C81F7DD43A7p+0241,  0x0.4957912ABD28D8p+0189 },   // 4.0e+0072
        { 0x1.6A3A275D49491p+0241,  0x0.1BAD75756C7310p+0189 },   // 5.0e+0072
        { 0x1.B2AC2F3CBE57Ap+0241,  0x0.EE0359C01BBD48p+0189 },   // 6.0e+0072
        { 0x1.FB1E371C33664p+0241,  0x0.C0593E0ACB0780p+0189 },   // 7.0e+0072
        { 0x1.21C81F7DD43A7p+0242,  0x0.4957912ABD28D8p+0190 },   // 8.0e+0072
        { 0x1.4601236D8EC1Cp+0242,  0x0.3282835014CDF8p+0190 },   // 9.0e+0072
        { 0x1.6A3A275D49491p+0242,  0x0.1BAD75756C7310p+0190 },   // 1.0e+0073
        { 0x1.6A3A275D49491p+0243,  0x0.1BAD75756C7310p+0191 },   // 2.0e+0073
        { 0x1.0FAB9D85F6F6Cp+0244,  0x0.D4C21818115650p+0192 },   // 3.0e+0073
        { 0x1.6A3A275D49491p+0244,  0x0.1BAD75756C7310p+0192 },   // 4.0e+0073
        { 0x1.C4C8B1349B9B5p+0244,  0x0.6298D2D2C78FD8p+0192 },   // 5.0e+0073
        { 0x1.0FAB9D85F6F6Cp+0245,  0x0.D4C21818115650p+0193 },   // 6.0e+0073
        { 0x1.3CF2E271A01FEp+0245,  0x0.F837C6C6BEE4B0p+0193 },   // 7.0e+0073
        { 0x1.6A3A275D49491p+0245,  0x0.1BAD75756C7310p+0193 },   // 8.0e+0073
        { 0x1.97816C48F2723p+0245,  0x0.3F2324241A0178p+0193 },   // 9.0e+0073
        { 0x1.C4C8B1349B9B5p+0245,  0x0.6298D2D2C78FD8p+0193 },   // 1.0e+0074
        { 0x1.C4C8B1349B9B5p+0246,  0x0.6298D2D2C78FD8p+0194 },   // 2.0e+0074
        { 0x1.539684E774B48p+0247,  0x0.09F29E1E15ABE0p+0195 },   // 3.0e+0074
        { 0x1.C4C8B1349B9B5p+0247,  0x0.6298D2D2C78FD8p+0195 },   // 4.0e+0074
        { 0x1.1AFD6EC0E1411p+0248,  0x0.5D9F83C3BCB9E8p+0196 },   // 5.0e+0074
        { 0x1.539684E774B48p+0248,  0x0.09F29E1E15ABE0p+0196 },   // 6.0e+0074
        { 0x1.8C2F9B0E0827Ep+0248,  0x0.B645B8786E9DE0p+0196 },   // 7.0e+0074
        { 0x1.C4C8B1349B9B5p+0248,  0x0.6298D2D2C78FD8p+0196 },   // 8.0e+0074
        { 0x1.FD61C75B2F0ECp+0248,  0x0.0EEBED2D2081D8p+0196 },   // 9.0e+0074
        { 0x1.1AFD6EC0E1411p+0249,  0x0.5D9F83C3BCB9E8p+0197 },   // 1.0e+0075
        { 0x1.1AFD6EC0E1411p+0250,  0x0.5D9F83C3BCB9E8p+0198 },   // 2.0e+0075
        { 0x1.A87C262151E1Ap+0250,  0x0.0C6F45A59B16D8p+0198 },   // 3.0e+0075
        { 0x1.1AFD6EC0E1411p+0251,  0x0.5D9F83C3BCB9E8p+0199 },   // 4.0e+0075
        { 0x1.61BCCA7119915p+0251,  0x0.B50764B4ABE860p+0199 },   // 5.0e+0075
        { 0x1.A87C262151E1Ap+0251,  0x0.0C6F45A59B16D8p+0199 },   // 6.0e+0075
        { 0x1.EF3B81D18A31Ep+0251,  0x0.63D726968A4558p+0199 },   // 7.0e+0075
        { 0x1.1AFD6EC0E1411p+0252,  0x0.5D9F83C3BCB9E8p+0200 },   // 8.0e+0075
        { 0x1.3E5D1C98FD693p+0252,  0x0.8953743C345120p+0200 },   // 9.0e+0075
        { 0x1.61BCCA7119915p+0252,  0x0.B50764B4ABE860p+0200 },   // 1.0e+0076
        { 0x1.61BCCA7119915p+0253,  0x0.B50764B4ABE860p+0201 },   // 2.0e+0076
        { 0x1.094D97D4D32D0p+0254,  0x0.47C58B8780EE48p+0202 },   // 3.0e+0076
        { 0x1.61BCCA7119915p+0254,  0x0.B50764B4ABE860p+0202 },   // 4.0e+0076
        { 0x1.BA2BFD0D5FF5Bp+0254,  0x0.22493DE1D6E278p+0202 },   // 5.0e+0076
        { 0x1.094D97D4D32D0p+0255,  0x0.47C58B8780EE48p+0203 },   // 6.0e+0076
        { 0x1.35853122F65F2p+0255,  0x0.FE66781E166B58p+0203 },   // 7.0e+0076
        { 0x1.61BCCA7119915p+0255,  0x0.B50764B4ABE860p+0203 },   // 8.0e+0076
        { 0x1.8DF463BF3CC38p+0255,  0x0.6BA8514B416570p+0203 },   // 9.0e+0076
        { 0x1.BA2BFD0D5FF5Bp+0255,  0x0.22493DE1D6E278p+0203 },   // 1.0e+0077
        { 0x1.BA2BFD0D5FF5Bp+0256,  0x0.22493DE1D6E278p+0204 },   // 2.0e+0077
        { 0x1.4BA0FDCA07F84p+0257,  0x0.59B6EE696129D8p+0205 },   // 3.0e+0077
        { 0x1.BA2BFD0D5FF5Bp+0257,  0x0.22493DE1D6E278p+0205 },   // 4.0e+0077
        { 0x1.145B7E285BF98p+0258,  0x0.F56DC6AD264D88p+0206 },   // 5.0e+0077
        { 0x1.4BA0FDCA07F84p+0258,  0x0.59B6EE696129D8p+0206 },   // 6.0e+0077
        { 0x1.82E67D6BB3F6Fp+0258,  0x0.BE0016259C0628p+0206 },   // 7.0e+0077
        { 0x1.BA2BFD0D5FF5Bp+0258,  0x0.22493DE1D6E278p+0206 },   // 8.0e+0077
        { 0x1.F1717CAF0BF46p+0258,  0x0.8692659E11BEC8p+0206 },   // 9.0e+0077
        { 0x1.145B7E285BF98p+0259,  0x0.F56DC6AD264D88p+0207 },   // 1.0e+0078
        { 0x1.145B7E285BF98p+0260,  0x0.F56DC6AD264D88p+0208 },   // 2.0e+0078
        { 0x1.9E893D3C89F65p+0260,  0x0.7024AA03B97450p+0208 },   // 3.0e+0078
        { 0x1.145B7E285BF98p+0261,  0x0.F56DC6AD264D88p+0209 },   // 4.0e+0078
        { 0x1.59725DB272F7Fp+0261,  0x0.32C938586FE0F0p+0209 },   // 5.0e+0078
        { 0x1.9E893D3C89F65p+0261,  0x0.7024AA03B97450p+0209 },   // 6.0e+0078
        { 0x1.E3A01CC6A0F4Bp+0261,  0x0.AD801BAF0307B8p+0209 },   // 7.0e+0078
        { 0x1.145B7E285BF98p+0262,  0x0.F56DC6AD264D88p+0210 },   // 8.0e+0078
        { 0x1.36E6EDED6778Cp+0262,  0x0.141B7F82CB1740p+0210 },   // 9.0e+0078
        { 0x1.59725DB272F7Fp+0262,  0x0.32C938586FE0F0p+0210 },   // 1.0e+0079
        { 0x1.59725DB272F7Fp+0263,  0x0.32C938586FE0F0p+0211 },   // 2.0e+0079
        { 0x1.0315C645D639Fp+0264,  0x0.6616EA4253E8B0p+0212 },   // 3.0e+0079
        { 0x1.59725DB272F7Fp+0264,  0x0.32C938586FE0F0p+0212 },   // 4.0e+0079
        { 0x1.AFCEF51F0FB5Ep+0264,  0x0.FF7B866E8BD928p+0212 },   // 5.0e+0079
        { 0x1.0315C645D639Fp+0265,  0x0.6616EA4253E8B0p+0213 },   // 6.0e+0079
        { 0x1.2E4411FC2498Fp+0265,  0x0.4C70114D61E4D0p+0213 },   // 7.0e+0079
        { 0x1.59725DB272F7Fp+0265,  0x0.32C938586FE0F0p+0213 },   // 8.0e+0079
        { 0x1.84A0A968C156Fp+0265,  0x0.19225F637DDD10p+0213 },   // 9.0e+0079
        { 0x1.AFCEF51F0FB5Ep+0265,  0x0.FF7B866E8BD928p+0213 },   // 1.0e+0080
        { 0x1.AFCEF51F0FB5Ep+0266,  0x0.FF7B866E8BD928p+0214 },   // 2.0e+0080
        { 0x1.43DB37D74BC87p+0267,  0x0.3F9CA4D2E8E2E0p+0215 },   // 3.0e+0080
        { 0x1.AFCEF51F0FB5Ep+0267,  0x0.FF7B866E8BD928p+0215 },   // 4.0e+0080
        { 0x1.0DE1593369D1Bp+0268,  0x0.5FAD34051767B8p+0216 },   // 5.0e+0080
        { 0x1.43DB37D74BC87p+0268,  0x0.3F9CA4D2E8E2E0p+0216 },   // 6.0e+0080
        { 0x1.79D5167B2DBF3p+0268,  0x0.1F8C15A0BA5E08p+0216 },   // 7.0e+0080
        { 0x1.AFCEF51F0FB5Ep+0268,  0x0.FF7B866E8BD928p+0216 },   // 8.0e+0080
        { 0x1.E5C8D3C2F1ACAp+0268,  0x0.DF6AF73C5D5450p+0216 },   // 9.0e+0080
        { 0x1.0DE1593369D1Bp+0269,  0x0.5FAD34051767B8p+0217 },   // 1.0e+0081
        { 0x1.0DE1593369D1Bp+0270,  0x0.5FAD34051767B8p+0218 },   // 2.0e+0081
        { 0x1.94D205CD1EBA9p+0270,  0x0.0F83CE07A31B98p+0218 },   // 3.0e+0081
        { 0x1.0DE1593369D1Bp+0271,  0x0.5FAD34051767B8p+0219 },   // 4.0e+0081
        { 0x1.5159AF8044462p+0271,  0x0.379881065D41A8p+0219 },   // 5.0e+0081
        { 0x1.94D205CD1EBA9p+0271,  0x0.0F83CE07A31B98p+0219 },   // 6.0e+0081
        { 0x1.D84A5C19F92EFp+0271,  0x0.E76F1B08E8F588p+0219 },   // 7.0e+0081
        { 0x1.0DE1593369D1Bp+0272,  0x0.5FAD34051767B8p+0220 },   // 8.0e+0081
        { 0x1.2F9D8459D70BEp+0272,  0x0.CBA2DA85BA54B0p+0220 },   // 9.0e+0081
        { 0x1.5159AF8044462p+0272,  0x0.379881065D41A8p+0220 },   // 1.0e+0082
        { 0x1.5159AF8044462p+0273,  0x0.379881065D41A8p+0221 },   // 2.0e+0082
        { 0x1.FA06874066693p+0273,  0x0.5364C1898BE280p+0221 },   // 3.0e+0082
        { 0x1.5159AF8044462p+0274,  0x0.379881065D41A8p+0222 },   // 4.0e+0082
        { 0x1.A5B01B605557Ap+0274,  0x0.C57EA147F49218p+0222 },   // 5.0e+0082
        { 0x1.FA06874066693p+0274,  0x0.5364C1898BE280p+0222 },   // 6.0e+0082
        { 0x1.272E79903BBD5p+0275,  0x0.F0A570E5919970p+0223 },   // 7.0e+0082
        { 0x1.5159AF8044462p+0275,  0x0.379881065D41A8p+0223 },   // 8.0e+0082
        { 0x1.7B84E5704CCEEp+0275,  0x0.7E8B912728E9E0p+0223 },   // 9.0e+0082
        { 0x1.A5B01B605557Ap+0275,  0x0.C57EA147F49218p+0223 },   // 1.0e+0083
        { 0x1.A5B01B605557Ap+0276,  0x0.C57EA147F49218p+0224 },   // 2.0e+0083
        { 0x1.3C4414884001Cp+0277,  0x0.141EF8F5F76D90p+0225 },   // 3.0e+0083
        { 0x1.A5B01B605557Ap+0277,  0x0.C57EA147F49218p+0225 },   // 4.0e+0083
        { 0x1.078E111C3556Cp+0278,  0x0.BB6F24CCF8DB48p+0226 },   // 5.0e+0083
        { 0x1.3C4414884001Cp+0278,  0x0.141EF8F5F76D90p+0226 },   // 6.0e+0083
        { 0x1.70FA17F44AACBp+0278,  0x0.6CCECD1EF5FFD0p+0226 },   // 7.0e+0083
        { 0x1.A5B01B605557Ap+0278,  0x0.C57EA147F49218p+0226 },   // 8.0e+0083
        { 0x1.DA661ECC6002Ap+0278,  0x0.1E2E7570F32458p+0226 },   // 9.0e+0083
        { 0x1.078E111C3556Cp+0279,  0x0.BB6F24CCF8DB48p+0227 },   // 1.0e+0084
        { 0x1.078E111C3556Cp+0280,  0x0.BB6F24CCF8DB48p+0228 },   // 2.0e+0084
        { 0x1.8B5519AA50023p+0280,  0x0.1926B7337548F0p+0228 },   // 3.0e+0084
        { 0x1.078E111C3556Cp+0281,  0x0.BB6F24CCF8DB48p+0229 },   // 4.0e+0084
        { 0x1.4971956342AC7p+0281,  0x0.EA4AEE00371220p+0229 },   // 5.0e+0084
        { 0x1.8B5519AA50023p+0281,  0x0.1926B7337548F0p+0229 },   // 6.0e+0084
        { 0x1.CD389DF15D57Ep+0281,  0x0.48028066B37FC8p+0229 },   // 7.0e+0084
        { 0x1.078E111C3556Cp+0282,  0x0.BB6F24CCF8DB48p+0230 },   // 8.0e+0084
        { 0x1.287FD33FBC01Ap+0282,  0x0.52DD096697F6B8p+0230 },   // 9.0e+0084
        { 0x1.4971956342AC7p+0282,  0x0.EA4AEE00371220p+0230 },   // 1.0e+0085
        { 0x1.4971956342AC7p+0283,  0x0.EA4AEE00371220p+0231 },   // 2.0e+0085
        { 0x1.EE2A6014E402Bp+0283,  0x0.DF706500529B30p+0231 },   // 3.0e+0085
        { 0x1.4971956342AC7p+0284,  0x0.EA4AEE00371220p+0232 },   // 4.0e+0085
        { 0x1.9BCDFABC13579p+0284,  0x0.E4DDA98044D6A8p+0232 },   // 5.0e+0085
        { 0x1.EE2A6014E402Bp+0284,  0x0.DF706500529B30p+0232 },   // 6.0e+0085
        { 0x1.204362B6DA56Ep+0285,  0x0.ED019040302FD8p+0233 },   // 7.0e+0085
        { 0x1.4971956342AC7p+0285,  0x0.EA4AEE00371220p+0233 },   // 8.0e+0085
        { 0x1.729FC80FAB020p+0285,  0x0.E7944BC03DF460p+0233 },   // 9.0e+0085
        { 0x1.9BCDFABC13579p+0285,  0x0.E4DDA98044D6A8p+0233 },   // 1.0e+0086
        { 0x1.9BCDFABC13579p+0286,  0x0.E4DDA98044D6A8p+0234 },   // 2.0e+0086
        { 0x1.34DA7C0D0E81Bp+0287,  0x0.6BA63F2033A100p+0235 },   // 3.0e+0086
        { 0x1.9BCDFABC13579p+0287,  0x0.E4DDA98044D6A8p+0235 },   // 4.0e+0086
        { 0x1.0160BCB58C16Cp+0288,  0x0.2F0A89F02B0628p+0236 },   // 5.0e+0086
        { 0x1.34DA7C0D0E81Bp+0288,  0x0.6BA63F2033A100p+0236 },   // 6.0e+0086
        { 0x1.68543B6490ECAp+0288,  0x0.A841F4503C3BD0p+0236 },   // 7.0e+0086
        { 0x1.9BCDFABC13579p+0288,  0x0.E4DDA98044D6A8p+0236 },   // 8.0e+0086
        { 0x1.CF47BA1395C29p+0288,  0x0.21795EB04D7180p+0236 },   // 9.0e+0086
        { 0x1.0160BCB58C16Cp+0289,  0x0.2F0A89F02B0628p+0237 },   // 1.0e+0087
        { 0x1.0160BCB58C16Cp+0290,  0x0.2F0A89F02B0628p+0238 },   // 2.0e+0087
        { 0x1.82111B1052222p+0290,  0x0.468FCEE8408940p+0238 },   // 3.0e+0087
        { 0x1.0160BCB58C16Cp+0291,  0x0.2F0A89F02B0628p+0239 },   // 4.0e+0087
        { 0x1.41B8EBE2EF1C7p+0291,  0x0.3ACD2C6C35C7B0p+0239 },   // 5.0e+0087
        { 0x1.82111B1052222p+0291,  0x0.468FCEE8408940p+0239 },   // 6.0e+0087
        { 0x1.C2694A3DB527Dp+0291,  0x0.525271644B4AC8p+0239 },   // 7.0e+0087
        { 0x1.0160BCB58C16Cp+0292,  0x0.2F0A89F02B0628p+0240 },   // 8.0e+0087
        { 0x1.218CD44C3D999p+0292,  0x0.B4EBDB2E3066F0p+0240 },   // 9.0e+0087
        { 0x1.41B8EBE2EF1C7p+0292,  0x0.3ACD2C6C35C7B0p+0240 },   // 1.0e+0088
        { 0x1.41B8EBE2EF1C7p+0293,  0x0.3ACD2C6C35C7B0p+0241 },   // 2.0e+0088
        { 0x1.E29561D466AAAp+0293,  0x0.D833C2A250AB90p+0241 },   // 3.0e+0088
        { 0x1.41B8EBE2EF1C7p+0294,  0x0.3ACD2C6C35C7B0p+0242 },   // 4.0e+0088
        { 0x1.922726DBAAE39p+0294,  0x0.098077874339A0p+0242 },   // 5.0e+0088
        { 0x1.E29561D466AAAp+0294,  0x0.D833C2A250AB90p+0242 },   // 6.0e+0088
        { 0x1.1981CE669138Ep+0295,  0x0.537386DEAF0EB8p+0243 },   // 7.0e+0088
        { 0x1.41B8EBE2EF1C7p+0295,  0x0.3ACD2C6C35C7B0p+0243 },   // 8.0e+0088
        { 0x1.69F0095F4D000p+0295,  0x0.2226D1F9BC80A8p+0243 },   // 9.0e+0088
        { 0x1.922726DBAAE39p+0295,  0x0.098077874339A0p+0243 },   // 1.0e+0089
        { 0x1.922726DBAAE39p+0296,  0x0.098077874339A0p+0244 },   // 2.0e+0089
        { 0x1.2D9D5D24C02AAp+0297,  0x0.C72059A5726B38p+0245 },   // 3.0e+0089
        { 0x1.922726DBAAE39p+0297,  0x0.098077874339A0p+0245 },   // 4.0e+0089
        { 0x1.F6B0F092959C7p+0297,  0x0.4BE09569140808p+0245 },   // 5.0e+0089
        { 0x1.2D9D5D24C02AAp+0298,  0x0.C72059A5726B38p+0246 },   // 6.0e+0089
        { 0x1.5FE2420035871p+0298,  0x0.E85068965AD268p+0246 },   // 7.0e+0089
        { 0x1.922726DBAAE39p+0298,  0x0.098077874339A0p+0246 },   // 8.0e+0089
        { 0x1.C46C0BB720400p+0298,  0x0.2AB086782BA0D8p+0246 },   // 9.0e+0089
        { 0x1.F6B0F092959C7p+0298,  0x0.4BE09569140808p+0246 },   // 1.0e+0090
        { 0x1.F6B0F092959C7p+0299,  0x0.4BE09569140808p+0247 },   // 2.0e+0090
        { 0x1.7904B46DF0355p+0300,  0x0.78E8700ECF0608p+0248 },   // 3.0e+0090
        { 0x1.F6B0F092959C7p+0300,  0x0.4BE09569140808p+0248 },   // 4.0e+0090
        { 0x1.3A2E965B9D81Cp+0301,  0x0.8F6C5D61AC8508p+0249 },   // 5.0e+0090
        { 0x1.7904B46DF0355p+0301,  0x0.78E8700ECF0608p+0249 },   // 6.0e+0090
        { 0x1.B7DAD28042E8Ep+0301,  0x0.626482BBF18708p+0249 },   // 7.0e+0090
        { 0x1.F6B0F092959C7p+0301,  0x0.4BE09569140808p+0249 },   // 8.0e+0090
        { 0x1.1AC3875274280p+0302,  0x0.1AAE540B1B4480p+0250 },   // 9.0e+0090
        { 0x1.3A2E965B9D81Cp+0302,  0x0.8F6C5D61AC8500p+0250 },   // 1.0e+0091
        { 0x1.3A2E965B9D81Cp+0303,  0x0.8F6C5D61AC8500p+0251 },   // 2.0e+0091
        { 0x1.D745E1896C42Ap+0303,  0x0.D7228C1282C788p+0251 },   // 3.0e+0091
        { 0x1.3A2E965B9D81Cp+0304,  0x0.8F6C5D61AC8500p+0252 },   // 4.0e+0091
        { 0x1.88BA3BF284E23p+0304,  0x0.B34774BA17A648p+0252 },   // 5.0e+0091
        { 0x1.D745E1896C42Ap+0304,  0x0.D7228C1282C788p+0252 },   // 6.0e+0091
        { 0x1.12E8C39029D18p+0305,  0x0.FD7ED1B576F460p+0253 },   // 7.0e+0091
        { 0x1.3A2E965B9D81Cp+0305,  0x0.8F6C5D61AC8500p+0253 },   // 8.0e+0091
        { 0x1.6174692711320p+0305,  0x0.2159E90DE215A8p+0253 },   // 9.0e+0091
        { 0x1.88BA3BF284E23p+0305,  0x0.B34774BA17A648p+0253 },   // 1.0e+0092
        { 0x1.88BA3BF284E23p+0306,  0x0.B34774BA17A648p+0254 },   // 2.0e+0092
        { 0x1.268BACF5E3A9Ap+0307,  0x0.C675978B91BCB0p+0255 },   // 3.0e+0092
        { 0x1.88BA3BF284E23p+0307,  0x0.B34774BA17A648p+0255 },   // 4.0e+0092
        { 0x1.EAE8CAEF261ACp+0307,  0x0.A01951E89D8FD8p+0255 },   // 5.0e+0092
        { 0x1.268BACF5E3A9Ap+0308,  0x0.C675978B91BCB0p+0256 },   // 6.0e+0092
        { 0x1.57A2F4743445Fp+0308,  0x0.3CDE8622D4B180p+0256 },   // 7.0e+0092
        { 0x1.88BA3BF284E23p+0308,  0x0.B34774BA17A648p+0256 },   // 8.0e+0092
        { 0x1.B9D18370D57E8p+0308,  0x0.29B063515A9B10p+0256 },   // 9.0e+0092
        { 0x1.EAE8CAEF261ACp+0308,  0x0.A01951E89D8FD8p+0256 },   // 1.0e+0093
        { 0x1.EAE8CAEF261ACp+0309,  0x0.A01951E89D8FD8p+0257 },   // 2.0e+0093
        { 0x1.702E98335C941p+0310,  0x0.7812FD6E762BE0p+0258 },   // 3.0e+0093
        { 0x1.EAE8CAEF261ACp+0310,  0x0.A01951E89D8FD8p+0258 },   // 4.0e+0093
        { 0x1.32D17ED577D0Bp+0311,  0x0.E40FD3316279E8p+0259 },   // 5.0e+0093
        { 0x1.702E98335C941p+0311,  0x0.7812FD6E762BE0p+0259 },   // 6.0e+0093
        { 0x1.AD8BB19141577p+0311,  0x0.0C1627AB89DDE0p+0259 },   // 7.0e+0093
        { 0x1.EAE8CAEF261ACp+0311,  0x0.A01951E89D8FD8p+0259 },   // 8.0e+0093
        { 0x1.1422F226856F1p+0312,  0x0.1A0E3E12D8A0E8p+0260 },   // 9.0e+0093
        { 0x1.32D17ED577D0Bp+0312,  0x0.E40FD3316279E8p+0260 },   // 1.0e+0094
        { 0x1.32D17ED577D0Bp+0313,  0x0.E40FD3316279E8p+0261 },   // 2.0e+0094
        { 0x1.CC3A3E4033B91p+0313,  0x0.D617BCCA13B6D8p+0261 },   // 3.0e+0094
        { 0x1.32D17ED577D0Bp+0314,  0x0.E40FD3316279E8p+0262 },   // 4.0e+0094
        { 0x1.7F85DE8AD5C4Ep+0314,  0x0.DD13C7FDBB1860p+0262 },   // 5.0e+0094
        { 0x1.CC3A3E4033B91p+0314,  0x0.D617BCCA13B6D8p+0262 },   // 6.0e+0094
        { 0x1.0C774EFAC8D6Ap+0315,  0x0.678DD8CB362AA8p+0263 },   // 7.0e+0094
        { 0x1.32D17ED577D0Bp+0315,  0x0.E40FD3316279E8p+0263 },   // 8.0e+0094
        { 0x1.592BAEB026CADp+0315,  0x0.6091CD978EC920p+0263 },   // 9.0e+0094
        { 0x1.7F85DE8AD5C4Ep+0315,  0x0.DD13C7FDBB1860p+0263 },   // 1.0e+0095
        { 0x1.7F85DE8AD5C4Ep+0316,  0x0.DD13C7FDBB1860p+0264 },   // 2.0e+0095
        { 0x1.1FA466E82053Bp+0317,  0x0.25CED5FE4C5248p+0265 },   // 3.0e+0095
        { 0x1.7F85DE8AD5C4Ep+0317,  0x0.DD13C7FDBB1860p+0265 },   // 4.0e+0095
        { 0x1.DF67562D8B362p+0317,  0x0.9458B9FD29DE78p+0265 },   // 5.0e+0095
        { 0x1.1FA466E82053Bp+0318,  0x0.25CED5FE4C5248p+0266 },   // 6.0e+0095
        { 0x1.4F9522B97B0C5p+0318,  0x0.01714EFE03B550p+0266 },   // 7.0e+0095
        { 0x1.7F85DE8AD5C4Ep+0318,  0x0.DD13C7FDBB1860p+0266 },   // 8.0e+0095
        { 0x1.AF769A5C307D8p+0318,  0x0.B8B640FD727B70p+0266 },   // 9.0e+0095
        { 0x1.DF67562D8B362p+0318,  0x0.9458B9FD29DE78p+0266 },   // 1.0e+0096
        { 0x1.DF67562D8B362p+0319,  0x0.9458B9FD29DE78p+0267 },   // 2.0e+0096
        { 0x1.678D80A228689p+0320,  0x0.EF428B7DDF66D8p+0268 },   // 3.0e+0096
        { 0x1.DF67562D8B362p+0320,  0x0.9458B9FD29DE78p+0268 },   // 4.0e+0096
        { 0x1.2BA095DC7701Dp+0321,  0x0.9CB7743E3A2B08p+0269 },   // 5.0e+0096
        { 0x1.678D80A228689p+0321,  0x0.EF428B7DDF66D8p+0269 },   // 6.0e+0096
        { 0x1.A37A6B67D9CF6p+0321,  0x0.41CDA2BD84A2A8p+0269 },   // 7.0e+0096
        { 0x1.DF67562D8B362p+0321,  0x0.9458B9FD29DE78p+0269 },   // 8.0e+0096
        { 0x1.0DAA20799E4E7p+0322,  0x0.7371E89E678D20p+0270 },   // 9.0e+0096
        { 0x1.2BA095DC7701Dp+0322,  0x0.9CB7743E3A2B08p+0270 },   // 1.0e+0097
        { 0x1.2BA095DC7701Dp+0323,  0x0.9CB7743E3A2B08p+0271 },   // 2.0e+0097
        { 0x1.C170E0CAB282Cp+0323,  0x0.6B132E5D574090p+0271 },   // 3.0e+0097
        { 0x1.2BA095DC7701Dp+0324,  0x0.9CB7743E3A2B08p+0272 },   // 4.0e+0097
        { 0x1.7688BB5394C25p+0324,  0x0.03E5514DC8B5D0p+0272 },   // 5.0e+0097
        { 0x1.C170E0CAB282Cp+0324,  0x0.6B132E5D574090p+0272 },   // 6.0e+0097
        { 0x1.062C8320E8219p+0325,  0x0.E92085B672E5A8p+0273 },   // 7.0e+0097
        { 0x1.2BA095DC7701Dp+0325,  0x0.9CB7743E3A2B08p+0273 },   // 8.0e+0097
        { 0x1.5114A89805E21p+0325,  0x0.504E62C6017070p+0273 },   // 9.0e+0097
        { 0x1.7688BB5394C25p+0325,  0x0.03E5514DC8B5D0p+0273 },   // 1.0e+0098
        { 0x1.7688BB5394C25p+0326,  0x0.03E5514DC8B5D0p+0274 },   // 2.0e+0098
        { 0x1.18E68C7EAF91Bp+0327,  0x0.C2EBFCFA568858p+0275 },   // 3.0e+0098
        { 0x1.7688BB5394C25p+0327,  0x0.03E5514DC8B5D0p+0275 },   // 4.0e+0098
        { 0x1.D42AEA2879F2Ep+0327,  0x0.44DEA5A13AE340p+0275 },   // 5.0e+0098
        { 0x1.18E68C7EAF91Bp+0328,  0x0.C2EBFCFA568858p+0276 },   // 6.0e+0098
        { 0x1.47B7A3E9222A0p+0328,  0x0.6368A7240F9F10p+0276 },   // 7.0e+0098
        { 0x1.7688BB5394C25p+0328,  0x0.03E5514DC8B5D0p+0276 },   // 8.0e+0098
        { 0x1.A559D2BE075A9p+0328,  0x0.A461FB7781CC88p+0276 },   // 9.0e+0098
        { 0x1.D42AEA2879F2Ep+0328,  0x0.44DEA5A13AE340p+0276 },   // 1.0e+0099
        { 0x1.D42AEA2879F2Ep+0329,  0x0.44DEA5A13AE340p+0277 },   // 2.0e+0099
        { 0x1.5F202F9E5B762p+0330,  0x0.B3A6FC38EC2A70p+0278 },   // 3.0e+0099
        { 0x1.D42AEA2879F2Ep+0330,  0x0.44DEA5A13AE340p+0278 },   // 4.0e+0099
        { 0x1.249AD2594C37Cp+0331,  0x0.EB0B2784C4CE08p+0279 },   // 5.0e+0099
        { 0x1.5F202F9E5B762p+0331,  0x0.B3A6FC38EC2A70p+0279 },   // 6.0e+0099
        { 0x1.99A58CE36AB48p+0331,  0x0.7C42D0ED1386D8p+0279 },   // 7.0e+0099
        { 0x1.D42AEA2879F2Ep+0331,  0x0.44DEA5A13AE340p+0279 },   // 8.0e+0099
        { 0x1.075823B6C498Ap+0332,  0x0.06BD3D2AB11FD0p+0280 },   // 9.0e+0099
        { 0x1.249AD2594C37Cp+0332,  0x0.EB0B2784C4CE08p+0280 },   // 1.0e+0100
        { 0x1.249AD2594C37Cp+0333,  0x0.EB0B2784C4CE08p+0281 },   // 2.0e+0100
        { 0x1.B6E83B85F253Bp+0333,  0x0.6090BB47273510p+0281 },   // 3.0e+0100
        { 0x1.249AD2594C37Cp+0334,  0x0.EB0B2784C4CE08p+0282 },   // 4.0e+0100
        { 0x1.6DC186EF9F45Cp+0334,  0x0.25CDF165F60188p+0282 },   // 5.0e+0100
        { 0x1.B6E83B85F253Bp+0334,  0x0.6090BB47273510p+0282 },   // 6.0e+0100
        { 0x1.0007780E22B0Dp+0335,  0x0.4DA9C2942C3448p+0283 },   // 7.0e+0100
        { 0x1.249AD2594C37Cp+0335,  0x0.EB0B2784C4CE08p+0283 },   // 8.0e+0100
        { 0x1.492E2CA475BECp+0335,  0x0.886C8C755D67C8p+0283 },   // 9.0e+0100
        { 0x1.6DC186EF9F45Cp+0335,  0x0.25CDF165F60188p+0283 },   // 1.0e+0101
        { 0x1.6DC186EF9F45Cp+0336,  0x0.25CDF165F60188p+0284 },   // 2.0e+0101
        { 0x1.12512533B7745p+0337,  0x0.1C5A750C788128p+0285 },   // 3.0e+0101
        { 0x1.6DC186EF9F45Cp+0337,  0x0.25CDF165F60188p+0285 },   // 4.0e+0101
        { 0x1.C931E8AB87173p+0337,  0x0.2F416DBF7381F0p+0285 },   // 5.0e+0101
        { 0x1.12512533B7745p+0338,  0x0.1C5A750C788128p+0286 },   // 6.0e+0101
        { 0x1.40095611AB5D0p+0338,  0x0.A1143339374158p+0286 },   // 7.0e+0101
        { 0x1.6DC186EF9F45Cp+0338,  0x0.25CDF165F60188p+0286 },   // 8.0e+0101
        { 0x1.9B79B7CD932E7p+0338,  0x0.AA87AF92B4C1C0p+0286 },   // 9.0e+0101
        { 0x1.C931E8AB87173p+0338,  0x0.2F416DBF7381F0p+0286 },   // 1.0e+0102
        { 0x1.C931E8AB87173p+0339,  0x0.2F416DBF7381F0p+0287 },   // 2.0e+0102
        { 0x1.56E56E80A5516p+0340,  0x0.6371124F96A170p+0288 },   // 3.0e+0102
        { 0x1.C931E8AB87173p+0340,  0x0.2F416DBF7381F0p+0288 },   // 4.0e+0102
        { 0x1.1DBF316B346E7p+0341,  0x0.FD88E497A83130p+0289 },   // 5.0e+0102
        { 0x1.56E56E80A5516p+0341,  0x0.6371124F96A170p+0289 },   // 6.0e+0102
        { 0x1.900BAB9616344p+0341,  0x0.C95940078511B0p+0289 },   // 7.0e+0102
        { 0x1.C931E8AB87173p+0341,  0x0.2F416DBF7381F0p+0289 },   // 8.0e+0102
        { 0x1.012C12E07BFD0p+0342,  0x0.CA94CDBBB0F918p+0290 },   // 9.0e+0102
        { 0x1.1DBF316B346E7p+0342,  0x0.FD88E497A83130p+0290 },   // 1.0e+0103
        { 0x1.1DBF316B346E7p+0343,  0x0.FD88E497A83130p+0291 },   // 2.0e+0103
        { 0x1.AC9ECA20CEA5Bp+0343,  0x0.FC4D56E37C49D0p+0291 },   // 3.0e+0103
        { 0x1.1DBF316B346E7p+0344,  0x0.FD88E497A83130p+0292 },   // 4.0e+0103
        { 0x1.652EFDC6018A1p+0344,  0x0.FCEB1DBD923D80p+0292 },   // 5.0e+0103
        { 0x1.AC9ECA20CEA5Bp+0344,  0x0.FC4D56E37C49D0p+0292 },   // 6.0e+0103
        { 0x1.F40E967B9BC15p+0344,  0x0.FBAF9009665620p+0292 },   // 7.0e+0103
        { 0x1.1DBF316B346E7p+0345,  0x0.FD88E497A83130p+0293 },   // 8.0e+0103
        { 0x1.417717989AFC4p+0345,  0x0.FD3A012A9D3758p+0293 },   // 9.0e+0103
        { 0x1.652EFDC6018A1p+0345,  0x0.FCEB1DBD923D80p+0293 },   // 1.0e+0104
        { 0x1.652EFDC6018A1p+0346,  0x0.FCEB1DBD923D80p+0294 },   // 2.0e+0104
        { 0x1.0BE33E5481279p+0347,  0x0.7DB0564E2DAE20p+0295 },   // 3.0e+0104
        { 0x1.652EFDC6018A1p+0347,  0x0.FCEB1DBD923D80p+0295 },   // 4.0e+0104
        { 0x1.BE7ABD3781ECAp+0347,  0x0.7C25E52CF6CCE0p+0295 },   // 5.0e+0104
        { 0x1.0BE33E5481279p+0348,  0x0.7DB0564E2DAE20p+0296 },   // 6.0e+0104
        { 0x1.38891E0D4158Dp+0348,  0x0.BD4DBA05DFF5D0p+0296 },   // 7.0e+0104
        { 0x1.652EFDC6018A1p+0348,  0x0.FCEB1DBD923D80p+0296 },   // 8.0e+0104
        { 0x1.91D4DD7EC1BB6p+0348,  0x0.3C888175448530p+0296 },   // 9.0e+0104
        { 0x1.BE7ABD3781ECAp+0348,  0x0.7C25E52CF6CCE0p+0296 },   // 1.0e+0105
        { 0x1.BE7ABD3781ECAp+0349,  0x0.7C25E52CF6CCE0p+0297 },   // 2.0e+0105
        { 0x1.4EDC0DE9A1717p+0350,  0x0.DD1C6BE1B919A8p+0298 },   // 3.0e+0105
        { 0x1.BE7ABD3781ECAp+0350,  0x0.7C25E52CF6CCE0p+0298 },   // 4.0e+0105
        { 0x1.170CB642B133Ep+0351,  0x0.8D97AF3C1A4010p+0299 },   // 5.0e+0105
        { 0x1.4EDC0DE9A1717p+0351,  0x0.DD1C6BE1B919A8p+0299 },   // 6.0e+0105
        { 0x1.86AB659091AF1p+0351,  0x0.2CA1288757F348p+0299 },   // 7.0e+0105
        { 0x1.BE7ABD3781ECAp+0351,  0x0.7C25E52CF6CCE0p+0299 },   // 8.0e+0105
        { 0x1.F64A14DE722A3p+0351,  0x0.CBAAA1D295A680p+0299 },   // 9.0e+0105
        { 0x1.170CB642B133Ep+0352,  0x0.8D97AF3C1A4010p+0300 },   // 1.0e+0106
        { 0x1.170CB642B133Ep+0353,  0x0.8D97AF3C1A4010p+0301 },   // 2.0e+0106
        { 0x1.A293116409CDDp+0353,  0x0.D46386DA276018p+0301 },   // 3.0e+0106
        { 0x1.170CB642B133Ep+0354,  0x0.8D97AF3C1A4010p+0302 },   // 4.0e+0106
        { 0x1.5CCFE3D35D80Ep+0354,  0x0.30FD9B0B20D010p+0302 },   // 5.0e+0106
        { 0x1.A293116409CDDp+0354,  0x0.D46386DA276018p+0302 },   // 6.0e+0106
        { 0x1.E8563EF4B61ADp+0354,  0x0.77C972A92DF018p+0302 },   // 7.0e+0106
        { 0x1.170CB642B133Ep+0355,  0x0.8D97AF3C1A4010p+0303 },   // 8.0e+0106
        { 0x1.39EE4D0B075A6p+0355,  0x0.5F4AA5239D8810p+0303 },   // 9.0e+0106
        { 0x1.5CCFE3D35D80Ep+0355,  0x0.30FD9B0B20D010p+0303 },   // 1.0e+0107
        { 0x1.5CCFE3D35D80Ep+0356,  0x0.30FD9B0B20D010p+0304 },   // 2.0e+0107
        { 0x1.059BEADE8620Ap+0357,  0x0.A4BE3448589C08p+0305 },   // 3.0e+0107
        { 0x1.5CCFE3D35D80Ep+0357,  0x0.30FD9B0B20D010p+0305 },   // 4.0e+0107
        { 0x1.B403DCC834E11p+0357,  0x0.BD3D01CDE90418p+0305 },   // 5.0e+0107
        { 0x1.059BEADE8620Ap+0358,  0x0.A4BE3448589C08p+0306 },   // 6.0e+0107
        { 0x1.3135E758F1D0Cp+0358,  0x0.6ADDE7A9BCB610p+0306 },   // 7.0e+0107
        { 0x1.5CCFE3D35D80Ep+0358,  0x0.30FD9B0B20D010p+0306 },   // 8.0e+0107
        { 0x1.8869E04DC930Fp+0358,  0x0.F71D4E6C84EA10p+0306 },   // 9.0e+0107
        { 0x1.B403DCC834E11p+0358,  0x0.BD3D01CDE90418p+0306 },   // 1.0e+0108
        { 0x1.B403DCC834E11p+0359,  0x0.BD3D01CDE90418p+0307 },   // 2.0e+0108
        { 0x1.4702E59627A8Dp+0360,  0x0.4DEDC15A6EC310p+0308 },   // 3.0e+0108
        { 0x1.B403DCC834E11p+0360,  0x0.BD3D01CDE90418p+0308 },   // 4.0e+0108
        { 0x1.108269FD210CBp+0361,  0x0.16462120B1A290p+0309 },   // 5.0e+0108
        { 0x1.4702E59627A8Dp+0361,  0x0.4DEDC15A6EC310p+0309 },   // 6.0e+0108
        { 0x1.7D83612F2E44Fp+0361,  0x0.859561942BE390p+0309 },   // 7.0e+0108
        { 0x1.B403DCC834E11p+0361,  0x0.BD3D01CDE90418p+0309 },   // 8.0e+0108
        { 0x1.EA8458613B7D3p+0361,  0x0.F4E4A207A62498p+0309 },   // 9.0e+0108
        { 0x1.108269FD210CBp+0362,  0x0.16462120B1A290p+0310 },   // 1.0e+0109
        { 0x1.108269FD210CBp+0363,  0x0.16462120B1A290p+0311 },   // 2.0e+0109
        { 0x1.98C39EFBB1930p+0363,  0x0.A16931B10A73D8p+0311 },   // 3.0e+0109
        { 0x1.108269FD210CBp+0364,  0x0.16462120B1A290p+0312 },   // 4.0e+0109
        { 0x1.54A3047C694FDp+0364,  0x0.DBD7A968DE0B30p+0312 },   // 5.0e+0109
        { 0x1.98C39EFBB1930p+0364,  0x0.A16931B10A73D8p+0312 },   // 6.0e+0109
        { 0x1.DCE4397AF9D63p+0364,  0x0.66FAB9F936DC78p+0312 },   // 7.0e+0109
        { 0x1.108269FD210CBp+0365,  0x0.16462120B1A290p+0313 },   // 8.0e+0109
        { 0x1.3292B73CC52E4p+0365,  0x0.790EE544C7D6E0p+0313 },   // 9.0e+0109
        { 0x1.54A3047C694FDp+0365,  0x0.DBD7A968DE0B30p+0313 },   // 1.0e+0110
        { 0x1.54A3047C694FDp+0366,  0x0.DBD7A968DE0B30p+0314 },   // 2.0e+0110
        { 0x1.FEF486BA9DF7Cp+0366,  0x0.C9C37E1D4D10C8p+0314 },   // 3.0e+0110
        { 0x1.54A3047C694FDp+0367,  0x0.DBD7A968DE0B30p+0315 },   // 4.0e+0110
        { 0x1.A9CBC59B83A3Dp+0367,  0x0.52CD93C3158E00p+0315 },   // 5.0e+0110
        { 0x1.FEF486BA9DF7Cp+0367,  0x0.C9C37E1D4D10C8p+0315 },   // 6.0e+0110
        { 0x1.2A0EA3ECDC25Ep+0368,  0x0.205CB43BC249C8p+0316 },   // 7.0e+0110
        { 0x1.54A3047C694FDp+0368,  0x0.DBD7A968DE0B30p+0316 },   // 8.0e+0110
        { 0x1.7F37650BF679Dp+0368,  0x0.97529E95F9CC98p+0316 },   // 9.0e+0110
        { 0x1.A9CBC59B83A3Dp+0368,  0x0.52CD93C3158E00p+0316 },   // 1.0e+0111
        { 0x1.A9CBC59B83A3Dp+0369,  0x0.52CD93C3158E00p+0317 },   // 2.0e+0111
        { 0x1.3F58D434A2BADp+0370,  0x0.FE1A2ED2502A80p+0318 },   // 3.0e+0111
        { 0x1.A9CBC59B83A3Dp+0370,  0x0.52CD93C3158E00p+0318 },   // 4.0e+0111
        { 0x1.0A1F5B8132466p+0371,  0x0.53C07C59ED78C0p+0319 },   // 5.0e+0111
        { 0x1.3F58D434A2BADp+0371,  0x0.FE1A2ED2502A80p+0319 },   // 6.0e+0111
        { 0x1.74924CE8132F5p+0371,  0x0.A873E14AB2DC40p+0319 },   // 7.0e+0111
        { 0x1.A9CBC59B83A3Dp+0371,  0x0.52CD93C3158E00p+0319 },   // 8.0e+0111
        { 0x1.DF053E4EF4184p+0371,  0x0.FD27463B783FC0p+0319 },   // 9.0e+0111
        { 0x1.0A1F5B8132466p+0372,  0x0.53C07C59ED78C0p+0320 },   // 1.0e+0112
        { 0x1.0A1F5B8132466p+0373,  0x0.53C07C59ED78C0p+0321 },   // 2.0e+0112
        { 0x1.8F2F0941CB699p+0373,  0x0.7DA0BA86E43520p+0321 },   // 3.0e+0112
        { 0x1.0A1F5B8132466p+0374,  0x0.53C07C59ED78C0p+0322 },   // 4.0e+0112
        { 0x1.4CA732617ED7Fp+0374,  0x0.E8B09B7068D6F0p+0322 },   // 5.0e+0112
        { 0x1.8F2F0941CB699p+0374,  0x0.7DA0BA86E43520p+0322 },   // 6.0e+0112
        { 0x1.D1B6E02217FB3p+0374,  0x0.1290D99D5F9350p+0322 },   // 7.0e+0112
        { 0x1.0A1F5B8132466p+0375,  0x0.53C07C59ED78C0p+0323 },   // 8.0e+0112
        { 0x1.2B6346F1588F3p+0375,  0x0.1E388BE52B27D8p+0323 },   // 9.0e+0112
        { 0x1.4CA732617ED7Fp+0375,  0x0.E8B09B7068D6F0p+0323 },   // 1.0e+0113
        { 0x1.4CA732617ED7Fp+0376,  0x0.E8B09B7068D6F0p+0324 },   // 2.0e+0113
        { 0x1.F2FACB923E43Fp+0376,  0x0.DD08E9289D4268p+0324 },   // 3.0e+0113
        { 0x1.4CA732617ED7Fp+0377,  0x0.E8B09B7068D6F0p+0325 },   // 4.0e+0113
        { 0x1.9FD0FEF9DE8DFp+0377,  0x0.E2DCC24C830CA8p+0325 },   // 5.0e+0113
        { 0x1.F2FACB923E43Fp+0377,  0x0.DD08E9289D4268p+0325 },   // 6.0e+0113
        { 0x1.23124C154EFCFp+0378,  0x0.EB9A88025BBC10p+0326 },   // 7.0e+0113
        { 0x1.4CA732617ED7Fp+0378,  0x0.E8B09B7068D6F0p+0326 },   // 8.0e+0113
        { 0x1.763C18ADAEB2Fp+0378,  0x0.E5C6AEDE75F1C8p+0326 },   // 9.0e+0113
        { 0x1.9FD0FEF9DE8DFp+0378,  0x0.E2DCC24C830CA8p+0326 },   // 1.0e+0114
        { 0x1.9FD0FEF9DE8DFp+0379,  0x0.E2DCC24C830CA8p+0327 },   // 2.0e+0114
        { 0x1.37DCBF3B66EA7p+0380,  0x0.EA2591B9624980p+0328 },   // 3.0e+0114
        { 0x1.9FD0FEF9DE8DFp+0380,  0x0.E2DCC24C830CA8p+0328 },   // 4.0e+0114
        { 0x1.03E29F5C2B18Bp+0381,  0x0.EDC9F96FD1E7E8p+0329 },   // 5.0e+0114
        { 0x1.37DCBF3B66EA7p+0381,  0x0.EA2591B9624980p+0329 },   // 6.0e+0114
        { 0x1.6BD6DF1AA2BC3p+0381,  0x0.E6812A02F2AB10p+0329 },   // 7.0e+0114
        { 0x1.9FD0FEF9DE8DFp+0381,  0x0.E2DCC24C830CA8p+0329 },   // 8.0e+0114
        { 0x1.D3CB1ED91A5FBp+0381,  0x0.DF385A96136E40p+0329 },   // 9.0e+0114
        { 0x1.03E29F5C2B18Bp+0382,  0x0.EDC9F96FD1E7E8p+0330 },   // 1.0e+0115
        { 0x1.03E29F5C2B18Bp+0383,  0x0.EDC9F96FD1E7E8p+0331 },   // 2.0e+0115
        { 0x1.85D3EF0A40A51p+0383,  0x0.E4AEF627BADBE0p+0331 },   // 3.0e+0115
        { 0x1.03E29F5C2B18Bp+0384,  0x0.EDC9F96FD1E7E8p+0332 },   // 4.0e+0115
        { 0x1.44DB473335DEEp+0384,  0x0.E93C77CBC661E0p+0332 },   // 5.0e+0115
        { 0x1.85D3EF0A40A51p+0384,  0x0.E4AEF627BADBE0p+0332 },   // 6.0e+0115
        { 0x1.C6CC96E14B6B4p+0384,  0x0.E0217483AF55D8p+0332 },   // 7.0e+0115
        { 0x1.03E29F5C2B18Bp+0385,  0x0.EDC9F96FD1E7E8p+0333 },   // 8.0e+0115
        { 0x1.245EF347B07BDp+0385,  0x0.6B83389DCC24E8p+0333 },   // 9.0e+0115
        { 0x1.44DB473335DEEp+0385,  0x0.E93C77CBC661E0p+0333 },   // 1.0e+0116
        { 0x1.44DB473335DEEp+0386,  0x0.E93C77CBC661E0p+0334 },   // 2.0e+0116
        { 0x1.E748EACCD0CE6p+0386,  0x0.5DDAB3B1A992D8p+0334 },   // 3.0e+0116
        { 0x1.44DB473335DEEp+0387,  0x0.E93C77CBC661E0p+0335 },   // 4.0e+0116
        { 0x1.961219000356Ap+0387,  0x0.A38B95BEB7FA60p+0335 },   // 5.0e+0116
        { 0x1.E748EACCD0CE6p+0387,  0x0.5DDAB3B1A992D8p+0335 },   // 6.0e+0116
        { 0x1.1C3FDE4CCF231p+0388,  0x0.0C14E8D24D95A8p+0336 },   // 7.0e+0116
        { 0x1.44DB473335DEEp+0388,  0x0.E93C77CBC661E0p+0336 },   // 8.0e+0116
        { 0x1.6D76B0199C9ACp+0388,  0x0.C66406C53F2E20p+0336 },   // 9.0e+0116
        { 0x1.961219000356Ap+0388,  0x0.A38B95BEB7FA60p+0336 },   // 1.0e+0117
        { 0x1.961219000356Ap+0389,  0x0.A38B95BEB7FA60p+0337 },   // 2.0e+0117
        { 0x1.308D92C00280Fp+0390,  0x0.FAA8B04F09FBC8p+0338 },   // 3.0e+0117
        { 0x1.961219000356Ap+0390,  0x0.A38B95BEB7FA60p+0338 },   // 4.0e+0117
        { 0x1.FB969F40042C5p+0390,  0x0.4C6E7B2E65F8F8p+0338 },   // 5.0e+0117
        { 0x1.308D92C00280Fp+0391,  0x0.FAA8B04F09FBC8p+0339 },   // 6.0e+0117
        { 0x1.634FD5E002EBDp+0391,  0x0.4F1A2306E0FB10p+0339 },   // 7.0e+0117
        { 0x1.961219000356Ap+0391,  0x0.A38B95BEB7FA60p+0339 },   // 8.0e+0117
        { 0x1.C8D45C2003C17p+0391,  0x0.F7FD08768EF9A8p+0339 },   // 9.0e+0117
        { 0x1.FB969F40042C5p+0391,  0x0.4C6E7B2E65F8F8p+0339 },   // 1.0e+0118
        { 0x1.FB969F40042C5p+0392,  0x0.4C6E7B2E65F8F8p+0340 },   // 2.0e+0118
        { 0x1.7CB0F77003213p+0393,  0x0.F952DC62CC7AB8p+0341 },   // 3.0e+0118
        { 0x1.FB969F40042C5p+0393,  0x0.4C6E7B2E65F8F8p+0341 },   // 4.0e+0118
        { 0x1.3D3E2388029BBp+0394,  0x0.4FC50CFCFFBB98p+0342 },   // 5.0e+0118
        { 0x1.7CB0F77003213p+0394,  0x0.F952DC62CC7AB8p+0342 },   // 6.0e+0118
        { 0x1.BC23CB5803A6Cp+0394,  0x0.A2E0ABC89939D8p+0342 },   // 7.0e+0118
        { 0x1.FB969F40042C5p+0394,  0x0.4C6E7B2E65F8F8p+0342 },   // 8.0e+0118
        { 0x1.1D84B9940258Ep+0395,  0x0.FAFE254A195C08p+0343 },   // 9.0e+0118
        { 0x1.3D3E2388029BBp+0395,  0x0.4FC50CFCFFBB98p+0343 },   // 1.0e+0119
        { 0x1.3D3E2388029BBp+0396,  0x0.4FC50CFCFFBB98p+0344 },   // 2.0e+0119
        { 0x1.DBDD354C03E98p+0396,  0x0.F7A7937B7F9968p+0344 },   // 3.0e+0119
        { 0x1.3D3E2388029BBp+0397,  0x0.4FC50CFCFFBB98p+0345 },   // 4.0e+0119
        { 0x1.8C8DAC6A0342Ap+0397,  0x0.23B6503C3FAA80p+0345 },   // 5.0e+0119
        { 0x1.DBDD354C03E98p+0397,  0x0.F7A7937B7F9968p+0345 },   // 6.0e+0119
        { 0x1.15965F1702483p+0398,  0x0.E5CC6B5D5FC428p+0346 },   // 7.0e+0119
        { 0x1.3D3E2388029BBp+0398,  0x0.4FC50CFCFFBB98p+0346 },   // 8.0e+0119
        { 0x1.64E5E7F902EF2p+0398,  0x0.B9BDAE9C9FB308p+0346 },   // 9.0e+0119
        { 0x1.8C8DAC6A0342Ap+0398,  0x0.23B6503C3FAA80p+0346 },   // 1.0e+0120
        { 0x1.8C8DAC6A0342Ap+0399,  0x0.23B6503C3FAA80p+0347 },   // 2.0e+0120
        { 0x1.296A414F8271Fp+0400,  0x0.9AC8BC2D2FBFE0p+0348 },   // 3.0e+0120
        { 0x1.8C8DAC6A0342Ap+0400,  0x0.23B6503C3FAA80p+0348 },   // 4.0e+0120
        { 0x1.EFB1178484134p+0400,  0x0.ACA3E44B4F9520p+0348 },   // 5.0e+0120
        { 0x1.296A414F8271Fp+0401,  0x0.9AC8BC2D2FBFE0p+0349 },   // 6.0e+0120
        { 0x1.5AFBF6DCC2DA4p+0401,  0x0.DF3F8634B7B530p+0349 },   // 7.0e+0120
        { 0x1.8C8DAC6A0342Ap+0401,  0x0.23B6503C3FAA80p+0349 },   // 8.0e+0120
        { 0x1.BE1F61F743AAFp+0401,  0x0.682D1A43C79FD0p+0349 },   // 9.0e+0120
        { 0x1.EFB1178484134p+0401,  0x0.ACA3E44B4F9520p+0349 },   // 1.0e+0121
        { 0x1.EFB1178484134p+0402,  0x0.ACA3E44B4F9520p+0350 },   // 2.0e+0121
        { 0x1.73C4D1A3630E7p+0403,  0x0.817AEB387BAFD8p+0351 },   // 3.0e+0121
        { 0x1.EFB1178484134p+0403,  0x0.ACA3E44B4F9520p+0351 },   // 4.0e+0121
        { 0x1.35CEAEB2D28C0p+0404,  0x0.EBE66EAF11BD30p+0352 },   // 5.0e+0121
        { 0x1.73C4D1A3630E7p+0404,  0x0.817AEB387BAFD8p+0352 },   // 6.0e+0121
        { 0x1.B1BAF493F390Ep+0404,  0x0.170F67C1E5A278p+0352 },   // 7.0e+0121
        { 0x1.EFB1178484134p+0404,  0x0.ACA3E44B4F9520p+0352 },   // 8.0e+0121
        { 0x1.16D39D3A8A4ADp+0405,  0x0.A11C306A5CC3E0p+0353 },   // 9.0e+0121
        { 0x1.35CEAEB2D28C0p+0405,  0x0.EBE66EAF11BD30p+0353 },   // 1.0e+0122
        { 0x1.35CEAEB2D28C0p+0406,  0x0.EBE66EAF11BD30p+0354 },   // 2.0e+0122
        { 0x1.D0B6060C3BD21p+0406,  0x0.61D9A6069A9BD0p+0354 },   // 3.0e+0122
        { 0x1.35CEAEB2D28C0p+0407,  0x0.EBE66EAF11BD30p+0355 },   // 4.0e+0122
        { 0x1.83425A5F872F1p+0407,  0x0.26E00A5AD62C80p+0355 },   // 5.0e+0122
        { 0x1.D0B6060C3BD21p+0407,  0x0.61D9A6069A9BD0p+0355 },   // 6.0e+0122
        { 0x1.0F14D8DC783A8p+0408,  0x0.CE69A0D92F8588p+0356 },   // 7.0e+0122
        { 0x1.35CEAEB2D28C0p+0408,  0x0.EBE66EAF11BD30p+0356 },   // 8.0e+0122
        { 0x1.5C8884892CDD9p+0408,  0x0.09633C84F3F4D8p+0356 },   // 9.0e+0122
        { 0x1.83425A5F872F1p+0408,  0x0.26E00A5AD62C80p+0356 },   // 1.0e+0123
        { 0x1.83425A5F872F1p+0409,  0x0.26E00A5AD62C80p+0357 },   // 2.0e+0123
        { 0x1.2271C3C7A5634p+0410,  0x0.DD2807C420A160p+0358 },   // 3.0e+0123
        { 0x1.83425A5F872F1p+0410,  0x0.26E00A5AD62C80p+0358 },   // 4.0e+0123
        { 0x1.E412F0F768FADp+0410,  0x0.70980CF18BB7A0p+0358 },   // 5.0e+0123
        { 0x1.2271C3C7A5634p+0411,  0x0.DD2807C420A160p+0359 },   // 6.0e+0123
        { 0x1.52DA0F1396493p+0411,  0x0.0204090F7B66F0p+0359 },   // 7.0e+0123
        { 0x1.83425A5F872F1p+0411,  0x0.26E00A5AD62C80p+0359 },   // 8.0e+0123
        { 0x1.B3AAA5AB7814Fp+0411,  0x0.4BBC0BA630F210p+0359 },   // 9.0e+0123
        { 0x1.E412F0F768FADp+0411,  0x0.70980CF18BB7A0p+0359 },   // 1.0e+0124
        { 0x1.E412F0F768FADp+0412,  0x0.70980CF18BB7A0p+0360 },   // 2.0e+0124
        { 0x1.6B0E34B98EBC2p+0413,  0x0.147209B528C9B8p+0361 },   // 3.0e+0124
        { 0x1.E412F0F768FADp+0413,  0x0.70980CF18BB7A0p+0361 },   // 4.0e+0124
        { 0x1.2E8BD69AA19CCp+0414,  0x0.665F0816F752C0p+0362 },   // 5.0e+0124
        { 0x1.6B0E34B98EBC2p+0414,  0x0.147209B528C9B8p+0362 },   // 6.0e+0124
        { 0x1.A79092D87BDB7p+0414,  0x0.C2850B535A40A8p+0362 },   // 7.0e+0124
        { 0x1.E412F0F768FADp+0414,  0x0.70980CF18BB7A0p+0362 },   // 8.0e+0124
        { 0x1.104AA78B2B0D1p+0415,  0x0.8F558747DE9748p+0363 },   // 9.0e+0124
        { 0x1.2E8BD69AA19CCp+0415,  0x0.665F0816F752C0p+0363 },   // 1.0e+0125
        { 0x1.2E8BD69AA19CCp+0416,  0x0.665F0816F752C0p+0364 },   // 2.0e+0125
        { 0x1.C5D1C1E7F26B2p+0416,  0x0.998E8C2272FC28p+0364 },   // 3.0e+0125
        { 0x1.2E8BD69AA19CCp+0417,  0x0.665F0816F752C0p+0365 },   // 4.0e+0125
        { 0x1.7A2ECC414A03Fp+0417,  0x0.7FF6CA1CB52778p+0365 },   // 5.0e+0125
        { 0x1.C5D1C1E7F26B2p+0417,  0x0.998E8C2272FC28p+0365 },   // 6.0e+0125
        { 0x1.08BA5BC74D692p+0418,  0x0.D9932714186868p+0366 },   // 7.0e+0125
        { 0x1.2E8BD69AA19CCp+0418,  0x0.665F0816F752C0p+0366 },   // 8.0e+0125
        { 0x1.545D516DF5D05p+0418,  0x0.F32AE919D63D18p+0366 },   // 9.0e+0125
        { 0x1.7A2ECC414A03Fp+0418,  0x0.7FF6CA1CB52778p+0366 },   // 1.0e+0126
        { 0x1.7A2ECC414A03Fp+0419,  0x0.7FF6CA1CB52778p+0367 },   // 2.0e+0126
        { 0x1.1BA31930F782Fp+0420,  0x0.9FF9179587DD98p+0368 },   // 3.0e+0126
        { 0x1.7A2ECC414A03Fp+0420,  0x0.7FF6CA1CB52778p+0368 },   // 4.0e+0126
        { 0x1.D8BA7F519C84Fp+0420,  0x0.5FF47CA3E27150p+0368 },   // 5.0e+0126
        { 0x1.1BA31930F782Fp+0421,  0x0.9FF9179587DD98p+0369 },   // 6.0e+0126
        { 0x1.4AE8F2B920C37p+0421,  0x0.8FF7F0D91E8288p+0369 },   // 7.0e+0126
        { 0x1.7A2ECC414A03Fp+0421,  0x0.7FF6CA1CB52778p+0369 },   // 8.0e+0126
        { 0x1.A974A5C973447p+0421,  0x0.6FF5A3604BCC60p+0369 },   // 9.0e+0126
        { 0x1.D8BA7F519C84Fp+0421,  0x0.5FF47CA3E27150p+0369 },   // 1.0e+0127
        { 0x1.D8BA7F519C84Fp+0422,  0x0.5FF47CA3E27150p+0370 },   // 2.0e+0127
        { 0x1.628BDF7D3563Bp+0423,  0x0.87F75D7AE9D500p+0371 },   // 3.0e+0127
        { 0x1.D8BA7F519C84Fp+0423,  0x0.5FF47CA3E27150p+0371 },   // 4.0e+0127
        { 0x1.27748F9301D31p+0424,  0x0.9BF8CDE66D86D0p+0372 },   // 5.0e+0127
        { 0x1.628BDF7D3563Bp+0424,  0x0.87F75D7AE9D500p+0372 },   // 6.0e+0127
        { 0x1.9DA32F6768F45p+0424,  0x0.73F5ED0F662328p+0372 },   // 7.0e+0127
        { 0x1.D8BA7F519C84Fp+0424,  0x0.5FF47CA3E27150p+0372 },   // 8.0e+0127
        { 0x1.09E8E79DE80ACp+0425,  0x0.A5F9861C2F5FC0p+0373 },   // 9.0e+0127
        { 0x1.27748F9301D31p+0425,  0x0.9BF8CDE66D86D0p+0373 },   // 1.0e+0128
        { 0x1.27748F9301D31p+0426,  0x0.9BF8CDE66D86D0p+0374 },   // 2.0e+0128
        { 0x1.BB2ED75C82BCAp+0426,  0x0.69F534D9A44A40p+0374 },   // 3.0e+0128
        { 0x1.27748F9301D31p+0427,  0x0.9BF8CDE66D86D0p+0375 },   // 4.0e+0128
        { 0x1.7151B377C247Ep+0427,  0x0.02F7016008E888p+0375 },   // 5.0e+0128
        { 0x1.BB2ED75C82BCAp+0427,  0x0.69F534D9A44A40p+0375 },   // 6.0e+0128
        { 0x1.0285FDA0A198Bp+0428,  0x0.6879B4299FD5F8p+0376 },   // 7.0e+0128
        { 0x1.27748F9301D31p+0428,  0x0.9BF8CDE66D86D0p+0376 },   // 8.0e+0128
        { 0x1.4C632185620D7p+0428,  0x0.CF77E7A33B37B0p+0376 },   // 9.0e+0128
        { 0x1.7151B377C247Ep+0428,  0x0.02F7016008E888p+0376 },   // 1.0e+0129
        { 0x1.7151B377C247Ep+0429,  0x0.02F7016008E888p+0377 },   // 2.0e+0129
        { 0x1.14FD4699D1B5Ep+0430,  0x0.8239410806AE68p+0378 },   // 3.0e+0129
        { 0x1.7151B377C247Ep+0430,  0x0.02F7016008E888p+0378 },   // 4.0e+0129
        { 0x1.CDA62055B2D9Dp+0430,  0x0.83B4C1B80B22A8p+0378 },   // 5.0e+0129
        { 0x1.14FD4699D1B5Ep+0431,  0x0.8239410806AE68p+0379 },   // 6.0e+0129
        { 0x1.43277D08C9FEEp+0431,  0x0.4298213407CB78p+0379 },   // 7.0e+0129
        { 0x1.7151B377C247Ep+0431,  0x0.02F7016008E888p+0379 },   // 8.0e+0129
        { 0x1.9F7BE9E6BA90Dp+0431,  0x0.C355E18C0A0598p+0379 },   // 9.0e+0129
        { 0x1.CDA62055B2D9Dp+0431,  0x0.83B4C1B80B22A8p+0379 },   // 1.0e+0130
        { 0x1.CDA62055B2D9Dp+0432,  0x0.83B4C1B80B22A8p+0380 },   // 2.0e+0130
        { 0x1.5A3C984046236p+0433,  0x0.22C7914A085A00p+0381 },   // 3.0e+0130
        { 0x1.CDA62055B2D9Dp+0433,  0x0.83B4C1B80B22A8p+0381 },   // 4.0e+0130
        { 0x1.2087D4358FC82p+0434,  0x0.7250F91306F5A8p+0382 },   // 5.0e+0130
        { 0x1.5A3C984046236p+0434,  0x0.22C7914A085A00p+0382 },   // 6.0e+0130
        { 0x1.93F15C4AFC7E9p+0434,  0x0.D33E298109BE58p+0382 },   // 7.0e+0130
        { 0x1.CDA62055B2D9Dp+0434,  0x0.83B4C1B80B22A8p+0382 },   // 8.0e+0130
        { 0x1.03AD7230349A8p+0435,  0x0.9A15ACF7864380p+0383 },   // 9.0e+0130
        { 0x1.2087D4358FC82p+0435,  0x0.7250F91306F5A8p+0383 },   // 1.0e+0131
        { 0x1.2087D4358FC82p+0436,  0x0.7250F91306F5A8p+0384 },   // 2.0e+0131
        { 0x1.B0CBBE5057AC3p+0436,  0x0.AB79759C8A7080p+0384 },   // 3.0e+0131
        { 0x1.2087D4358FC82p+0437,  0x0.7250F91306F5A8p+0385 },   // 4.0e+0131
        { 0x1.68A9C942F3BA3p+0437,  0x0.0EE53757C8B318p+0385 },   // 5.0e+0131
        { 0x1.B0CBBE5057AC3p+0437,  0x0.AB79759C8A7080p+0385 },   // 6.0e+0131
        { 0x1.F8EDB35DBB9E4p+0437,  0x0.480DB3E14C2DE8p+0385 },   // 7.0e+0131
        { 0x1.2087D4358FC82p+0438,  0x0.7250F91306F5A8p+0386 },   // 8.0e+0131
        { 0x1.4498CEBC41C12p+0438,  0x0.C09B183567D460p+0386 },   // 9.0e+0131
        { 0x1.68A9C942F3BA3p+0438,  0x0.0EE53757C8B318p+0386 },   // 1.0e+0132
        { 0x1.68A9C942F3BA3p+0439,  0x0.0EE53757C8B318p+0387 },   // 2.0e+0132
        { 0x1.0E7F56F236CBAp+0440,  0x0.4B2BE981D68650p+0388 },   // 3.0e+0132
        { 0x1.68A9C942F3BA3p+0440,  0x0.0EE53757C8B318p+0388 },   // 4.0e+0132
        { 0x1.C2D43B93B0A8Bp+0440,  0x0.D29E852DBADFD8p+0388 },   // 5.0e+0132
        { 0x1.0E7F56F236CBAp+0441,  0x0.4B2BE981D68650p+0389 },   // 6.0e+0132
        { 0x1.3B94901A9542Ep+0441,  0x0.AD08906CCF9CB0p+0389 },   // 7.0e+0132
        { 0x1.68A9C942F3BA3p+0441,  0x0.0EE53757C8B318p+0389 },   // 8.0e+0132
        { 0x1.95BF026B52317p+0441,  0x0.70C1DE42C1C978p+0389 },   // 9.0e+0132
        { 0x1.C2D43B93B0A8Bp+0441,  0x0.D29E852DBADFD8p+0389 },   // 1.0e+0133
        { 0x1.C2D43B93B0A8Bp+0442,  0x0.D29E852DBADFD8p+0390 },   // 2.0e+0133
        { 0x1.521F2CAEC47E8p+0443,  0x0.DDF6E3E24C27E0p+0391 },   // 3.0e+0133
        { 0x1.C2D43B93B0A8Bp+0443,  0x0.D29E852DBADFD8p+0391 },   // 4.0e+0133
        { 0x1.19C4A53C4E697p+0444,  0x0.63A3133C94CBE8p+0392 },   // 5.0e+0133
        { 0x1.521F2CAEC47E8p+0444,  0x0.DDF6E3E24C27E0p+0392 },   // 6.0e+0133
        { 0x1.8A79B4213A93Ap+0444,  0x0.584AB4880383E0p+0392 },   // 7.0e+0133
        { 0x1.C2D43B93B0A8Bp+0444,  0x0.D29E852DBADFD8p+0392 },   // 8.0e+0133
        { 0x1.FB2EC30626BDDp+0444,  0x0.4CF255D3723BD8p+0392 },   // 9.0e+0133
        { 0x1.19C4A53C4E697p+0445,  0x0.63A3133C94CBE8p+0393 },   // 1.0e+0134
        { 0x1.19C4A53C4E697p+0446,  0x0.63A3133C94CBE8p+0394 },   // 2.0e+0134
        { 0x1.A6A6F7DA759E3p+0446,  0x0.15749CDADF31E0p+0394 },   // 3.0e+0134
        { 0x1.19C4A53C4E697p+0447,  0x0.63A3133C94CBE8p+0395 },   // 4.0e+0134
        { 0x1.6035CE8B6203Dp+0447,  0x0.3C8BD80BB9FEE0p+0395 },   // 5.0e+0134
        { 0x1.A6A6F7DA759E3p+0447,  0x0.15749CDADF31E0p+0395 },   // 6.0e+0134
        { 0x1.ED18212989388p+0447,  0x0.EE5D61AA0464D8p+0395 },   // 7.0e+0134
        { 0x1.19C4A53C4E697p+0448,  0x0.63A3133C94CBE8p+0396 },   // 8.0e+0134
        { 0x1.3CFD39E3D836Ap+0448,  0x0.501775A4276568p+0396 },   // 9.0e+0134
        { 0x1.6035CE8B6203Dp+0448,  0x0.3C8BD80BB9FEE0p+0396 },   // 1.0e+0135
        { 0x1.6035CE8B6203Dp+0449,  0x0.3C8BD80BB9FEE0p+0397 },   // 2.0e+0135
        { 0x1.08285AE88982Dp+0450,  0x0.ED68E208CB7F28p+0398 },   // 3.0e+0135
        { 0x1.6035CE8B6203Dp+0450,  0x0.3C8BD80BB9FEE0p+0398 },   // 4.0e+0135
        { 0x1.B843422E3A84Cp+0450,  0x0.8BAECE0EA87E98p+0398 },   // 5.0e+0135
        { 0x1.08285AE88982Dp+0451,  0x0.ED68E208CB7F28p+0399 },   // 6.0e+0135
        { 0x1.342F14B9F5C35p+0451,  0x0.94FA5D0A42BF08p+0399 },   // 7.0e+0135
        { 0x1.6035CE8B6203Dp+0451,  0x0.3C8BD80BB9FEE0p+0399 },   // 8.0e+0135
        { 0x1.8C3C885CCE444p+0451,  0x0.E41D530D313EC0p+0399 },   // 9.0e+0135
        { 0x1.B843422E3A84Cp+0451,  0x0.8BAECE0EA87E98p+0399 },   // 1.0e+0136
        { 0x1.B843422E3A84Cp+0452,  0x0.8BAECE0EA87E98p+0400 },   // 2.0e+0136
        { 0x1.4A3271A2ABE39p+0453,  0x0.68C31A8AFE5EF0p+0401 },   // 3.0e+0136
        { 0x1.B843422E3A84Cp+0453,  0x0.8BAECE0EA87E98p+0401 },   // 4.0e+0136
        { 0x1.132A095CE492Fp+0454,  0x0.D74D40C9294F20p+0402 },   // 5.0e+0136
        { 0x1.4A3271A2ABE39p+0454,  0x0.68C31A8AFE5EF0p+0402 },   // 6.0e+0136
        { 0x1.813AD9E873342p+0454,  0x0.FA38F44CD36EC8p+0402 },   // 7.0e+0136
        { 0x1.B843422E3A84Cp+0454,  0x0.8BAECE0EA87E98p+0402 },   // 8.0e+0136
        { 0x1.EF4BAA7401D56p+0454,  0x0.1D24A7D07D8E70p+0402 },   // 9.0e+0136
        { 0x1.132A095CE492Fp+0455,  0x0.D74D40C9294F20p+0403 },   // 1.0e+0137
        { 0x1.132A095CE492Fp+0456,  0x0.D74D40C9294F20p+0404 },   // 2.0e+0137
        { 0x1.9CBF0E0B56DC7p+0456,  0x0.C2F3E12DBDF6B0p+0404 },   // 3.0e+0137
        { 0x1.132A095CE492Fp+0457,  0x0.D74D40C9294F20p+0405 },   // 4.0e+0137
        { 0x1.57F48BB41DB7Bp+0457,  0x0.CD2090FB73A2E8p+0405 },   // 5.0e+0137
        { 0x1.9CBF0E0B56DC7p+0457,  0x0.C2F3E12DBDF6B0p+0405 },   // 6.0e+0137
        { 0x1.E189906290013p+0457,  0x0.B8C73160084A78p+0405 },   // 7.0e+0137
        { 0x1.132A095CE492Fp+0458,  0x0.D74D40C9294F20p+0406 },   // 8.0e+0137
        { 0x1.358F4A8881255p+0458,  0x0.D236E8E24E7908p+0406 },   // 9.0e+0137
        { 0x1.57F48BB41DB7Bp+0458,  0x0.CD2090FB73A2E8p+0406 },   // 1.0e+0138
        { 0x1.57F48BB41DB7Bp+0459,  0x0.CD2090FB73A2E8p+0407 },   // 2.0e+0138
        { 0x1.01F768C71649Cp+0460,  0x0.D9D86CBC96BA30p+0408 },   // 3.0e+0138
        { 0x1.57F48BB41DB7Bp+0460,  0x0.CD2090FB73A2E8p+0408 },   // 4.0e+0138
        { 0x1.ADF1AEA12525Ap+0460,  0x0.C068B53A508BA0p+0408 },   // 5.0e+0138
        { 0x1.01F768C71649Cp+0461,  0x0.D9D86CBC96BA30p+0409 },   // 6.0e+0138
        { 0x1.2CF5FA3D9A00Cp+0461,  0x0.537C7EDC052E88p+0409 },   // 7.0e+0138
        { 0x1.57F48BB41DB7Bp+0461,  0x0.CD2090FB73A2E8p+0409 },   // 8.0e+0138
        { 0x1.82F31D2AA16EBp+0461,  0x0.46C4A31AE21748p+0409 },   // 9.0e+0138
        { 0x1.ADF1AEA12525Ap+0461,  0x0.C068B53A508BA0p+0409 },   // 1.0e+0139
        { 0x1.ADF1AEA12525Ap+0462,  0x0.C068B53A508BA0p+0410 },   // 2.0e+0139
        { 0x1.427542F8DBDC4p+0463,  0x0.104E87EBBC68B8p+0411 },   // 3.0e+0139
        { 0x1.ADF1AEA12525Ap+0463,  0x0.C068B53A508BA0p+0411 },   // 4.0e+0139
        { 0x1.0CB70D24B7378p+0464,  0x0.B8417144725748p+0412 },   // 5.0e+0139
        { 0x1.427542F8DBDC4p+0464,  0x0.104E87EBBC68B8p+0412 },   // 6.0e+0139
        { 0x1.783378CD0080Fp+0464,  0x0.685B9E93067A30p+0412 },   // 7.0e+0139
        { 0x1.ADF1AEA12525Ap+0464,  0x0.C068B53A508BA0p+0412 },   // 8.0e+0139
        { 0x1.E3AFE47549CA6p+0464,  0x0.1875CBE19A9D18p+0412 },   // 9.0e+0139
        { 0x1.0CB70D24B7378p+0465,  0x0.B8417144725748p+0413 },   // 1.0e+0140
        { 0x1.0CB70D24B7378p+0466,  0x0.B8417144725748p+0414 },   // 2.0e+0140
        { 0x1.931293B712D35p+0466,  0x0.146229E6AB82E8p+0414 },   // 3.0e+0140
        { 0x1.0CB70D24B7378p+0467,  0x0.B8417144725748p+0415 },   // 4.0e+0140
        { 0x1.4FE4D06DE5056p+0467,  0x0.E651CD958EED18p+0415 },   // 5.0e+0140
        { 0x1.931293B712D35p+0467,  0x0.146229E6AB82E8p+0415 },   // 6.0e+0140
        { 0x1.D640570040A13p+0467,  0x0.42728637C818B8p+0415 },   // 7.0e+0140
        { 0x1.0CB70D24B7378p+0468,  0x0.B8417144725748p+0416 },   // 8.0e+0140
        { 0x1.2E4DEEC94E1E7p+0468,  0x0.CF499F6D00A230p+0416 },   // 9.0e+0140
        { 0x1.4FE4D06DE5056p+0468,  0x0.E651CD958EED18p+0416 },   // 1.0e+0141
        { 0x1.4FE4D06DE5056p+0469,  0x0.E651CD958EED18p+0417 },   // 2.0e+0141
        { 0x1.F7D738A4D7882p+0469,  0x0.597AB4605663A8p+0417 },   // 3.0e+0141
        { 0x1.4FE4D06DE5056p+0470,  0x0.E651CD958EED18p+0418 },   // 4.0e+0141
        { 0x1.A3DE04895E46Cp+0470,  0x0.9FE640FAF2A860p+0418 },   // 5.0e+0141
        { 0x1.F7D738A4D7882p+0470,  0x0.597AB4605663A8p+0418 },   // 6.0e+0141
        { 0x1.25E836602864Cp+0471,  0x0.098793E2DD0F70p+0419 },   // 7.0e+0141
        { 0x1.4FE4D06DE5056p+0471,  0x0.E651CD958EED18p+0419 },   // 8.0e+0141
        { 0x1.79E16A7BA1A61p+0471,  0x0.C31C074840CAB8p+0419 },   // 9.0e+0141
        { 0x1.A3DE04895E46Cp+0471,  0x0.9FE640FAF2A860p+0419 },   // 1.0e+0142
        { 0x1.A3DE04895E46Cp+0472,  0x0.9FE640FAF2A860p+0420 },   // 2.0e+0142
        { 0x1.3AE6836706B51p+0473,  0x0.77ECB0BC35FE48p+0421 },   // 3.0e+0142
        { 0x1.A3DE04895E46Cp+0473,  0x0.9FE640FAF2A860p+0421 },   // 4.0e+0142
        { 0x1.066AC2D5DAEC3p+0474,  0x0.E3EFE89CD7A938p+0422 },   // 5.0e+0142
        { 0x1.3AE6836706B51p+0474,  0x0.77ECB0BC35FE48p+0422 },   // 6.0e+0142
        { 0x1.6F6243F8327DFp+0474,  0x0.0BE978DB945350p+0422 },   // 7.0e+0142
        { 0x1.A3DE04895E46Cp+0474,  0x0.9FE640FAF2A860p+0422 },   // 8.0e+0142
        { 0x1.D859C51A8A0FAp+0474,  0x0.33E3091A50FD68p+0422 },   // 9.0e+0142
        { 0x1.066AC2D5DAEC3p+0475,  0x0.E3EFE89CD7A938p+0423 },   // 1.0e+0143
        { 0x1.066AC2D5DAEC3p+0476,  0x0.E3EFE89CD7A938p+0424 },   // 2.0e+0143
        { 0x1.89A02440C8625p+0476,  0x0.D5E7DCEB437DD8p+0424 },   // 3.0e+0143
        { 0x1.066AC2D5DAEC3p+0477,  0x0.E3EFE89CD7A938p+0425 },   // 4.0e+0143
        { 0x1.4805738B51A74p+0477,  0x0.DCEBE2C40D9388p+0425 },   // 5.0e+0143
        { 0x1.89A02440C8625p+0477,  0x0.D5E7DCEB437DD8p+0425 },   // 6.0e+0143
        { 0x1.CB3AD4F63F1D6p+0477,  0x0.CEE3D712796828p+0425 },   // 7.0e+0143
        { 0x1.066AC2D5DAEC3p+0478,  0x0.E3EFE89CD7A938p+0426 },   // 8.0e+0143
        { 0x1.27381B309649Cp+0478,  0x0.606DE5B0729E60p+0426 },   // 9.0e+0143
        { 0x1.4805738B51A74p+0478,  0x0.DCEBE2C40D9388p+0426 },   // 1.0e+0144
        { 0x1.4805738B51A74p+0479,  0x0.DCEBE2C40D9388p+0427 },   // 2.0e+0144
        { 0x1.EC082D50FA7AFp+0479,  0x0.4B61D426145D50p+0427 },   // 3.0e+0144
        { 0x1.4805738B51A74p+0480,  0x0.DCEBE2C40D9388p+0428 },   // 4.0e+0144
        { 0x1.9A06D06E26112p+0480,  0x0.1426DB7510F868p+0428 },   // 5.0e+0144
        { 0x1.EC082D50FA7AFp+0480,  0x0.4B61D426145D50p+0428 },   // 6.0e+0144
        { 0x1.1F04C519E7726p+0481,  0x0.414E666B8BE118p+0429 },   // 7.0e+0144
        { 0x1.4805738B51A74p+0481,  0x0.DCEBE2C40D9388p+0429 },   // 8.0e+0144
        { 0x1.710621FCBBDC3p+0481,  0x0.78895F1C8F45F8p+0429 },   // 9.0e+0144
        { 0x1.9A06D06E26112p+0481,  0x0.1426DB7510F868p+0429 },   // 1.0e+0145
        { 0x1.9A06D06E26112p+0482,  0x0.1426DB7510F868p+0430 },   // 2.0e+0145
        { 0x1.33851C529C8CDp+0483,  0x0.8F1D2497CCBA50p+0431 },   // 3.0e+0145
        { 0x1.9A06D06E26112p+0483,  0x0.1426DB7510F868p+0431 },   // 4.0e+0145
        { 0x1.00444244D7CABp+0484,  0x0.4C9849292A9B40p+0432 },   // 5.0e+0145
        { 0x1.33851C529C8CDp+0484,  0x0.8F1D2497CCBA50p+0432 },   // 6.0e+0145
        { 0x1.66C5F660614EFp+0484,  0x0.D1A200066ED960p+0432 },   // 7.0e+0145
        { 0x1.9A06D06E26112p+0484,  0x0.1426DB7510F868p+0432 },   // 8.0e+0145
        { 0x1.CD47AA7BEAD34p+0484,  0x0.56ABB6E3B31778p+0432 },   // 9.0e+0145
        { 0x1.00444244D7CABp+0485,  0x0.4C9849292A9B40p+0433 },   // 1.0e+0146
        { 0x1.00444244D7CABp+0486,  0x0.4C9849292A9B40p+0434 },   // 2.0e+0146
        { 0x1.8066636743B00p+0486,  0x0.F2E46DBDBFE8E8p+0434 },   // 3.0e+0146
        { 0x1.00444244D7CABp+0487,  0x0.4C9849292A9B40p+0435 },   // 4.0e+0146
        { 0x1.405552D60DBD6p+0487,  0x0.1FBE5B73754210p+0435 },   // 5.0e+0146
        { 0x1.8066636743B00p+0487,  0x0.F2E46DBDBFE8E8p+0435 },   // 6.0e+0146
        { 0x1.C07773F879A2Bp+0487,  0x0.C60A80080A8FB8p+0435 },   // 7.0e+0146
        { 0x1.00444244D7CABp+0488,  0x0.4C9849292A9B40p+0436 },   // 8.0e+0146
        { 0x1.204CCA8D72C40p+0488,  0x0.B62B524E4FEEA8p+0436 },   // 9.0e+0146
        { 0x1.405552D60DBD6p+0488,  0x0.1FBE5B73754210p+0436 },   // 1.0e+0147
        { 0x1.405552D60DBD6p+0489,  0x0.1FBE5B73754210p+0437 },   // 2.0e+0147
        { 0x1.E07FFC41149C1p+0489,  0x0.2F9D892D2FE320p+0437 },   // 3.0e+0147
        { 0x1.405552D60DBD6p+0490,  0x0.1FBE5B73754210p+0438 },   // 4.0e+0147
        { 0x1.906AA78B912CBp+0490,  0x0.A7ADF250529298p+0438 },   // 5.0e+0147
        { 0x1.E07FFC41149C1p+0490,  0x0.2F9D892D2FE320p+0438 },   // 6.0e+0147
        { 0x1.184AA87B4C05Bp+0491,  0x0.5BC690050699D0p+0439 },   // 7.0e+0147
        { 0x1.405552D60DBD6p+0491,  0x0.1FBE5B73754210p+0439 },   // 8.0e+0147
        { 0x1.685FFD30CF750p+0491,  0x0.E3B626E1E3EA58p+0439 },   // 9.0e+0147
        { 0x1.906AA78B912CBp+0491,  0x0.A7ADF250529298p+0439 },   // 1.0e+0148
        { 0x1.906AA78B912CBp+0492,  0x0.A7ADF250529298p+0440 },   // 2.0e+0148
        { 0x1.2C4FFDA8ACE18p+0493,  0x0.BDC275BC3DEDF0p+0441 },   // 3.0e+0148
        { 0x1.906AA78B912CBp+0493,  0x0.A7ADF250529298p+0441 },   // 4.0e+0148
        { 0x1.F485516E7577Ep+0493,  0x0.91996EE4673740p+0441 },   // 5.0e+0148
        { 0x1.2C4FFDA8ACE18p+0494,  0x0.BDC275BC3DEDF0p+0442 },   // 6.0e+0148
        { 0x1.5E5D529A1F072p+0494,  0x0.32B83406484048p+0442 },   // 7.0e+0148
        { 0x1.906AA78B912CBp+0494,  0x0.A7ADF250529298p+0442 },   // 8.0e+0148
        { 0x1.C277FC7D03525p+0494,  0x0.1CA3B09A5CE4F0p+0442 },   // 9.0e+0148
        { 0x1.F485516E7577Ep+0494,  0x0.91996EE4673740p+0442 },   // 1.0e+0149
        { 0x1.F485516E7577Ep+0495,  0x0.91996EE4673740p+0443 },   // 2.0e+0149
        { 0x1.7763FD12D819Ep+0496,  0x0.ED33132B4D6970p+0444 },   // 3.0e+0149
        { 0x1.F485516E7577Ep+0496,  0x0.91996EE4673740p+0444 },   // 4.0e+0149
        { 0x1.38D352E5096AFp+0497,  0x0.1AFFE54EC08288p+0445 },   // 5.0e+0149
        { 0x1.7763FD12D819Ep+0497,  0x0.ED33132B4D6970p+0445 },   // 6.0e+0149
        { 0x1.B5F4A740A6C8Ep+0497,  0x0.BF664107DA5058p+0445 },   // 7.0e+0149
        { 0x1.F485516E7577Ep+0497,  0x0.91996EE4673740p+0445 },   // 8.0e+0149
        { 0x1.198AFDCE22137p+0498,  0x0.31E64E607A0F10p+0446 },   // 9.0e+0149
        { 0x1.38D352E5096AFp+0498,  0x0.1AFFE54EC08288p+0446 },   // 1.0e+0150
        { 0x1.38D352E5096AFp+0499,  0x0.1AFFE54EC08288p+0447 },   // 2.0e+0150
        { 0x1.D53CFC578E206p+0499,  0x0.A87FD7F620C3C8p+0447 },   // 3.0e+0150
        { 0x1.38D352E5096AFp+0500,  0x0.1AFFE54EC08288p+0448 },   // 4.0e+0150
        { 0x1.8708279E4BC5Ap+0500,  0x0.E1BFDEA270A328p+0448 },   // 5.0e+0150
        { 0x1.D53CFC578E206p+0500,  0x0.A87FD7F620C3C8p+0448 },   // 6.0e+0150
        { 0x1.11B8E888683D9p+0501,  0x0.379FE8A4E87238p+0449 },   // 7.0e+0150
        { 0x1.38D352E5096AFp+0501,  0x0.1AFFE54EC08288p+0449 },   // 8.0e+0150
        { 0x1.5FEDBD41AA984p+0501,  0x0.FE5FE1F89892D8p+0449 },   // 9.0e+0150
        { 0x1.8708279E4BC5Ap+0501,  0x0.E1BFDEA270A328p+0449 },   // 1.0e+0151
        { 0x1.8708279E4BC5Ap+0502,  0x0.E1BFDEA270A328p+0450 },   // 2.0e+0151
        { 0x1.25461DB6B8D44p+0503,  0x0.294FE6F9D47A60p+0451 },   // 3.0e+0151
        { 0x1.8708279E4BC5Ap+0503,  0x0.E1BFDEA270A328p+0451 },   // 4.0e+0151
        { 0x1.E8CA3185DEB71p+0503,  0x0.9A2FD64B0CCBF8p+0451 },   // 5.0e+0151
        { 0x1.25461DB6B8D44p+0504,  0x0.294FE6F9D47A60p+0452 },   // 6.0e+0151
        { 0x1.562722AA824CFp+0504,  0x0.8587E2CE228EC0p+0452 },   // 7.0e+0151
        { 0x1.8708279E4BC5Ap+0504,  0x0.E1BFDEA270A328p+0452 },   // 8.0e+0151
        { 0x1.B7E92C92153E6p+0504,  0x0.3DF7DA76BEB790p+0452 },   // 9.0e+0151
        { 0x1.E8CA3185DEB71p+0504,  0x0.9A2FD64B0CCBF8p+0452 },   // 1.0e+0152
        { 0x1.E8CA3185DEB71p+0505,  0x0.9A2FD64B0CCBF8p+0453 },   // 2.0e+0152
        { 0x1.6E97A52467095p+0506,  0x0.33A3E0B84998F8p+0454 },   // 3.0e+0152
        { 0x1.E8CA3185DEB71p+0506,  0x0.9A2FD64B0CCBF8p+0454 },   // 4.0e+0152
        { 0x1.317E5EF3AB327p+0507,  0x0.005DE5EEE7FF78p+0455 },   // 5.0e+0152
        { 0x1.6E97A52467095p+0507,  0x0.33A3E0B84998F8p+0455 },   // 6.0e+0152
        { 0x1.ABB0EB5522E03p+0507,  0x0.66E9DB81AB3278p+0455 },   // 7.0e+0152
        { 0x1.E8CA3185DEB71p+0507,  0x0.9A2FD64B0CCBF8p+0455 },   // 8.0e+0152
        { 0x1.12F1BBDB4D46Fp+0508,  0x0.E6BAE88A3732B8p+0456 },   // 9.0e+0152
        { 0x1.317E5EF3AB327p+0508,  0x0.005DE5EEE7FF78p+0456 },   // 1.0e+0153
        { 0x1.317E5EF3AB327p+0509,  0x0.005DE5EEE7FF78p+0457 },   // 2.0e+0153
        { 0x1.CA3D8E6D80CBAp+0509,  0x0.808CD8E65BFF38p+0457 },   // 3.0e+0153
        { 0x1.317E5EF3AB327p+0510,  0x0.005DE5EEE7FF78p+0458 },   // 4.0e+0153
        { 0x1.7DDDF6B095FF0p+0510,  0x0.C0755F6AA1FF58p+0458 },   // 5.0e+0153
        { 0x1.CA3D8E6D80CBAp+0510,  0x0.808CD8E65BFF38p+0458 },   // 6.0e+0153
        { 0x1.0B4E931535CC2p+0511,  0x0.205229310AFF88p+0459 },   // 7.0e+0153
        { 0x1.317E5EF3AB327p+0511,  0x0.005DE5EEE7FF78p+0459 },   // 8.0e+0153
        { 0x1.57AE2AD22098Bp+0511,  0x0.E069A2ACC4FF68p+0459 },   // 9.0e+0153
        { 0x1.7DDDF6B095FF0p+0511,  0x0.C0755F6AA1FF58p+0459 },   // 1.0e+0154
        { 0x1.7DDDF6B095FF0p+0512,  0x0.C0755F6AA1FF58p+0460 },   // 2.0e+0154
        { 0x1.1E667904707F4p+0513,  0x0.9058078FF97F80p+0461 },   // 3.0e+0154
        { 0x1.7DDDF6B095FF0p+0513,  0x0.C0755F6AA1FF58p+0461 },   // 4.0e+0154
        { 0x1.DD55745CBB7ECp+0513,  0x0.F092B7454A7F30p+0461 },   // 5.0e+0154
        { 0x1.1E667904707F4p+0514,  0x0.9058078FF97F80p+0462 },   // 6.0e+0154
        { 0x1.4E2237DA833F2p+0514,  0x0.A866B37D4DBF68p+0462 },   // 7.0e+0154
        { 0x1.7DDDF6B095FF0p+0514,  0x0.C0755F6AA1FF58p+0462 },   // 8.0e+0154
        { 0x1.AD99B586A8BEEp+0514,  0x0.D8840B57F63F40p+0462 },   // 9.0e+0154
        { 0x1.DD55745CBB7ECp+0514,  0x0.F092B7454A7F30p+0462 },   // 1.0e+0155
        { 0x1.DD55745CBB7ECp+0515,  0x0.F092B7454A7F30p+0463 },   // 2.0e+0155
        { 0x1.660017458C9F1p+0516,  0x0.B46E0973F7DF60p+0464 },   // 3.0e+0155
        { 0x1.DD55745CBB7ECp+0516,  0x0.F092B7454A7F30p+0464 },   // 4.0e+0155
        { 0x1.2A5568B9F52F4p+0517,  0x0.165BB28B4E8F78p+0465 },   // 5.0e+0155
        { 0x1.660017458C9F1p+0517,  0x0.B46E0973F7DF60p+0465 },   // 6.0e+0155
        { 0x1.A1AAC5D1240EFp+0517,  0x0.5280605CA12F48p+0465 },   // 7.0e+0155
        { 0x1.DD55745CBB7ECp+0517,  0x0.F092B7454A7F30p+0465 },   // 8.0e+0155
        { 0x1.0C80117429775p+0518,  0x0.47528716F9E788p+0466 },   // 9.0e+0155
        { 0x1.2A5568B9F52F4p+0518,  0x0.165BB28B4E8F78p+0466 },   // 1.0e+0156
        { 0x1.2A5568B9F52F4p+0519,  0x0.165BB28B4E8F78p+0467 },   // 2.0e+0156
        { 0x1.BF801D16EFC6Ep+0519,  0x0.21898BD0F5D738p+0467 },   // 3.0e+0156
        { 0x1.2A5568B9F52F4p+0520,  0x0.165BB28B4E8F78p+0468 },   // 4.0e+0156
        { 0x1.74EAC2E8727B1p+0520,  0x0.1BF29F2E223358p+0468 },   // 5.0e+0156
        { 0x1.BF801D16EFC6Ep+0520,  0x0.21898BD0F5D738p+0468 },   // 6.0e+0156
        { 0x1.050ABBA2B6895p+0521,  0x0.93903C39E4BD88p+0469 },   // 7.0e+0156
        { 0x1.2A5568B9F52F4p+0521,  0x0.165BB28B4E8F78p+0469 },   // 8.0e+0156
        { 0x1.4FA015D133D52p+0521,  0x0.992728DCB86168p+0469 },   // 9.0e+0156
        { 0x1.74EAC2E8727B1p+0521,  0x0.1BF29F2E223358p+0469 },   // 1.0e+0157
        { 0x1.74EAC2E8727B1p+0522,  0x0.1BF29F2E223358p+0470 },   // 2.0e+0157
        { 0x1.17B0122E55DC4p+0523,  0x0.D4F5F76299A680p+0471 },   // 3.0e+0157
        { 0x1.74EAC2E8727B1p+0523,  0x0.1BF29F2E223358p+0471 },   // 4.0e+0157
        { 0x1.D22573A28F19Dp+0523,  0x0.62EF46F9AAC030p+0471 },   // 5.0e+0157
        { 0x1.17B0122E55DC4p+0524,  0x0.D4F5F76299A680p+0472 },   // 6.0e+0157
        { 0x1.464D6A8B642BAp+0524,  0x0.F8744B485DECF0p+0472 },   // 7.0e+0157
        { 0x1.74EAC2E8727B1p+0524,  0x0.1BF29F2E223358p+0472 },   // 8.0e+0157
        { 0x1.A3881B4580CA7p+0524,  0x0.3F70F313E679C8p+0472 },   // 9.0e+0157
        { 0x1.D22573A28F19Dp+0524,  0x0.62EF46F9AAC030p+0472 },   // 1.0e+0158
        { 0x1.D22573A28F19Dp+0525,  0x0.62EF46F9AAC030p+0473 },   // 2.0e+0158
        { 0x1.5D9C16B9EB536p+0526,  0x0.0A33753B401028p+0474 },   // 3.0e+0158
        { 0x1.D22573A28F19Dp+0526,  0x0.62EF46F9AAC030p+0474 },   // 4.0e+0158
        { 0x1.2357684599702p+0527,  0x0.5DD58C5C0AB820p+0475 },   // 5.0e+0158
        { 0x1.5D9C16B9EB536p+0527,  0x0.0A33753B401028p+0475 },   // 6.0e+0158
        { 0x1.97E0C52E3D369p+0527,  0x0.B6915E1A756828p+0475 },   // 7.0e+0158
        { 0x1.D22573A28F19Dp+0527,  0x0.62EF46F9AAC030p+0475 },   // 8.0e+0158
        { 0x1.0635110B707E8p+0528,  0x0.87A697EC700C18p+0476 },   // 9.0e+0158
        { 0x1.2357684599702p+0528,  0x0.5DD58C5C0AB820p+0476 },   // 1.0e+0159
        { 0x1.2357684599702p+0529,  0x0.5DD58C5C0AB820p+0477 },   // 2.0e+0159
        { 0x1.B5031C6866283p+0529,  0x0.8CC0528A101430p+0477 },   // 3.0e+0159
        { 0x1.2357684599702p+0530,  0x0.5DD58C5C0AB820p+0478 },   // 4.0e+0159
        { 0x1.6C2D4256FFCC2p+0530,  0x0.F54AEF730D6628p+0478 },   // 5.0e+0159
        { 0x1.B5031C6866283p+0530,  0x0.8CC0528A101430p+0478 },   // 6.0e+0159
        { 0x1.FDD8F679CC844p+0530,  0x0.2435B5A112C238p+0478 },   // 7.0e+0159
        { 0x1.2357684599702p+0531,  0x0.5DD58C5C0AB820p+0479 },   // 8.0e+0159
        { 0x1.47C2554E4C9E2p+0531,  0x0.A9903DE78C0F20p+0479 },   // 9.0e+0159
        { 0x1.6C2D4256FFCC2p+0531,  0x0.F54AEF730D6628p+0479 },   // 1.0e+0160
        { 0x1.6C2D4256FFCC2p+0532,  0x0.F54AEF730D6628p+0480 },   // 2.0e+0160
        { 0x1.1121F1C13FD92p+0533,  0x0.37F833964A0C98p+0481 },   // 3.0e+0160
        { 0x1.6C2D4256FFCC2p+0533,  0x0.F54AEF730D6628p+0481 },   // 4.0e+0160
        { 0x1.C73892ECBFBF3p+0533,  0x0.B29DAB4FD0BFB0p+0481 },   // 5.0e+0160
        { 0x1.1121F1C13FD92p+0534,  0x0.37F833964A0C98p+0482 },   // 6.0e+0160
        { 0x1.3EA79A0C1FD2Ap+0534,  0x0.96A19184ABB960p+0482 },   // 7.0e+0160
        { 0x1.6C2D4256FFCC2p+0534,  0x0.F54AEF730D6628p+0482 },   // 8.0e+0160
        { 0x1.99B2EAA1DFC5Bp+0534,  0x0.53F44D616F12E8p+0482 },   // 9.0e+0160
        { 0x1.C73892ECBFBF3p+0534,  0x0.B29DAB4FD0BFB0p+0482 },   // 1.0e+0161
        { 0x1.C73892ECBFBF3p+0535,  0x0.B29DAB4FD0BFB0p+0483 },   // 2.0e+0161
        { 0x1.556A6E318FCF6p+0536,  0x0.C5F6407BDC8FC0p+0484 },   // 3.0e+0161
        { 0x1.C73892ECBFBF3p+0536,  0x0.B29DAB4FD0BFB0p+0484 },   // 4.0e+0161
        { 0x1.1C835BD3F7D78p+0537,  0x0.4FA28B11E277D0p+0485 },   // 5.0e+0161
        { 0x1.556A6E318FCF6p+0537,  0x0.C5F6407BDC8FC0p+0485 },   // 6.0e+0161
        { 0x1.8E51808F27C75p+0537,  0x0.3C49F5E5D6A7B8p+0485 },   // 7.0e+0161
        { 0x1.C73892ECBFBF3p+0537,  0x0.B29DAB4FD0BFB0p+0485 },   // 8.0e+0161
        { 0x1.000FD2A52BDB9p+0538,  0x0.1478B05CE56BD0p+0486 },   // 9.0e+0161
        { 0x1.1C835BD3F7D78p+0538,  0x0.4FA28B11E277D0p+0486 },   // 1.0e+0162
        { 0x1.1C835BD3F7D78p+0539,  0x0.4FA28B11E277D0p+0487 },   // 2.0e+0162
        { 0x1.AAC509BDF3C34p+0539,  0x0.7773D09AD3B3B8p+0487 },   // 3.0e+0162
        { 0x1.1C835BD3F7D78p+0540,  0x0.4FA28B11E277D0p+0488 },   // 4.0e+0162
        { 0x1.63A432C8F5CD6p+0540,  0x0.638B2DD65B15C0p+0488 },   // 5.0e+0162
        { 0x1.AAC509BDF3C34p+0540,  0x0.7773D09AD3B3B8p+0488 },   // 6.0e+0162
        { 0x1.F1E5E0B2F1B92p+0540,  0x0.8B5C735F4C51A8p+0488 },   // 7.0e+0162
        { 0x1.1C835BD3F7D78p+0541,  0x0.4FA28B11E277D0p+0489 },   // 8.0e+0162
        { 0x1.4013C74E76D27p+0541,  0x0.5996DC741EC6C8p+0489 },   // 9.0e+0162
        { 0x1.63A432C8F5CD6p+0541,  0x0.638B2DD65B15C0p+0489 },   // 1.0e+0163
        { 0x1.63A432C8F5CD6p+0542,  0x0.638B2DD65B15C0p+0490 },   // 2.0e+0163
        { 0x1.0ABB2616B85A0p+0543,  0x0.CAA86260C45050p+0491 },   // 3.0e+0163
        { 0x1.63A432C8F5CD6p+0543,  0x0.638B2DD65B15C0p+0491 },   // 4.0e+0163
        { 0x1.BC8D3F7B3340Bp+0543,  0x0.FC6DF94BF1DB30p+0491 },   // 5.0e+0163
        { 0x1.0ABB2616B85A0p+0544,  0x0.CAA86260C45050p+0492 },   // 6.0e+0163
        { 0x1.372FAC6FD713Bp+0544,  0x0.9719C81B8FB308p+0492 },   // 7.0e+0163
        { 0x1.63A432C8F5CD6p+0544,  0x0.638B2DD65B15C0p+0492 },   // 8.0e+0163
        { 0x1.9018B92214871p+0544,  0x0.2FFC9391267878p+0492 },   // 9.0e+0163
        { 0x1.BC8D3F7B3340Bp+0544,  0x0.FC6DF94BF1DB30p+0492 },   // 1.0e+0164
        { 0x1.BC8D3F7B3340Bp+0545,  0x0.FC6DF94BF1DB30p+0493 },   // 2.0e+0164
        { 0x1.4D69EF9C66708p+0546,  0x0.FD527AF8F56468p+0494 },   // 3.0e+0164
        { 0x1.BC8D3F7B3340Bp+0546,  0x0.FC6DF94BF1DB30p+0494 },   // 4.0e+0164
        { 0x1.15D847AD00087p+0547,  0x0.7DC4BBCF772900p+0495 },   // 5.0e+0164
        { 0x1.4D69EF9C66708p+0547,  0x0.FD527AF8F56468p+0495 },   // 6.0e+0164
        { 0x1.84FB978BCCD8Ap+0547,  0x0.7CE03A22739FC8p+0495 },   // 7.0e+0164
        { 0x1.BC8D3F7B3340Bp+0547,  0x0.FC6DF94BF1DB30p+0495 },   // 8.0e+0164
        { 0x1.F41EE76A99A8Dp+0547,  0x0.7BFBB875701698p+0495 },   // 9.0e+0164
        { 0x1.15D847AD00087p+0548,  0x0.7DC4BBCF772900p+0496 },   // 1.0e+0165
        { 0x1.15D847AD00087p+0549,  0x0.7DC4BBCF772900p+0497 },   // 2.0e+0165
        { 0x1.A0C46B83800CBp+0549,  0x0.3CA719B732BD80p+0497 },   // 3.0e+0165
        { 0x1.15D847AD00087p+0550,  0x0.7DC4BBCF772900p+0498 },   // 4.0e+0165
        { 0x1.5B4E5998400A9p+0550,  0x0.5D35EAC354F340p+0498 },   // 5.0e+0165
        { 0x1.A0C46B83800CBp+0550,  0x0.3CA719B732BD80p+0498 },   // 6.0e+0165
        { 0x1.E63A7D6EC00EDp+0550,  0x0.1C1848AB1087C0p+0498 },   // 7.0e+0165
        { 0x1.15D847AD00087p+0551,  0x0.7DC4BBCF772900p+0499 },   // 8.0e+0165
        { 0x1.389350A2A0098p+0551,  0x0.6D7D5349660E20p+0499 },   // 9.0e+0165
        { 0x1.5B4E5998400A9p+0551,  0x0.5D35EAC354F340p+0499 },   // 1.0e+0166
        { 0x1.5B4E5998400A9p+0552,  0x0.5D35EAC354F340p+0500 },   // 2.0e+0166
        { 0x1.047AC3323007Fp+0553,  0x0.05E870127FB670p+0501 },   // 3.0e+0166
        { 0x1.5B4E5998400A9p+0553,  0x0.5D35EAC354F340p+0501 },   // 4.0e+0166
        { 0x1.B221EFFE500D3p+0553,  0x0.B48365742A3010p+0501 },   // 5.0e+0166
        { 0x1.047AC3323007Fp+0554,  0x0.05E870127FB670p+0502 },   // 6.0e+0166
        { 0x1.2FE48E6538094p+0554,  0x0.318F2D6AEA54D8p+0502 },   // 7.0e+0166
        { 0x1.5B4E5998400A9p+0554,  0x0.5D35EAC354F340p+0502 },   // 8.0e+0166
        { 0x1.86B824CB480BEp+0554,  0x0.88DCA81BBF91A8p+0502 },   // 9.0e+0166
        { 0x1.B221EFFE500D3p+0554,  0x0.B48365742A3010p+0502 },   // 1.0e+0167
        { 0x1.B221EFFE500D3p+0555,  0x0.B48365742A3010p+0503 },   // 2.0e+0167
        { 0x1.459973FEBC09Ep+0556,  0x0.C7628C171FA408p+0504 },   // 3.0e+0167
        { 0x1.B221EFFE500D3p+0556,  0x0.B48365742A3010p+0504 },   // 4.0e+0167
        { 0x1.0F5535FEF2084p+0557,  0x0.50D21F689A5E08p+0505 },   // 5.0e+0167
        { 0x1.459973FEBC09Ep+0557,  0x0.C7628C171FA408p+0505 },   // 6.0e+0167
        { 0x1.7BDDB1FE860B9p+0557,  0x0.3DF2F8C5A4EA10p+0505 },   // 7.0e+0167
        { 0x1.B221EFFE500D3p+0557,  0x0.B48365742A3010p+0505 },   // 8.0e+0167
        { 0x1.E8662DFE1A0EEp+0557,  0x0.2B13D222AF7610p+0505 },   // 9.0e+0167
        { 0x1.0F5535FEF2084p+0558,  0x0.50D21F689A5E08p+0506 },   // 1.0e+0168
        { 0x1.0F5535FEF2084p+0559,  0x0.50D21F689A5E08p+0507 },   // 2.0e+0168
        { 0x1.96FFD0FE6B0C6p+0559,  0x0.793B2F1CE78D10p+0507 },   // 3.0e+0168
        { 0x1.0F5535FEF2084p+0560,  0x0.50D21F689A5E08p+0508 },   // 4.0e+0168
        { 0x1.532A837EAE8A5p+0560,  0x0.6506A742C0F588p+0508 },   // 5.0e+0168
        { 0x1.96FFD0FE6B0C6p+0560,  0x0.793B2F1CE78D10p+0508 },   // 6.0e+0168
        { 0x1.DAD51E7E278E7p+0560,  0x0.8D6FB6F70E2490p+0508 },   // 7.0e+0168
        { 0x1.0F5535FEF2084p+0561,  0x0.50D21F689A5E08p+0509 },   // 8.0e+0168
        { 0x1.313FDCBED0494p+0561,  0x0.DAEC6355ADA9C8p+0509 },   // 9.0e+0168
        { 0x1.532A837EAE8A5p+0561,  0x0.6506A742C0F588p+0509 },   // 1.0e+0169
        { 0x1.532A837EAE8A5p+0562,  0x0.6506A742C0F588p+0510 },   // 2.0e+0169
        { 0x1.FCBFC53E05CF8p+0562,  0x0.1789FAE4217050p+0510 },   // 3.0e+0169
        { 0x1.532A837EAE8A5p+0563,  0x0.6506A742C0F588p+0511 },   // 4.0e+0169
        { 0x1.A7F5245E5A2CEp+0563,  0x0.BE4851137132F0p+0511 },   // 5.0e+0169
        { 0x1.FCBFC53E05CF8p+0563,  0x0.1789FAE4217050p+0511 },   // 6.0e+0169
        { 0x1.28C5330ED8B90p+0564,  0x0.B865D25A68D6D8p+0512 },   // 7.0e+0169
        { 0x1.532A837EAE8A5p+0564,  0x0.6506A742C0F588p+0512 },   // 8.0e+0169
        { 0x1.7D8FD3EE845BAp+0564,  0x0.11A77C2B191440p+0512 },   // 9.0e+0169
        { 0x1.A7F5245E5A2CEp+0564,  0x0.BE4851137132F0p+0512 },   // 1.0e+0170
        { 0x1.A7F5245E5A2CEp+0565,  0x0.BE4851137132F0p+0513 },   // 2.0e+0170
        { 0x1.3DF7DB46C3A1Bp+0566,  0x0.0EB63CCE94E630p+0514 },   // 3.0e+0170
        { 0x1.A7F5245E5A2CEp+0566,  0x0.BE4851137132F0p+0514 },   // 4.0e+0170
        { 0x1.08F936BAF85C1p+0567,  0x0.36ED32AC26BFD0p+0515 },   // 5.0e+0170
        { 0x1.3DF7DB46C3A1Bp+0567,  0x0.0EB63CCE94E630p+0515 },   // 6.0e+0170
        { 0x1.72F67FD28EE74p+0567,  0x0.E67F46F1030C90p+0515 },   // 7.0e+0170
        { 0x1.A7F5245E5A2CEp+0567,  0x0.BE4851137132F0p+0515 },   // 8.0e+0170
        { 0x1.DCF3C8EA25728p+0567,  0x0.96115B35DF5950p+0515 },   // 9.0e+0170
        { 0x1.08F936BAF85C1p+0568,  0x0.36ED32AC26BFD0p+0516 },   // 1.0e+0171
        { 0x1.08F936BAF85C1p+0569,  0x0.36ED32AC26BFD0p+0517 },   // 2.0e+0171
        { 0x1.8D75D218748A1p+0569,  0x0.D263CC023A1FC0p+0517 },   // 3.0e+0171
        { 0x1.08F936BAF85C1p+0570,  0x0.36ED32AC26BFD0p+0518 },   // 4.0e+0171
        { 0x1.4B378469B6731p+0570,  0x0.84A87F57306FC8p+0518 },   // 5.0e+0171
        { 0x1.8D75D218748A1p+0570,  0x0.D263CC023A1FC0p+0518 },   // 6.0e+0171
        { 0x1.CFB41FC732A12p+0570,  0x0.201F18AD43CFB8p+0518 },   // 7.0e+0171
        { 0x1.08F936BAF85C1p+0571,  0x0.36ED32AC26BFD0p+0519 },   // 8.0e+0171
        { 0x1.2A185D9257679p+0571,  0x0.5DCAD901AB97D0p+0519 },   // 9.0e+0171
        { 0x1.4B378469B6731p+0571,  0x0.84A87F57306FC8p+0519 },   // 1.0e+0172
        { 0x1.4B378469B6731p+0572,  0x0.84A87F57306FC8p+0520 },   // 2.0e+0172
        { 0x1.F0D3469E91ACAp+0572,  0x0.46FCBF02C8A7B0p+0520 },   // 3.0e+0172
        { 0x1.4B378469B6731p+0573,  0x0.84A87F57306FC8p+0521 },   // 4.0e+0172
        { 0x1.9E056584240FDp+0573,  0x0.E5D29F2CFC8BC0p+0521 },   // 5.0e+0172
        { 0x1.F0D3469E91ACAp+0573,  0x0.46FCBF02C8A7B0p+0521 },   // 6.0e+0172
        { 0x1.21D093DC7FA4Bp+0574,  0x0.54136F6C4A61D0p+0522 },   // 7.0e+0172
        { 0x1.4B378469B6731p+0574,  0x0.84A87F57306FC8p+0522 },   // 8.0e+0172
        { 0x1.749E74F6ED417p+0574,  0x0.B53D8F42167DC0p+0522 },   // 9.0e+0172
        { 0x1.9E056584240FDp+0574,  0x0.E5D29F2CFC8BC0p+0522 },   // 1.0e+0173
        { 0x1.9E056584240FDp+0575,  0x0.E5D29F2CFC8BC0p+0523 },   // 2.0e+0173
        { 0x1.36840C231B0BEp+0576,  0x0.6C5DF761BD68D0p+0524 },   // 3.0e+0173
        { 0x1.9E056584240FDp+0576,  0x0.E5D29F2CFC8BC0p+0524 },   // 4.0e+0173
        { 0x1.02C35F729689Ep+0577,  0x0.AFA3A37C1DD758p+0525 },   // 5.0e+0173
        { 0x1.36840C231B0BEp+0577,  0x0.6C5DF761BD68D0p+0525 },   // 6.0e+0173
        { 0x1.6A44B8D39F8DEp+0577,  0x0.29184B475CFA48p+0525 },   // 7.0e+0173
        { 0x1.9E056584240FDp+0577,  0x0.E5D29F2CFC8BC0p+0525 },   // 8.0e+0173
        { 0x1.D1C61234A891Dp+0577,  0x0.A28CF3129C1D38p+0525 },   // 9.0e+0173
        { 0x1.02C35F729689Ep+0578,  0x0.AFA3A37C1DD758p+0526 },   // 1.0e+0174
        { 0x1.02C35F729689Ep+0579,  0x0.AFA3A37C1DD758p+0527 },   // 2.0e+0174
        { 0x1.84250F2BE1CEEp+0579,  0x0.0775753A2CC300p+0527 },   // 3.0e+0174
        { 0x1.02C35F729689Ep+0580,  0x0.AFA3A37C1DD758p+0528 },   // 4.0e+0174
        { 0x1.4374374F3C2C6p+0580,  0x0.5B8C8C5B254D28p+0528 },   // 5.0e+0174
        { 0x1.84250F2BE1CEEp+0580,  0x0.0775753A2CC300p+0528 },   // 6.0e+0174
        { 0x1.C4D5E70887715p+0580,  0x0.B35E5E193438D8p+0528 },   // 7.0e+0174
        { 0x1.02C35F729689Ep+0581,  0x0.AFA3A37C1DD758p+0529 },   // 8.0e+0174
        { 0x1.231BCB60E95B2p+0581,  0x0.859817EBA19240p+0529 },   // 9.0e+0174
        { 0x1.4374374F3C2C6p+0581,  0x0.5B8C8C5B254D28p+0529 },   // 1.0e+0175
        { 0x1.4374374F3C2C6p+0582,  0x0.5B8C8C5B254D28p+0530 },   // 2.0e+0175
        { 0x1.E52E52F6DA429p+0582,  0x0.8952D288B7F3C0p+0530 },   // 3.0e+0175
        { 0x1.4374374F3C2C6p+0583,  0x0.5B8C8C5B254D28p+0531 },   // 4.0e+0175
        { 0x1.945145230B377p+0583,  0x0.F26FAF71EEA078p+0531 },   // 5.0e+0175
        { 0x1.E52E52F6DA429p+0583,  0x0.8952D288B7F3C0p+0531 },   // 6.0e+0175
        { 0x1.1B05B06554A6Dp+0584,  0x0.901AFACFC0A388p+0532 },   // 7.0e+0175
        { 0x1.4374374F3C2C6p+0584,  0x0.5B8C8C5B254D28p+0532 },   // 8.0e+0175
        { 0x1.6BE2BE3923B1Fp+0584,  0x0.26FE1DE689F6D0p+0532 },   // 9.0e+0175
        { 0x1.945145230B377p+0584,  0x0.F26FAF71EEA078p+0532 },   // 1.0e+0176
        { 0x1.945145230B377p+0585,  0x0.F26FAF71EEA078p+0533 },   // 2.0e+0176
        { 0x1.2F3CF3DA48699p+0586,  0x0.F5D3C39572F858p+0534 },   // 3.0e+0176
        { 0x1.945145230B377p+0586,  0x0.F26FAF71EEA078p+0534 },   // 4.0e+0176
        { 0x1.F965966BCE055p+0586,  0x0.EF0B9B4E6A4898p+0534 },   // 5.0e+0176
        { 0x1.2F3CF3DA48699p+0587,  0x0.F5D3C39572F858p+0535 },   // 6.0e+0176
        { 0x1.61C71C7EA9D08p+0587,  0x0.F421B983B0CC68p+0535 },   // 7.0e+0176
        { 0x1.945145230B377p+0587,  0x0.F26FAF71EEA078p+0535 },   // 8.0e+0176
        { 0x1.C6DB6DC76C9E6p+0587,  0x0.F0BDA5602C7488p+0535 },   // 9.0e+0176
        { 0x1.F965966BCE055p+0587,  0x0.EF0B9B4E6A4898p+0535 },   // 1.0e+0177
        { 0x1.F965966BCE055p+0588,  0x0.EF0B9B4E6A4898p+0536 },   // 2.0e+0177
        { 0x1.7B0C30D0DA840p+0589,  0x0.7348B47ACFB670p+0537 },   // 3.0e+0177
        { 0x1.F965966BCE055p+0589,  0x0.EF0B9B4E6A4898p+0537 },   // 4.0e+0177
        { 0x1.3BDF7E0360C35p+0590,  0x0.B5674111026D58p+0538 },   // 5.0e+0177
        { 0x1.7B0C30D0DA840p+0590,  0x0.7348B47ACFB670p+0538 },   // 6.0e+0177
        { 0x1.BA38E39E5444Bp+0590,  0x0.312A27E49CFF80p+0538 },   // 7.0e+0177
        { 0x1.F965966BCE055p+0590,  0x0.EF0B9B4E6A4898p+0538 },   // 8.0e+0177
        { 0x1.1C49249CA3E30p+0591,  0x0.5676875C1BC8D0p+0539 },   // 9.0e+0177
        { 0x1.3BDF7E0360C35p+0591,  0x0.B5674111026D58p+0539 },   // 1.0e+0178
        { 0x1.3BDF7E0360C35p+0592,  0x0.B5674111026D58p+0540 },   // 2.0e+0178
        { 0x1.D9CF3D0511250p+0592,  0x0.901AE19983A408p+0540 },   // 3.0e+0178
        { 0x1.3BDF7E0360C35p+0593,  0x0.B5674111026D58p+0541 },   // 4.0e+0178
        { 0x1.8AD75D8438F43p+0593,  0x0.22C111554308B0p+0541 },   // 5.0e+0178
        { 0x1.D9CF3D0511250p+0593,  0x0.901AE19983A408p+0541 },   // 6.0e+0178
        { 0x1.14638E42F4AAEp+0594,  0x0.FEBA58EEE21FB0p+0542 },   // 7.0e+0178
        { 0x1.3BDF7E0360C35p+0594,  0x0.B5674111026D58p+0542 },   // 8.0e+0178
        { 0x1.635B6DC3CCDBCp+0594,  0x0.6C14293322BB08p+0542 },   // 9.0e+0178
        { 0x1.8AD75D8438F43p+0594,  0x0.22C111554308B0p+0542 },   // 1.0e+0179
        { 0x1.8AD75D8438F43p+0595,  0x0.22C111554308B0p+0543 },   // 2.0e+0179
        { 0x1.282186232AB72p+0596,  0x0.5A10CCFFF24688p+0544 },   // 3.0e+0179
        { 0x1.8AD75D8438F43p+0596,  0x0.22C111554308B0p+0544 },   // 4.0e+0179
        { 0x1.ED8D34E547313p+0596,  0x0.EB7155AA93CAE0p+0544 },   // 5.0e+0179
        { 0x1.282186232AB72p+0597,  0x0.5A10CCFFF24688p+0545 },   // 6.0e+0179
        { 0x1.597C71D3B1D5Ap+0597,  0x0.BE68EF2A9AA7A0p+0545 },   // 7.0e+0179
        { 0x1.8AD75D8438F43p+0597,  0x0.22C111554308B0p+0545 },   // 8.0e+0179
        { 0x1.BC324934C012Bp+0597,  0x0.8719337FEB69C8p+0545 },   // 9.0e+0179
        { 0x1.ED8D34E547313p+0597,  0x0.EB7155AA93CAE0p+0545 },   // 1.0e+0180
        { 0x1.ED8D34E547313p+0598,  0x0.EB7155AA93CAE0p+0546 },   // 2.0e+0180
        { 0x1.7229E7ABF564Ep+0599,  0x0.F095003FEED828p+0547 },   // 3.0e+0180
        { 0x1.ED8D34E547313p+0599,  0x0.EB7155AA93CAE0p+0547 },   // 4.0e+0180
        { 0x1.3478410F4C7ECp+0600,  0x0.7326D58A9C5EC8p+0548 },   // 5.0e+0180
        { 0x1.7229E7ABF564Ep+0600,  0x0.F095003FEED828p+0548 },   // 6.0e+0180
        { 0x1.AFDB8E489E4B1p+0600,  0x0.6E032AF5415188p+0548 },   // 7.0e+0180
        { 0x1.ED8D34E547313p+0600,  0x0.EB7155AA93CAE0p+0548 },   // 8.0e+0180
        { 0x1.159F6DC0F80BBp+0601,  0x0.346FC02FF32220p+0549 },   // 9.0e+0180
        { 0x1.3478410F4C7ECp+0601,  0x0.7326D58A9C5EC8p+0549 },   // 1.0e+0181
        { 0x1.3478410F4C7ECp+0602,  0x0.7326D58A9C5EC8p+0550 },   // 2.0e+0181
        { 0x1.CEB46196F2BE2p+0602,  0x0.ACBA404FEA8E30p+0550 },   // 3.0e+0181
        { 0x1.3478410F4C7ECp+0603,  0x0.7326D58A9C5EC8p+0551 },   // 4.0e+0181
        { 0x1.819651531F9E7p+0603,  0x0.8FF08AED437680p+0551 },   // 5.0e+0181
        { 0x1.CEB46196F2BE2p+0603,  0x0.ACBA404FEA8E30p+0551 },   // 6.0e+0181
        { 0x1.0DE938ED62EEEp+0604,  0x0.E4C1FAD948D2F0p+0552 },   // 7.0e+0181
        { 0x1.3478410F4C7ECp+0604,  0x0.7326D58A9C5EC8p+0552 },   // 8.0e+0181
        { 0x1.5B074931360EAp+0604,  0x0.018BB03BEFEAA8p+0552 },   // 9.0e+0181
        { 0x1.819651531F9E7p+0604,  0x0.8FF08AED437680p+0552 },   // 1.0e+0182
        { 0x1.819651531F9E7p+0605,  0x0.8FF08AED437680p+0553 },   // 2.0e+0182
        { 0x1.2130BCFE57B6Dp+0606,  0x0.ABF46831F298E0p+0554 },   // 3.0e+0182
        { 0x1.819651531F9E7p+0606,  0x0.8FF08AED437680p+0554 },   // 4.0e+0182
        { 0x1.E1FBE5A7E7861p+0606,  0x0.73ECADA8945420p+0554 },   // 5.0e+0182
        { 0x1.2130BCFE57B6Dp+0607,  0x0.ABF46831F298E0p+0555 },   // 6.0e+0182
        { 0x1.51638728BBAAAp+0607,  0x0.9DF2798F9B07B0p+0555 },   // 7.0e+0182
        { 0x1.819651531F9E7p+0607,  0x0.8FF08AED437680p+0555 },   // 8.0e+0182
        { 0x1.B1C91B7D83924p+0607,  0x0.81EE9C4AEBE550p+0555 },   // 9.0e+0182
        { 0x1.E1FBE5A7E7861p+0607,  0x0.73ECADA8945420p+0555 },   // 1.0e+0183
        { 0x1.E1FBE5A7E7861p+0608,  0x0.73ECADA8945420p+0556 },   // 2.0e+0183
        { 0x1.697CEC3DEDA49p+0609,  0x0.16F1823E6F3F18p+0557 },   // 3.0e+0183
        { 0x1.E1FBE5A7E7861p+0609,  0x0.73ECADA8945420p+0557 },   // 4.0e+0183
        { 0x1.2D3D6F88F0B3Cp+0610,  0x0.E873EC895CB490p+0558 },   // 5.0e+0183
        { 0x1.697CEC3DEDA49p+0610,  0x0.16F1823E6F3F18p+0558 },   // 6.0e+0183
        { 0x1.A5BC68F2EA955p+0610,  0x0.456F17F381C998p+0558 },   // 7.0e+0183
        { 0x1.E1FBE5A7E7861p+0610,  0x0.73ECADA8945420p+0558 },   // 8.0e+0183
        { 0x1.0F1DB12E723B6p+0611,  0x0.D13521AED36F50p+0559 },   // 9.0e+0183
        { 0x1.2D3D6F88F0B3Cp+0611,  0x0.E873EC895CB490p+0559 },   // 1.0e+0184
        { 0x1.2D3D6F88F0B3Cp+0612,  0x0.E873EC895CB490p+0560 },   // 2.0e+0184
        { 0x1.C3DC274D690DBp+0612,  0x0.5CADE2CE0B0EE0p+0560 },   // 3.0e+0184
        { 0x1.2D3D6F88F0B3Cp+0613,  0x0.E873EC895CB490p+0561 },   // 4.0e+0184
        { 0x1.788CCB6B2CE0Cp+0613,  0x0.2290E7ABB3E1B8p+0561 },   // 5.0e+0184
        { 0x1.C3DC274D690DBp+0613,  0x0.5CADE2CE0B0EE0p+0561 },   // 6.0e+0184
        { 0x1.0795C197D29D5p+0614,  0x0.4B656EF8311E00p+0562 },   // 7.0e+0184
        { 0x1.2D3D6F88F0B3Cp+0614,  0x0.E873EC895CB490p+0562 },   // 8.0e+0184
        { 0x1.52E51D7A0ECA4p+0614,  0x0.85826A1A884B28p+0562 },   // 9.0e+0184
        { 0x1.788CCB6B2CE0Cp+0614,  0x0.2290E7ABB3E1B8p+0562 },   // 1.0e+0185
        { 0x1.788CCB6B2CE0Cp+0615,  0x0.2290E7ABB3E1B8p+0563 },   // 2.0e+0185
        { 0x1.1A69989061A89p+0616,  0x0.19ECADC0C6E948p+0564 },   // 3.0e+0185
        { 0x1.788CCB6B2CE0Cp+0616,  0x0.2290E7ABB3E1B8p+0564 },   // 4.0e+0185
        { 0x1.D6AFFE45F818Fp+0616,  0x0.2B352196A0DA28p+0564 },   // 5.0e+0185
        { 0x1.1A69989061A89p+0617,  0x0.19ECADC0C6E948p+0565 },   // 6.0e+0185
        { 0x1.497B31FDC744Ap+0617,  0x0.9E3ECAB63D6580p+0565 },   // 7.0e+0185
        { 0x1.788CCB6B2CE0Cp+0617,  0x0.2290E7ABB3E1B8p+0565 },   // 8.0e+0185
        { 0x1.A79E64D8927CDp+0617,  0x0.A6E304A12A5DF0p+0565 },   // 9.0e+0185
        { 0x1.D6AFFE45F818Fp+0617,  0x0.2B352196A0DA28p+0565 },   // 1.0e+0186
        { 0x1.D6AFFE45F818Fp+0618,  0x0.2B352196A0DA28p+0566 },   // 2.0e+0186
        { 0x1.6103FEB47A12Bp+0619,  0x0.6067D930F8A3A0p+0567 },   // 3.0e+0186
        { 0x1.D6AFFE45F818Fp+0619,  0x0.2B352196A0DA28p+0567 },   // 4.0e+0186
        { 0x1.262DFEEBBB0F9p+0620,  0x0.7B0134FE248858p+0568 },   // 5.0e+0186
        { 0x1.6103FEB47A12Bp+0620,  0x0.6067D930F8A3A0p+0568 },   // 6.0e+0186
        { 0x1.9BD9FE7D3915Dp+0620,  0x0.45CE7D63CCBEE0p+0568 },   // 7.0e+0186
        { 0x1.D6AFFE45F818Fp+0620,  0x0.2B352196A0DA28p+0568 },   // 8.0e+0186
        { 0x1.08C2FF075B8E0p+0621,  0x0.884DE2E4BA7AB8p+0569 },   // 9.0e+0186
        { 0x1.262DFEEBBB0F9p+0621,  0x0.7B0134FE248858p+0569 },   // 1.0e+0187
        { 0x1.262DFEEBBB0F9p+0622,  0x0.7B0134FE248858p+0570 },   // 2.0e+0187
        { 0x1.B944FE6198976p+0622,  0x0.3881CF7D36CC88p+0570 },   // 3.0e+0187
        { 0x1.262DFEEBBB0F9p+0623,  0x0.7B0134FE248858p+0571 },   // 4.0e+0187
        { 0x1.6FB97EA6A9D37p+0623,  0x0.D9C1823DADAA70p+0571 },   // 5.0e+0187
        { 0x1.B944FE6198976p+0623,  0x0.3881CF7D36CC88p+0571 },   // 6.0e+0187
        { 0x1.01683F0E43ADAp+0624,  0x0.4BA10E5E5FF748p+0572 },   // 7.0e+0187
        { 0x1.262DFEEBBB0F9p+0624,  0x0.7B0134FE248858p+0572 },   // 8.0e+0187
        { 0x1.4AF3BEC932718p+0624,  0x0.AA615B9DE91960p+0572 },   // 9.0e+0187
        { 0x1.6FB97EA6A9D37p+0624,  0x0.D9C1823DADAA70p+0572 },   // 1.0e+0188
        { 0x1.6FB97EA6A9D37p+0625,  0x0.D9C1823DADAA70p+0573 },   // 2.0e+0188
        { 0x1.13CB1EFCFF5E9p+0626,  0x0.E35121AE423FD0p+0574 },   // 3.0e+0188
        { 0x1.6FB97EA6A9D37p+0626,  0x0.D9C1823DADAA70p+0574 },   // 4.0e+0188
        { 0x1.CBA7DE5054485p+0626,  0x0.D031E2CD191508p+0574 },   // 5.0e+0188
        { 0x1.13CB1EFCFF5E9p+0627,  0x0.E35121AE423FD0p+0575 },   // 6.0e+0188
        { 0x1.41C24ED1D4990p+0627,  0x0.DE8951F5F7F520p+0575 },   // 7.0e+0188
        { 0x1.6FB97EA6A9D37p+0627,  0x0.D9C1823DADAA70p+0575 },   // 8.0e+0188
        { 0x1.9DB0AE7B7F0DEp+0627,  0x0.D4F9B285635FB8p+0575 },   // 9.0e+0188
        { 0x1.CBA7DE5054485p+0627,  0x0.D031E2CD191508p+0575 },   // 1.0e+0189
        { 0x1.CBA7DE5054485p+0628,  0x0.D031E2CD191508p+0576 },   // 2.0e+0189
        { 0x1.58BDE6BC3F364p+0629,  0x0.5C256A19D2CFC8p+0577 },   // 3.0e+0189
        { 0x1.CBA7DE5054485p+0629,  0x0.D031E2CD191508p+0577 },   // 4.0e+0189
        { 0x1.1F48EAF234AD3p+0630,  0x0.A21F2DC02FAD28p+0578 },   // 5.0e+0189
        { 0x1.58BDE6BC3F364p+0630,  0x0.5C256A19D2CFC8p+0578 },   // 6.0e+0189
        { 0x1.9232E28649BF5p+0630,  0x0.162BA67375F268p+0578 },   // 7.0e+0189
        { 0x1.CBA7DE5054485p+0630,  0x0.D031E2CD191508p+0578 },   // 8.0e+0189
        { 0x1.028E6D0D2F68Bp+0631,  0x0.451C0F935E1BD0p+0579 },   // 9.0e+0189
        { 0x1.1F48EAF234AD3p+0631,  0x0.A21F2DC02FAD28p+0579 },   // 1.0e+0190
        { 0x1.1F48EAF234AD3p+0632,  0x0.A21F2DC02FAD28p+0580 },   // 2.0e+0190
        { 0x1.AEED606B4F03Dp+0632,  0x0.732EC4A04783B8p+0580 },   // 3.0e+0190
        { 0x1.1F48EAF234AD3p+0633,  0x0.A21F2DC02FAD28p+0581 },   // 4.0e+0190
        { 0x1.671B25AEC1D88p+0633,  0x0.8AA6F9303B9870p+0581 },   // 5.0e+0190
        { 0x1.AEED606B4F03Dp+0633,  0x0.732EC4A04783B8p+0581 },   // 6.0e+0190
        { 0x1.F6BF9B27DC2F2p+0633,  0x0.5BB69010536F00p+0581 },   // 7.0e+0190
        { 0x1.1F48EAF234AD3p+0634,  0x0.A21F2DC02FAD28p+0582 },   // 8.0e+0190
        { 0x1.433208507B42Ep+0634,  0x0.1663137835A2C8p+0582 },   // 9.0e+0190
        { 0x1.671B25AEC1D88p+0634,  0x0.8AA6F9303B9870p+0582 },   // 1.0e+0191
        { 0x1.671B25AEC1D88p+0635,  0x0.8AA6F9303B9870p+0583 },   // 2.0e+0191
        { 0x1.0D545C4311626p+0636,  0x0.67FD3AE42CB250p+0584 },   // 3.0e+0191
        { 0x1.671B25AEC1D88p+0636,  0x0.8AA6F9303B9870p+0584 },   // 4.0e+0191
        { 0x1.C0E1EF1A724EAp+0636,  0x0.AD50B77C4A7E88p+0584 },   // 5.0e+0191
        { 0x1.0D545C4311626p+0637,  0x0.67FD3AE42CB250p+0585 },   // 6.0e+0191
        { 0x1.3A37C0F8E99D7p+0637,  0x0.79521A0A342560p+0585 },   // 7.0e+0191
        { 0x1.671B25AEC1D88p+0637,  0x0.8AA6F9303B9870p+0585 },   // 8.0e+0191
        { 0x1.93FE8A649A139p+0637,  0x0.9BFBD856430B80p+0585 },   // 9.0e+0191
        { 0x1.C0E1EF1A724EAp+0637,  0x0.AD50B77C4A7E88p+0585 },   // 1.0e+0192
        { 0x1.C0E1EF1A724EAp+0638,  0x0.AD50B77C4A7E88p+0586 },   // 2.0e+0192
        { 0x1.50A97353D5BB0p+0639,  0x0.01FC899D37DEE8p+0587 },   // 3.0e+0192
        { 0x1.C0E1EF1A724EAp+0639,  0x0.AD50B77C4A7E88p+0587 },   // 4.0e+0192
        { 0x1.188D357087712p+0640,  0x0.AC5272ADAE8F18p+0588 },   // 5.0e+0192
        { 0x1.50A97353D5BB0p+0640,  0x0.01FC899D37DEE8p+0588 },   // 6.0e+0192
        { 0x1.88C5B1372404Dp+0640,  0x0.57A6A08CC12EB8p+0588 },   // 7.0e+0192
        { 0x1.C0E1EF1A724EAp+0640,  0x0.AD50B77C4A7E88p+0588 },   // 8.0e+0192
        { 0x1.F8FE2CFDC0988p+0640,  0x0.02FACE6BD3CE60p+0588 },   // 9.0e+0192
        { 0x1.188D357087712p+0641,  0x0.AC5272ADAE8F18p+0589 },   // 1.0e+0193
        { 0x1.188D357087712p+0642,  0x0.AC5272ADAE8F18p+0590 },   // 2.0e+0193
        { 0x1.A4D3D028CB29Cp+0642,  0x0.027BAC0485D6A0p+0590 },   // 3.0e+0193
        { 0x1.188D357087712p+0643,  0x0.AC5272ADAE8F18p+0591 },   // 4.0e+0193
        { 0x1.5EB082CCA94D7p+0643,  0x0.57670F591A32E0p+0591 },   // 5.0e+0193
        { 0x1.A4D3D028CB29Cp+0643,  0x0.027BAC0485D6A0p+0591 },   // 6.0e+0193
        { 0x1.EAF71D84ED060p+0643,  0x0.AD9048AFF17A68p+0591 },   // 7.0e+0193
        { 0x1.188D357087712p+0644,  0x0.AC5272ADAE8F18p+0592 },   // 8.0e+0193
        { 0x1.3B9EDC1E985F5p+0644,  0x0.01DCC1036460F8p+0592 },   // 9.0e+0193
        { 0x1.5EB082CCA94D7p+0644,  0x0.57670F591A32E0p+0592 },   // 1.0e+0194
        { 0x1.5EB082CCA94D7p+0645,  0x0.57670F591A32E0p+0593 },   // 2.0e+0194
        { 0x1.070462197EFA1p+0646,  0x0.818D4B82D3A628p+0594 },   // 3.0e+0194
        { 0x1.5EB082CCA94D7p+0646,  0x0.57670F591A32E0p+0594 },   // 4.0e+0194
        { 0x1.B65CA37FD3A0Dp+0646,  0x0.2D40D32F60BF98p+0594 },   // 5.0e+0194
        { 0x1.070462197EFA1p+0647,  0x0.818D4B82D3A628p+0595 },   // 6.0e+0194
        { 0x1.32DA72731423Cp+0647,  0x0.6C7A2D6DF6EC80p+0595 },   // 7.0e+0194
        { 0x1.5EB082CCA94D7p+0647,  0x0.57670F591A32E0p+0595 },   // 8.0e+0194
        { 0x1.8A8693263E772p+0647,  0x0.4253F1443D7938p+0595 },   // 9.0e+0194
        { 0x1.B65CA37FD3A0Dp+0647,  0x0.2D40D32F60BF98p+0595 },   // 1.0e+0195
        { 0x1.B65CA37FD3A0Dp+0648,  0x0.2D40D32F60BF98p+0596 },   // 2.0e+0195
        { 0x1.48C57A9FDEB89p+0649,  0x0.E1F09E63888FB0p+0597 },   // 3.0e+0195
        { 0x1.B65CA37FD3A0Dp+0649,  0x0.2D40D32F60BF98p+0597 },   // 4.0e+0195
        { 0x1.11F9E62FE4448p+0650,  0x0.3C4883FD9C77B8p+0598 },   // 5.0e+0195
        { 0x1.48C57A9FDEB89p+0650,  0x0.E1F09E63888FB0p+0598 },   // 6.0e+0195
        { 0x1.7F910F0FD92CBp+0650,  0x0.8798B8C974A7A0p+0598 },   // 7.0e+0195
        { 0x1.B65CA37FD3A0Dp+0650,  0x0.2D40D32F60BF98p+0598 },   // 8.0e+0195
        { 0x1.ED2837EFCE14Ep+0650,  0x0.D2E8ED954CD788p+0598 },   // 9.0e+0195
        { 0x1.11F9E62FE4448p+0651,  0x0.3C4883FD9C77B8p+0599 },   // 1.0e+0196
        { 0x1.11F9E62FE4448p+0652,  0x0.3C4883FD9C77B8p+0600 },   // 2.0e+0196
        { 0x1.9AF6D947D666Cp+0652,  0x0.5A6CC5FC6AB398p+0600 },   // 3.0e+0196
        { 0x1.11F9E62FE4448p+0653,  0x0.3C4883FD9C77B8p+0601 },   // 4.0e+0196
        { 0x1.56785FBBDD55Ap+0653,  0x0.4B5AA4FD0395A8p+0601 },   // 5.0e+0196
        { 0x1.9AF6D947D666Cp+0653,  0x0.5A6CC5FC6AB398p+0601 },   // 6.0e+0196
        { 0x1.DF7552D3CF77Ep+0653,  0x0.697EE6FBD1D188p+0601 },   // 7.0e+0196
        { 0x1.11F9E62FE4448p+0654,  0x0.3C4883FD9C77B8p+0602 },   // 8.0e+0196
        { 0x1.343922F5E0CD1p+0654,  0x0.43D1947D5006B0p+0602 },   // 9.0e+0196
        { 0x1.56785FBBDD55Ap+0654,  0x0.4B5AA4FD0395A8p+0602 },   // 1.0e+0197
        { 0x1.56785FBBDD55Ap+0655,  0x0.4B5AA4FD0395A8p+0603 },   // 2.0e+0197
        { 0x1.00DA47CCE6003p+0656,  0x0.B883FBBDC2B040p+0604 },   // 3.0e+0197
        { 0x1.56785FBBDD55Ap+0656,  0x0.4B5AA4FD0395A8p+0604 },   // 4.0e+0197
        { 0x1.AC1677AAD4AB0p+0656,  0x0.DE314E3C447B18p+0604 },   // 5.0e+0197
        { 0x1.00DA47CCE6003p+0657,  0x0.B883FBBDC2B040p+0605 },   // 6.0e+0197
        { 0x1.2BA953C461AAFp+0657,  0x0.01EF505D6322F8p+0605 },   // 7.0e+0197
        { 0x1.56785FBBDD55Ap+0657,  0x0.4B5AA4FD0395A8p+0605 },   // 8.0e+0197
        { 0x1.81476BB359005p+0657,  0x0.94C5F99CA40860p+0605 },   // 9.0e+0197
        { 0x1.AC1677AAD4AB0p+0657,  0x0.DE314E3C447B18p+0605 },   // 1.0e+0198
        { 0x1.AC1677AAD4AB0p+0658,  0x0.DE314E3C447B18p+0606 },   // 2.0e+0198
        { 0x1.4110D9C01F804p+0659,  0x0.A6A4FAAD335C50p+0607 },   // 3.0e+0198
        { 0x1.AC1677AAD4AB0p+0659,  0x0.DE314E3C447B18p+0607 },   // 4.0e+0198
        { 0x1.0B8E0ACAC4EAEp+0660,  0x0.8ADED0E5AACCF0p+0608 },   // 5.0e+0198
        { 0x1.4110D9C01F804p+0660,  0x0.A6A4FAAD335C50p+0608 },   // 6.0e+0198
        { 0x1.7693A8B57A15Ap+0660,  0x0.C26B2474BBEBB0p+0608 },   // 7.0e+0198
        { 0x1.AC1677AAD4AB0p+0660,  0x0.DE314E3C447B18p+0608 },   // 8.0e+0198
        { 0x1.E19946A02F406p+0660,  0x0.F9F77803CD0A78p+0608 },   // 9.0e+0198
        { 0x1.0B8E0ACAC4EAEp+0661,  0x0.8ADED0E5AACCF0p+0609 },   // 1.0e+0199
        { 0x1.0B8E0ACAC4EAEp+0662,  0x0.8ADED0E5AACCF0p+0610 },   // 2.0e+0199
        { 0x1.9155103027605p+0662,  0x0.D04E3958803368p+0610 },   // 3.0e+0199
        { 0x1.0B8E0ACAC4EAEp+0663,  0x0.8ADED0E5AACCF0p+0611 },   // 4.0e+0199
        { 0x1.4E718D7D7625Ap+0663,  0x0.2D96851F158028p+0611 },   // 5.0e+0199
        { 0x1.9155103027605p+0663,  0x0.D04E3958803368p+0611 },   // 6.0e+0199
        { 0x1.D43892E2D89B1p+0663,  0x0.7305ED91EAE6A0p+0611 },   // 7.0e+0199
        { 0x1.0B8E0ACAC4EAEp+0664,  0x0.8ADED0E5AACCF0p+0612 },   // 8.0e+0199
        { 0x1.2CFFCC241D884p+0664,  0x0.5C3AAB02602688p+0612 },   // 9.0e+0199
        { 0x1.4E718D7D7625Ap+0664,  0x0.2D96851F158028p+0612 },   // 1.0e+0200
        { 0x1.4E718D7D7625Ap+0665,  0x0.2D96851F158028p+0613 },   // 2.0e+0200
        { 0x1.F5AA543C31387p+0665,  0x0.4461C7AEA04040p+0613 },   // 3.0e+0200
        { 0x1.4E718D7D7625Ap+0666,  0x0.2D96851F158028p+0614 },   // 4.0e+0200
        { 0x1.A20DF0DCD3AF0p+0666,  0x0.B8FC2666DAE030p+0614 },   // 5.0e+0200
        { 0x1.F5AA543C31387p+0666,  0x0.4461C7AEA04040p+0614 },   // 6.0e+0200
        { 0x1.24A35BCDC760Ep+0667,  0x0.E7E3B47B32D020p+0615 },   // 7.0e+0200
        { 0x1.4E718D7D7625Ap+0667,  0x0.2D96851F158028p+0615 },   // 8.0e+0200
        { 0x1.783FBF2D24EA5p+0667,  0x0.734955C2F83030p+0615 },   // 9.0e+0200
        { 0x1.A20DF0DCD3AF0p+0667,  0x0.B8FC2666DAE030p+0615 },   // 1.0e+0201
        { 0x1.A20DF0DCD3AF0p+0668,  0x0.B8FC2666DAE030p+0616 },   // 2.0e+0201
        { 0x1.398A74A59EC34p+0669,  0x0.8ABD1CCD242828p+0617 },   // 3.0e+0201
        { 0x1.A20DF0DCD3AF0p+0669,  0x0.B8FC2666DAE030p+0617 },   // 4.0e+0201
        { 0x1.0548B68A044D6p+0670,  0x0.739D980048CC20p+0618 },   // 5.0e+0201
        { 0x1.398A74A59EC34p+0670,  0x0.8ABD1CCD242828p+0618 },   // 6.0e+0201
        { 0x1.6DCC32C139392p+0670,  0x0.A1DCA199FF8430p+0618 },   // 7.0e+0201
        { 0x1.A20DF0DCD3AF0p+0670,  0x0.B8FC2666DAE030p+0618 },   // 8.0e+0201
        { 0x1.D64FAEF86E24Ep+0670,  0x0.D01BAB33B63C38p+0618 },   // 9.0e+0201
        { 0x1.0548B68A044D6p+0671,  0x0.739D980048CC20p+0619 },   // 1.0e+0202
        { 0x1.0548B68A044D6p+0672,  0x0.739D980048CC20p+0620 },   // 2.0e+0202
        { 0x1.87ED11CF06741p+0672,  0x0.AD6C64006D3230p+0620 },   // 3.0e+0202
        { 0x1.0548B68A044D6p+0673,  0x0.739D980048CC20p+0621 },   // 4.0e+0202
        { 0x1.469AE42C8560Cp+0673,  0x0.1084FE005AFF28p+0621 },   // 5.0e+0202
        { 0x1.87ED11CF06741p+0673,  0x0.AD6C64006D3230p+0621 },   // 6.0e+0202
        { 0x1.C93F3F7187877p+0673,  0x0.4A53CA007F6538p+0621 },   // 7.0e+0202
        { 0x1.0548B68A044D6p+0674,  0x0.739D980048CC20p+0622 },   // 8.0e+0202
        { 0x1.25F1CD5B44D71p+0674,  0x0.42114B0051E5A0p+0622 },   // 9.0e+0202
        { 0x1.469AE42C8560Cp+0674,  0x0.1084FE005AFF28p+0622 },   // 1.0e+0203
        { 0x1.469AE42C8560Cp+0675,  0x0.1084FE005AFF28p+0623 },   // 2.0e+0203
        { 0x1.E9E85642C8112p+0675,  0x0.18C77D00887EC0p+0623 },   // 3.0e+0203
        { 0x1.469AE42C8560Cp+0676,  0x0.1084FE005AFF28p+0624 },   // 4.0e+0203
        { 0x1.98419D37A6B8Fp+0676,  0x0.14A63D8071BEF0p+0624 },   // 5.0e+0203
        { 0x1.E9E85642C8112p+0676,  0x0.18C77D00887EC0p+0624 },   // 6.0e+0203
        { 0x1.1DC787A6F4B4Ap+0677,  0x0.8E745E404F9F40p+0625 },   // 7.0e+0203
        { 0x1.469AE42C8560Cp+0677,  0x0.1084FE005AFF28p+0625 },   // 8.0e+0203
        { 0x1.6F6E40B2160CDp+0677,  0x0.92959DC0665F10p+0625 },   // 9.0e+0203
        { 0x1.98419D37A6B8Fp+0677,  0x0.14A63D8071BEF0p+0625 },   // 1.0e+0204
        { 0x1.98419D37A6B8Fp+0678,  0x0.14A63D8071BEF0p+0626 },   // 2.0e+0204
        { 0x1.323135E9BD0ABp+0679,  0x0.4F7CAE20554F38p+0627 },   // 3.0e+0204
        { 0x1.98419D37A6B8Fp+0679,  0x0.14A63D8071BEF0p+0627 },   // 4.0e+0204
        { 0x1.FE52048590672p+0679,  0x0.D9CFCCE08E2EB0p+0627 },   // 5.0e+0204
        { 0x1.323135E9BD0ABp+0680,  0x0.4F7CAE20554F38p+0628 },   // 6.0e+0204
        { 0x1.65396990B1E1Dp+0680,  0x0.321175D0638710p+0628 },   // 7.0e+0204
        { 0x1.98419D37A6B8Fp+0680,  0x0.14A63D8071BEF0p+0628 },   // 8.0e+0204
        { 0x1.CB49D0DE9B900p+0680,  0x0.F73B05307FF6D0p+0628 },   // 9.0e+0204
        { 0x1.FE52048590672p+0680,  0x0.D9CFCCE08E2EB0p+0628 },   // 1.0e+0205
        { 0x1.FE52048590672p+0681,  0x0.D9CFCCE08E2EB0p+0629 },   // 2.0e+0205
        { 0x1.7EBD83642C4D6p+0682,  0x0.235BD9A86AA300p+0630 },   // 3.0e+0205
        { 0x1.FE52048590672p+0682,  0x0.D9CFCCE08E2EB0p+0630 },   // 4.0e+0205
        { 0x1.3EF342D37A407p+0683,  0x0.C821E00C58DD30p+0631 },   // 5.0e+0205
        { 0x1.7EBD83642C4D6p+0683,  0x0.235BD9A86AA300p+0631 },   // 6.0e+0205
        { 0x1.BE87C3F4DE5A4p+0683,  0x0.7E95D3447C68D8p+0631 },   // 7.0e+0205
        { 0x1.FE52048590672p+0683,  0x0.D9CFCCE08E2EB0p+0631 },   // 8.0e+0205
        { 0x1.1F0E228B213A0p+0684,  0x0.9A84E33E4FFA40p+0632 },   // 9.0e+0205
        { 0x1.3EF342D37A407p+0684,  0x0.C821E00C58DD30p+0632 },   // 1.0e+0206
        { 0x1.3EF342D37A407p+0685,  0x0.C821E00C58DD30p+0633 },   // 2.0e+0206
        { 0x1.DE6CE43D3760Bp+0685,  0x0.AC32D012854BC8p+0633 },   // 3.0e+0206
        { 0x1.3EF342D37A407p+0686,  0x0.C821E00C58DD30p+0634 },   // 4.0e+0206
        { 0x1.8EB0138858D09p+0686,  0x0.BA2A580F6F1478p+0634 },   // 5.0e+0206
        { 0x1.DE6CE43D3760Bp+0686,  0x0.AC32D012854BC8p+0634 },   // 6.0e+0206
        { 0x1.1714DA790AF86p+0687,  0x0.CF1DA40ACDC188p+0635 },   // 7.0e+0206
        { 0x1.3EF342D37A407p+0687,  0x0.C821E00C58DD30p+0635 },   // 8.0e+0206
        { 0x1.66D1AB2DE9888p+0687,  0x0.C1261C0DE3F8D0p+0635 },   // 9.0e+0206
        { 0x1.8EB0138858D09p+0687,  0x0.BA2A580F6F1478p+0635 },   // 1.0e+0207
        { 0x1.8EB0138858D09p+0688,  0x0.BA2A580F6F1478p+0636 },   // 2.0e+0207
        { 0x1.2B040EA6429C7p+0689,  0x0.4B9FC20B934F58p+0637 },   // 3.0e+0207
        { 0x1.8EB0138858D09p+0689,  0x0.BA2A580F6F1478p+0637 },   // 4.0e+0207
        { 0x1.F25C186A6F04Cp+0689,  0x0.28B4EE134AD998p+0637 },   // 5.0e+0207
        { 0x1.2B040EA6429C7p+0690,  0x0.4B9FC20B934F58p+0638 },   // 6.0e+0207
        { 0x1.5CDA11174DB68p+0690,  0x0.82E50D0D8131E8p+0638 },   // 7.0e+0207
        { 0x1.8EB0138858D09p+0690,  0x0.BA2A580F6F1478p+0638 },   // 8.0e+0207
        { 0x1.C08615F963EAAp+0690,  0x0.F16FA3115CF708p+0638 },   // 9.0e+0207
        { 0x1.F25C186A6F04Cp+0690,  0x0.28B4EE134AD998p+0638 },   // 1.0e+0208
        { 0x1.F25C186A6F04Cp+0691,  0x0.28B4EE134AD998p+0639 },   // 2.0e+0208
        { 0x1.75C5124FD3439p+0692,  0x0.1E87B28E782330p+0640 },   // 3.0e+0208
        { 0x1.F25C186A6F04Cp+0692,  0x0.28B4EE134AD998p+0640 },   // 4.0e+0208
        { 0x1.37798F428562Fp+0693,  0x0.997114CC0EC800p+0641 },   // 5.0e+0208
        { 0x1.75C5124FD3439p+0693,  0x0.1E87B28E782330p+0641 },   // 6.0e+0208
        { 0x1.B410955D21242p+0693,  0x0.A39E5050E17E68p+0641 },   // 7.0e+0208
        { 0x1.F25C186A6F04Cp+0693,  0x0.28B4EE134AD998p+0641 },   // 8.0e+0208
        { 0x1.1853CDBBDE72Ap+0694,  0x0.D6E5C5EADA1A60p+0642 },   // 9.0e+0208
        { 0x1.37798F428562Fp+0694,  0x0.997114CC0EC800p+0642 },   // 1.0e+0209
        { 0x1.37798F428562Fp+0695,  0x0.997114CC0EC800p+0643 },   // 2.0e+0209
        { 0x1.D33656E3C8147p+0695,  0x0.66299F32162C00p+0643 },   // 3.0e+0209
        { 0x1.37798F428562Fp+0696,  0x0.997114CC0EC800p+0644 },   // 4.0e+0209
        { 0x1.8557F31326BBBp+0696,  0x0.7FCD59FF127A00p+0644 },   // 5.0e+0209
        { 0x1.D33656E3C8147p+0696,  0x0.66299F32162C00p+0644 },   // 6.0e+0209
        { 0x1.108A5D5A34B69p+0697,  0x0.A642F2328CEF00p+0645 },   // 7.0e+0209
        { 0x1.37798F428562Fp+0697,  0x0.997114CC0EC800p+0645 },   // 8.0e+0209
        { 0x1.5E68C12AD60F5p+0697,  0x0.8C9F376590A100p+0645 },   // 9.0e+0209
        { 0x1.8557F31326BBBp+0697,  0x0.7FCD59FF127A00p+0645 },   // 1.0e+0210
        { 0x1.8557F31326BBBp+0698,  0x0.7FCD59FF127A00p+0646 },   // 2.0e+0210
        { 0x1.2401F64E5D0CCp+0699,  0x0.9FDA037F4DDB80p+0647 },   // 3.0e+0210
        { 0x1.8557F31326BBBp+0699,  0x0.7FCD59FF127A00p+0647 },   // 4.0e+0210
        { 0x1.E6ADEFD7F06AAp+0699,  0x0.5FC0B07ED71880p+0647 },   // 5.0e+0210
        { 0x1.2401F64E5D0CCp+0700,  0x0.9FDA037F4DDB80p+0648 },   // 6.0e+0210
        { 0x1.54ACF4B0C1E44p+0700,  0x0.0FD3AEBF302AC0p+0648 },   // 7.0e+0210
        { 0x1.8557F31326BBBp+0700,  0x0.7FCD59FF127A00p+0648 },   // 8.0e+0210
        { 0x1.B602F1758B932p+0700,  0x0.EFC7053EF4C940p+0648 },   // 9.0e+0210
        { 0x1.E6ADEFD7F06AAp+0700,  0x0.5FC0B07ED71880p+0648 },   // 1.0e+0211
        { 0x1.E6ADEFD7F06AAp+0701,  0x0.5FC0B07ED71880p+0649 },   // 2.0e+0211
        { 0x1.6D0273E1F44FFp+0702,  0x0.C7D0845F215260p+0650 },   // 3.0e+0211
        { 0x1.E6ADEFD7F06AAp+0702,  0x0.5FC0B07ED71880p+0650 },   // 4.0e+0211
        { 0x1.302CB5E6F642Ap+0703,  0x0.7BD86E4F466F50p+0651 },   // 5.0e+0211
        { 0x1.6D0273E1F44FFp+0703,  0x0.C7D0845F215260p+0651 },   // 6.0e+0211
        { 0x1.A9D831DCF25D5p+0703,  0x0.13C89A6EFC3570p+0651 },   // 7.0e+0211
        { 0x1.E6ADEFD7F06AAp+0703,  0x0.5FC0B07ED71880p+0651 },   // 8.0e+0211
        { 0x1.11C1D6E9773BFp+0704,  0x0.D5DC634758FDC8p+0652 },   // 9.0e+0211
        { 0x1.302CB5E6F642Ap+0704,  0x0.7BD86E4F466F50p+0652 },   // 1.0e+0212
        { 0x1.302CB5E6F642Ap+0705,  0x0.7BD86E4F466F50p+0653 },   // 2.0e+0212
        { 0x1.C84310DA7163Fp+0705,  0x0.B9C4A576E9A6F8p+0653 },   // 3.0e+0212
        { 0x1.302CB5E6F642Ap+0706,  0x0.7BD86E4F466F50p+0654 },   // 4.0e+0212
        { 0x1.7C37E360B3D35p+0706,  0x0.1ACE89E3180B20p+0654 },   // 5.0e+0212
        { 0x1.C84310DA7163Fp+0706,  0x0.B9C4A576E9A6F8p+0654 },   // 6.0e+0212
        { 0x1.0A271F2A177A5p+0707,  0x0.2C5D60855DA160p+0655 },   // 7.0e+0212
        { 0x1.302CB5E6F642Ap+0707,  0x0.7BD86E4F466F50p+0655 },   // 8.0e+0212
        { 0x1.56324CA3D50AFp+0707,  0x0.CB537C192F3D38p+0655 },   // 9.0e+0212
        { 0x1.7C37E360B3D35p+0707,  0x0.1ACE89E3180B20p+0655 },   // 1.0e+0213
        { 0x1.7C37E360B3D35p+0708,  0x0.1ACE89E3180B20p+0656 },   // 2.0e+0213
        { 0x1.1D29EA8886DE7p+0709,  0x0.D41AE76A520858p+0657 },   // 3.0e+0213
        { 0x1.7C37E360B3D35p+0709,  0x0.1ACE89E3180B20p+0657 },   // 4.0e+0213
        { 0x1.DB45DC38E0C82p+0709,  0x0.61822C5BDE0DE8p+0657 },   // 5.0e+0213
        { 0x1.1D29EA8886DE7p+0710,  0x0.D41AE76A520858p+0658 },   // 6.0e+0213
        { 0x1.4CB0E6F49D58Ep+0710,  0x0.7774B8A6B509C0p+0658 },   // 7.0e+0213
        { 0x1.7C37E360B3D35p+0710,  0x0.1ACE89E3180B20p+0658 },   // 8.0e+0213
        { 0x1.ABBEDFCCCA4DBp+0710,  0x0.BE285B1F7B0C88p+0658 },   // 9.0e+0213
        { 0x1.DB45DC38E0C82p+0710,  0x0.61822C5BDE0DE8p+0658 },   // 1.0e+0214
        { 0x1.DB45DC38E0C82p+0711,  0x0.61822C5BDE0DE8p+0659 },   // 2.0e+0214
        { 0x1.6474652AA8961p+0712,  0x0.C921A144E68A70p+0660 },   // 3.0e+0214
        { 0x1.DB45DC38E0C82p+0712,  0x0.61822C5BDE0DE8p+0660 },   // 4.0e+0214
        { 0x1.290BA9A38C7D1p+0713,  0x0.7CF15BB96AC8B0p+0661 },   // 5.0e+0214
        { 0x1.6474652AA8961p+0713,  0x0.C921A144E68A70p+0661 },   // 6.0e+0214
        { 0x1.9FDD20B1C4AF2p+0713,  0x0.1551E6D0624C30p+0661 },   // 7.0e+0214
        { 0x1.DB45DC38E0C82p+0713,  0x0.61822C5BDE0DE8p+0661 },   // 8.0e+0214
        { 0x1.0B574BDFFE709p+0714,  0x0.56D938F3ACE7D0p+0662 },   // 9.0e+0214
        { 0x1.290BA9A38C7D1p+0714,  0x0.7CF15BB96AC8B0p+0662 },   // 1.0e+0215
        { 0x1.290BA9A38C7D1p+0715,  0x0.7CF15BB96AC8B0p+0663 },   // 2.0e+0215
        { 0x1.BD917E7552BBAp+0715,  0x0.3B6A0996202D10p+0663 },   // 3.0e+0215
        { 0x1.290BA9A38C7D1p+0716,  0x0.7CF15BB96AC8B0p+0664 },   // 4.0e+0215
        { 0x1.734E940C6F9C5p+0716,  0x0.DC2DB2A7C57AE0p+0664 },   // 5.0e+0215
        { 0x1.BD917E7552BBAp+0716,  0x0.3B6A0996202D10p+0664 },   // 6.0e+0215
        { 0x1.03EA346F1AED7p+0717,  0x0.4D5330423D6F98p+0665 },   // 7.0e+0215
        { 0x1.290BA9A38C7D1p+0717,  0x0.7CF15BB96AC8B0p+0665 },   // 8.0e+0215
        { 0x1.4E2D1ED7FE0CBp+0717,  0x0.AC8F87309821C8p+0665 },   // 9.0e+0215
        { 0x1.734E940C6F9C5p+0717,  0x0.DC2DB2A7C57AE0p+0665 },   // 1.0e+0216
        { 0x1.734E940C6F9C5p+0718,  0x0.DC2DB2A7C57AE0p+0666 },   // 2.0e+0216
        { 0x1.167AEF0953B54p+0719,  0x0.652245FDD41C28p+0667 },   // 3.0e+0216
        { 0x1.734E940C6F9C5p+0719,  0x0.DC2DB2A7C57AE0p+0667 },   // 4.0e+0216
        { 0x1.D022390F8B837p+0719,  0x0.53391F51B6D998p+0667 },   // 5.0e+0216
        { 0x1.167AEF0953B54p+0720,  0x0.652245FDD41C28p+0668 },   // 6.0e+0216
        { 0x1.44E4C18AE1A8Dp+0720,  0x0.20A7FC52CCCB80p+0668 },   // 7.0e+0216
        { 0x1.734E940C6F9C5p+0720,  0x0.DC2DB2A7C57AE0p+0668 },   // 8.0e+0216
        { 0x1.A1B8668DFD8FEp+0720,  0x0.97B368FCBE2A38p+0668 },   // 9.0e+0216
        { 0x1.D022390F8B837p+0720,  0x0.53391F51B6D998p+0668 },   // 1.0e+0217
        { 0x1.D022390F8B837p+0721,  0x0.53391F51B6D998p+0669 },   // 2.0e+0217
        { 0x1.5C19AACBA8A29p+0722,  0x0.7E6AD77D492330p+0670 },   // 3.0e+0217
        { 0x1.D022390F8B837p+0722,  0x0.53391F51B6D998p+0670 },   // 4.0e+0217
        { 0x1.221563A9B7322p+0723,  0x0.9403B393124800p+0671 },   // 5.0e+0217
        { 0x1.5C19AACBA8A29p+0723,  0x0.7E6AD77D492330p+0671 },   // 6.0e+0217
        { 0x1.961DF1ED9A130p+0723,  0x0.68D1FB677FFE68p+0671 },   // 7.0e+0217
        { 0x1.D022390F8B837p+0723,  0x0.53391F51B6D998p+0671 },   // 8.0e+0217
        { 0x1.05134018BE79Fp+0724,  0x0.1ED0219DF6DA60p+0672 },   // 9.0e+0217
        { 0x1.221563A9B7322p+0724,  0x0.9403B393124800p+0672 },   // 1.0e+0218
        { 0x1.221563A9B7322p+0725,  0x0.9403B393124800p+0673 },   // 2.0e+0218
        { 0x1.B320157E92CB3p+0725,  0x0.DE058D5C9B6C00p+0673 },   // 3.0e+0218
        { 0x1.221563A9B7322p+0726,  0x0.9403B393124800p+0674 },   // 4.0e+0218
        { 0x1.6A9ABC9424FEBp+0726,  0x0.3904A077D6DA00p+0674 },   // 5.0e+0218
        { 0x1.B320157E92CB3p+0726,  0x0.DE058D5C9B6C00p+0674 },   // 6.0e+0218
        { 0x1.FBA56E690097Cp+0726,  0x0.83067A415FFE00p+0674 },   // 7.0e+0218
        { 0x1.221563A9B7322p+0727,  0x0.9403B393124800p+0675 },   // 8.0e+0218
        { 0x1.4658101EEE186p+0727,  0x0.E6842A05749100p+0675 },   // 9.0e+0218
        { 0x1.6A9ABC9424FEBp+0727,  0x0.3904A077D6DA00p+0675 },   // 1.0e+0219
        { 0x1.6A9ABC9424FEBp+0728,  0x0.3904A077D6DA00p+0676 },   // 2.0e+0219
        { 0x1.0FF40D6F1BBF0p+0729,  0x0.6AC37859E12380p+0677 },   // 3.0e+0219
        { 0x1.6A9ABC9424FEBp+0729,  0x0.3904A077D6DA00p+0677 },   // 4.0e+0219
        { 0x1.C5416BB92E3E6p+0729,  0x0.0745C895CC9080p+0677 },   // 5.0e+0219
        { 0x1.0FF40D6F1BBF0p+0730,  0x0.6AC37859E12380p+0678 },   // 6.0e+0219
        { 0x1.3D476501A05EDp+0730,  0x0.D1E40C68DBFEC0p+0678 },   // 7.0e+0219
        { 0x1.6A9ABC9424FEBp+0730,  0x0.3904A077D6DA00p+0678 },   // 8.0e+0219
        { 0x1.97EE1426A99E8p+0730,  0x0.A0253486D1B540p+0678 },   // 9.0e+0219
        { 0x1.C5416BB92E3E6p+0730,  0x0.0745C895CC9080p+0678 },   // 1.0e+0220
        { 0x1.C5416BB92E3E6p+0731,  0x0.0745C895CC9080p+0679 },   // 2.0e+0220
        { 0x1.53F110CAE2AECp+0732,  0x0.85745670596C60p+0680 },   // 3.0e+0220
        { 0x1.C5416BB92E3E6p+0732,  0x0.0745C895CC9080p+0680 },   // 4.0e+0220
        { 0x1.1B48E353BCE6Fp+0733,  0x0.C48B9D5D9FDA50p+0681 },   // 5.0e+0220
        { 0x1.53F110CAE2AECp+0733,  0x0.85745670596C60p+0681 },   // 6.0e+0220
        { 0x1.8C993E4208769p+0733,  0x0.465D0F8312FE70p+0681 },   // 7.0e+0220
        { 0x1.C5416BB92E3E6p+0733,  0x0.0745C895CC9080p+0681 },   // 8.0e+0220
        { 0x1.FDE9993054062p+0733,  0x0.C82E81A8862290p+0681 },   // 9.0e+0220
        { 0x1.1B48E353BCE6Fp+0734,  0x0.C48B9D5D9FDA50p+0682 },   // 1.0e+0221
        { 0x1.1B48E353BCE6Fp+0735,  0x0.C48B9D5D9FDA50p+0683 },   // 2.0e+0221
        { 0x1.A8ED54FD9B5A7p+0735,  0x0.A6D16C0C6FC778p+0683 },   // 3.0e+0221
        { 0x1.1B48E353BCE6Fp+0736,  0x0.C48B9D5D9FDA50p+0684 },   // 4.0e+0221
        { 0x1.621B1C28AC20Bp+0736,  0x0.B5AE84B507D0E0p+0684 },   // 5.0e+0221
        { 0x1.A8ED54FD9B5A7p+0736,  0x0.A6D16C0C6FC778p+0684 },   // 6.0e+0221
        { 0x1.EFBF8DD28A943p+0736,  0x0.97F45363D7BE08p+0684 },   // 7.0e+0221
        { 0x1.1B48E353BCE6Fp+0737,  0x0.C48B9D5D9FDA50p+0685 },   // 8.0e+0221
        { 0x1.3EB1FFBE3483Dp+0737,  0x0.BD1D110953D598p+0685 },   // 9.0e+0221
        { 0x1.621B1C28AC20Bp+0737,  0x0.B5AE84B507D0E0p+0685 },   // 1.0e+0222
        { 0x1.621B1C28AC20Bp+0738,  0x0.B5AE84B507D0E0p+0686 },   // 2.0e+0222
        { 0x1.0994551E81188p+0739,  0x0.C842E387C5DCA8p+0687 },   // 3.0e+0222
        { 0x1.621B1C28AC20Bp+0739,  0x0.B5AE84B507D0E0p+0687 },   // 4.0e+0222
        { 0x1.BAA1E332D728Ep+0739,  0x0.A31A25E249C518p+0687 },   // 5.0e+0222
        { 0x1.0994551E81188p+0740,  0x0.C842E387C5DCA8p+0688 },   // 6.0e+0222
        { 0x1.35D7B8A3969CAp+0740,  0x0.3EF8B41E66D6C8p+0688 },   // 7.0e+0222
        { 0x1.621B1C28AC20Bp+0740,  0x0.B5AE84B507D0E0p+0688 },   // 8.0e+0222
        { 0x1.8E5E7FADC1A4Dp+0740,  0x0.2C64554BA8CB00p+0688 },   // 9.0e+0222
        { 0x1.BAA1E332D728Ep+0740,  0x0.A31A25E249C518p+0688 },   // 1.0e+0223
        { 0x1.BAA1E332D728Ep+0741,  0x0.A31A25E249C518p+0689 },   // 2.0e+0223
        { 0x1.4BF96A66215EAp+0742,  0x0.FA539C69B753D0p+0690 },   // 3.0e+0223
        { 0x1.BAA1E332D728Ep+0742,  0x0.A31A25E249C518p+0690 },   // 4.0e+0223
        { 0x1.14A52DFFC6799p+0743,  0x0.25F057AD6E1B30p+0691 },   // 5.0e+0223
        { 0x1.4BF96A66215EAp+0743,  0x0.FA539C69B753D0p+0691 },   // 6.0e+0223
        { 0x1.834DA6CC7C43Cp+0743,  0x0.CEB6E126008C78p+0691 },   // 7.0e+0223
        { 0x1.BAA1E332D728Ep+0743,  0x0.A31A25E249C518p+0691 },   // 8.0e+0223
        { 0x1.F1F61F99320E0p+0743,  0x0.777D6A9E92FDC0p+0691 },   // 9.0e+0223
        { 0x1.14A52DFFC6799p+0744,  0x0.25F057AD6E1B30p+0692 },   // 1.0e+0224
        { 0x1.14A52DFFC6799p+0745,  0x0.25F057AD6E1B30p+0693 },   // 2.0e+0224
        { 0x1.9EF7C4FFA9B65p+0745,  0x0.B8E883842528C8p+0693 },   // 3.0e+0224
        { 0x1.14A52DFFC6799p+0746,  0x0.25F057AD6E1B30p+0694 },   // 4.0e+0224
        { 0x1.59CE797FB817Fp+0746,  0x0.6F6C6D98C9A200p+0694 },   // 5.0e+0224
        { 0x1.9EF7C4FFA9B65p+0746,  0x0.B8E883842528C8p+0694 },   // 6.0e+0224
        { 0x1.E421107F9B54Cp+0746,  0x0.0264996F80AF98p+0694 },   // 7.0e+0224
        { 0x1.14A52DFFC6799p+0747,  0x0.25F057AD6E1B30p+0695 },   // 8.0e+0224
        { 0x1.3739D3BFBF48Cp+0747,  0x0.4AAE62A31BDE98p+0695 },   // 9.0e+0224
        { 0x1.59CE797FB817Fp+0747,  0x0.6F6C6D98C9A200p+0695 },   // 1.0e+0225
        { 0x1.59CE797FB817Fp+0748,  0x0.6F6C6D98C9A200p+0696 },   // 2.0e+0225
        { 0x1.035ADB1FCA11Fp+0749,  0x0.93915232973980p+0697 },   // 3.0e+0225
        { 0x1.59CE797FB817Fp+0749,  0x0.6F6C6D98C9A200p+0697 },   // 4.0e+0225
        { 0x1.B04217DFA61DFp+0749,  0x0.4B4788FEFC0A80p+0697 },   // 5.0e+0225
        { 0x1.035ADB1FCA11Fp+0750,  0x0.93915232973980p+0698 },   // 6.0e+0225
        { 0x1.2E94AA4FC114Fp+0750,  0x0.817EDFE5B06DC0p+0698 },   // 7.0e+0225
        { 0x1.59CE797FB817Fp+0750,  0x0.6F6C6D98C9A200p+0698 },   // 8.0e+0225
        { 0x1.850848AFAF1AFp+0750,  0x0.5D59FB4BE2D640p+0698 },   // 9.0e+0225
        { 0x1.B04217DFA61DFp+0750,  0x0.4B4788FEFC0A80p+0698 },   // 1.0e+0226
        { 0x1.B04217DFA61DFp+0751,  0x0.4B4788FEFC0A80p+0699 },   // 2.0e+0226
        { 0x1.443191E7BC967p+0752,  0x0.7875A6BF3D07E0p+0700 },   // 3.0e+0226
        { 0x1.B04217DFA61DFp+0752,  0x0.4B4788FEFC0A80p+0700 },   // 4.0e+0226
        { 0x1.0E294EEBC7D2Bp+0753,  0x0.8F0CB59F5D8690p+0701 },   // 5.0e+0226
        { 0x1.443191E7BC967p+0753,  0x0.7875A6BF3D07E0p+0701 },   // 6.0e+0226
        { 0x1.7A39D4E3B15A3p+0753,  0x0.61DE97DF1C8930p+0701 },   // 7.0e+0226
        { 0x1.B04217DFA61DFp+0753,  0x0.4B4788FEFC0A80p+0701 },   // 8.0e+0226
        { 0x1.E64A5ADB9AE1Bp+0753,  0x0.34B07A1EDB8BD0p+0701 },   // 9.0e+0226
        { 0x1.0E294EEBC7D2Bp+0754,  0x0.8F0CB59F5D8690p+0702 },   // 1.0e+0227
        { 0x1.0E294EEBC7D2Bp+0755,  0x0.8F0CB59F5D8690p+0703 },   // 2.0e+0227
        { 0x1.953DF661ABBC1p+0755,  0x0.5693106F0C49D8p+0703 },   // 3.0e+0227
        { 0x1.0E294EEBC7D2Bp+0756,  0x0.8F0CB59F5D8690p+0704 },   // 4.0e+0227
        { 0x1.51B3A2A6B9C76p+0756,  0x0.72CFE30734E830p+0704 },   // 5.0e+0227
        { 0x1.953DF661ABBC1p+0756,  0x0.5693106F0C49D8p+0704 },   // 6.0e+0227
        { 0x1.D8C84A1C9DB0Cp+0756,  0x0.3A563DD6E3AB78p+0704 },   // 7.0e+0227
        { 0x1.0E294EEBC7D2Bp+0757,  0x0.8F0CB59F5D8690p+0705 },   // 8.0e+0227
        { 0x1.2FEE78C940CD1p+0757,  0x0.00EE4C53493760p+0705 },   // 9.0e+0227
        { 0x1.51B3A2A6B9C76p+0757,  0x0.72CFE30734E830p+0705 },   // 1.0e+0228
        { 0x1.51B3A2A6B9C76p+0758,  0x0.72CFE30734E830p+0706 },   // 2.0e+0228
        { 0x1.FA8D73FA16AB1p+0758,  0x0.AC37D48ACF5C48p+0706 },   // 3.0e+0228
        { 0x1.51B3A2A6B9C76p+0759,  0x0.72CFE30734E830p+0707 },   // 4.0e+0228
        { 0x1.A6208B5068394p+0759,  0x0.0F83DBC9022240p+0707 },   // 5.0e+0228
        { 0x1.FA8D73FA16AB1p+0759,  0x0.AC37D48ACF5C48p+0707 },   // 6.0e+0228
        { 0x1.277D2E51E28E7p+0760,  0x0.A475E6A64E4B28p+0708 },   // 7.0e+0228
        { 0x1.51B3A2A6B9C76p+0760,  0x0.72CFE30734E830p+0708 },   // 8.0e+0228
        { 0x1.7BEA16FB91005p+0760,  0x0.4129DF681B8538p+0708 },   // 9.0e+0228
        { 0x1.A6208B5068394p+0760,  0x0.0F83DBC9022240p+0708 },   // 1.0e+0229
        { 0x1.A6208B5068394p+0761,  0x0.0F83DBC9022240p+0709 },   // 2.0e+0229
        { 0x1.3C98687C4E2AFp+0762,  0x0.0BA2E4D6C199B0p+0710 },   // 3.0e+0229
        { 0x1.A6208B5068394p+0762,  0x0.0F83DBC9022240p+0710 },   // 4.0e+0229
        { 0x1.07D457124123Cp+0763,  0x0.89B2695DA15568p+0711 },   // 5.0e+0229
        { 0x1.3C98687C4E2AFp+0763,  0x0.0BA2E4D6C199B0p+0711 },   // 6.0e+0229
        { 0x1.715C79E65B321p+0763,  0x0.8D93604FE1DDF8p+0711 },   // 7.0e+0229
        { 0x1.A6208B5068394p+0763,  0x0.0F83DBC9022240p+0711 },   // 8.0e+0229
        { 0x1.DAE49CBA75406p+0763,  0x0.91745742226688p+0711 },   // 9.0e+0229
        { 0x1.07D457124123Cp+0764,  0x0.89B2695DA15568p+0712 },   // 1.0e+0230
        { 0x1.07D457124123Cp+0765,  0x0.89B2695DA15568p+0713 },   // 2.0e+0230
        { 0x1.8BBE829B61B5Ap+0765,  0x0.CE8B9E0C720018p+0713 },   // 3.0e+0230
        { 0x1.07D457124123Cp+0766,  0x0.89B2695DA15568p+0714 },   // 4.0e+0230
        { 0x1.49C96CD6D16CBp+0766,  0x0.AC1F03B509AAC0p+0714 },   // 5.0e+0230
        { 0x1.8BBE829B61B5Ap+0766,  0x0.CE8B9E0C720018p+0714 },   // 6.0e+0230
        { 0x1.CDB3985FF1FE9p+0766,  0x0.F0F83863DA5570p+0714 },   // 7.0e+0230
        { 0x1.07D457124123Cp+0767,  0x0.89B2695DA15568p+0715 },   // 8.0e+0230
        { 0x1.28CEE1F489484p+0767,  0x0.1AE8B689558010p+0715 },   // 9.0e+0230
        { 0x1.49C96CD6D16CBp+0767,  0x0.AC1F03B509AAC0p+0715 },   // 1.0e+0231
        { 0x1.49C96CD6D16CBp+0768,  0x0.AC1F03B509AAC0p+0716 },   // 2.0e+0231
        { 0x1.EEAE23423A231p+0768,  0x0.822E858F8E8020p+0716 },   // 3.0e+0231
        { 0x1.49C96CD6D16CBp+0769,  0x0.AC1F03B509AAC0p+0717 },   // 4.0e+0231
        { 0x1.9C3BC80C85C7Ep+0769,  0x0.9726C4A24C1570p+0717 },   // 5.0e+0231
        { 0x1.EEAE23423A231p+0769,  0x0.822E858F8E8020p+0717 },   // 6.0e+0231
        { 0x1.20903F3BF73F2p+0770,  0x0.369B233E687568p+0718 },   // 7.0e+0231
        { 0x1.49C96CD6D16CBp+0770,  0x0.AC1F03B509AAC0p+0718 },   // 8.0e+0231
        { 0x1.73029A71AB9A5p+0770,  0x0.21A2E42BAAE018p+0718 },   // 9.0e+0231
        { 0x1.9C3BC80C85C7Ep+0770,  0x0.9726C4A24C1570p+0718 },   // 1.0e+0232
        { 0x1.9C3BC80C85C7Ep+0771,  0x0.9726C4A24C1570p+0719 },   // 2.0e+0232
        { 0x1.352CD6096455Ep+0772,  0x0.F15D1379B91010p+0720 },   // 3.0e+0232
        { 0x1.9C3BC80C85C7Ep+0772,  0x0.9726C4A24C1570p+0720 },   // 4.0e+0232
        { 0x1.01A55D07D39CFp+0773,  0x0.1E783AE56F8D68p+0721 },   // 5.0e+0232
        { 0x1.352CD6096455Ep+0773,  0x0.F15D1379B91010p+0721 },   // 6.0e+0232
        { 0x1.68B44F0AF50EEp+0773,  0x0.C441EC0E0292C0p+0721 },   // 7.0e+0232
        { 0x1.9C3BC80C85C7Ep+0773,  0x0.9726C4A24C1570p+0721 },   // 8.0e+0232
        { 0x1.CFC3410E1680Ep+0773,  0x0.6A0B9D36959820p+0721 },   // 9.0e+0232
        { 0x1.01A55D07D39CFp+0774,  0x0.1E783AE56F8D68p+0722 },   // 1.0e+0233
        { 0x1.01A55D07D39CFp+0775,  0x0.1E783AE56F8D68p+0723 },   // 2.0e+0233
        { 0x1.82780B8BBD6B6p+0775,  0x0.ADB45858275418p+0723 },   // 3.0e+0233
        { 0x1.01A55D07D39CFp+0776,  0x0.1E783AE56F8D68p+0724 },   // 4.0e+0233
        { 0x1.420EB449C8842p+0776,  0x0.E616499ECB70C0p+0724 },   // 5.0e+0233
        { 0x1.82780B8BBD6B6p+0776,  0x0.ADB45858275418p+0724 },   // 6.0e+0233
        { 0x1.C2E162CDB252Ap+0776,  0x0.75526711833770p+0724 },   // 7.0e+0233
        { 0x1.01A55D07D39CFp+0777,  0x0.1E783AE56F8D68p+0725 },   // 8.0e+0233
        { 0x1.21DA08A8CE109p+0777,  0x0.024742421D7F10p+0725 },   // 9.0e+0233
        { 0x1.420EB449C8842p+0777,  0x0.E616499ECB70C0p+0725 },   // 1.0e+0234
        { 0x1.420EB449C8842p+0778,  0x0.E616499ECB70C0p+0726 },   // 2.0e+0234
        { 0x1.E3160E6EACC64p+0778,  0x0.59216E6E312920p+0726 },   // 3.0e+0234
        { 0x1.420EB449C8842p+0779,  0x0.E616499ECB70C0p+0727 },   // 4.0e+0234
        { 0x1.9292615C3AA53p+0779,  0x0.9F9BDC067E4CF0p+0727 },   // 5.0e+0234
        { 0x1.E3160E6EACC64p+0779,  0x0.59216E6E312920p+0727 },   // 6.0e+0234
        { 0x1.19CCDDC08F73Ap+0780,  0x0.8953806AF202A8p+0728 },   // 7.0e+0234
        { 0x1.420EB449C8842p+0780,  0x0.E616499ECB70C0p+0728 },   // 8.0e+0234
        { 0x1.6A508AD30194Bp+0780,  0x0.42D912D2A4DED8p+0728 },   // 9.0e+0234
        { 0x1.9292615C3AA53p+0780,  0x0.9F9BDC067E4CF0p+0728 },   // 1.0e+0235
        { 0x1.9292615C3AA53p+0781,  0x0.9F9BDC067E4CF0p+0729 },   // 2.0e+0235
        { 0x1.2DEDC9052BFBEp+0782,  0x0.B7B4E504DEB9B0p+0730 },   // 3.0e+0235
        { 0x1.9292615C3AA53p+0782,  0x0.9F9BDC067E4CF0p+0730 },   // 4.0e+0235
        { 0x1.F736F9B3494E8p+0782,  0x0.8782D3081DE028p+0730 },   // 5.0e+0235
        { 0x1.2DEDC9052BFBEp+0783,  0x0.B7B4E504DEB9B0p+0731 },   // 6.0e+0235
        { 0x1.60401530B3509p+0783,  0x0.2BA86085AE8350p+0731 },   // 7.0e+0235
        { 0x1.9292615C3AA53p+0783,  0x0.9F9BDC067E4CF0p+0731 },   // 8.0e+0235
        { 0x1.C4E4AD87C1F9Ep+0783,  0x0.138F57874E1690p+0731 },   // 9.0e+0235
        { 0x1.F736F9B3494E8p+0783,  0x0.8782D3081DE028p+0731 },   // 1.0e+0236
        { 0x1.F736F9B3494E8p+0784,  0x0.8782D3081DE028p+0732 },   // 2.0e+0236
        { 0x1.79693B4676FAEp+0785,  0x0.65A21E46166820p+0733 },   // 3.0e+0236
        { 0x1.F736F9B3494E8p+0785,  0x0.8782D3081DE028p+0733 },   // 4.0e+0236
        { 0x1.3A825C100DD11p+0786,  0x0.54B1C3E512AC18p+0734 },   // 5.0e+0236
        { 0x1.79693B4676FAEp+0786,  0x0.65A21E46166820p+0734 },   // 6.0e+0236
        { 0x1.B8501A7CE024Bp+0786,  0x0.769278A71A2428p+0734 },   // 7.0e+0236
        { 0x1.F736F9B3494E8p+0786,  0x0.8782D3081DE028p+0734 },   // 8.0e+0236
        { 0x1.1B0EEC74D93C2p+0787,  0x0.CC3996B490CE18p+0735 },   // 9.0e+0236
        { 0x1.3A825C100DD11p+0787,  0x0.54B1C3E512AC18p+0735 },   // 1.0e+0237
        { 0x1.3A825C100DD11p+0788,  0x0.54B1C3E512AC18p+0736 },   // 2.0e+0237
        { 0x1.D7C38A1814B99p+0788,  0x0.FF0AA5D79C0228p+0736 },   // 3.0e+0237
        { 0x1.3A825C100DD11p+0789,  0x0.54B1C3E512AC18p+0737 },   // 4.0e+0237
        { 0x1.8922F31411455p+0789,  0x0.A9DE34DE575720p+0737 },   // 5.0e+0237
        { 0x1.D7C38A1814B99p+0789,  0x0.FF0AA5D79C0228p+0737 },   // 6.0e+0237
        { 0x1.1332108E0C16Fp+0790,  0x0.2A1B8B68705698p+0738 },   // 7.0e+0237
        { 0x1.3A825C100DD11p+0790,  0x0.54B1C3E512AC18p+0738 },   // 8.0e+0237
        { 0x1.61D2A7920F8B3p+0790,  0x0.7F47FC61B501A0p+0738 },   // 9.0e+0237
        { 0x1.8922F31411455p+0790,  0x0.A9DE34DE575720p+0738 },   // 1.0e+0238
        { 0x1.8922F31411455p+0791,  0x0.A9DE34DE575720p+0739 },   // 2.0e+0238
        { 0x1.26DA364F0CF40p+0792,  0x0.3F66A7A6C18158p+0740 },   // 3.0e+0238
        { 0x1.8922F31411455p+0792,  0x0.A9DE34DE575720p+0740 },   // 4.0e+0238
        { 0x1.EB6BAFD91596Bp+0792,  0x0.1455C215ED2CE8p+0740 },   // 5.0e+0238
        { 0x1.26DA364F0CF40p+0793,  0x0.3F66A7A6C18158p+0741 },   // 6.0e+0238
        { 0x1.57FE94B18F1CAp+0793,  0x0.F4A26E428C6C40p+0741 },   // 7.0e+0238
        { 0x1.8922F31411455p+0793,  0x0.A9DE34DE575720p+0741 },   // 8.0e+0238
        { 0x1.BA475176936E0p+0793,  0x0.5F19FB7A224208p+0741 },   // 9.0e+0238
        { 0x1.EB6BAFD91596Bp+0793,  0x0.1455C215ED2CE8p+0741 },   // 1.0e+0239
        { 0x1.EB6BAFD91596Bp+0794,  0x0.1455C215ED2CE8p+0742 },   // 2.0e+0239
        { 0x1.7090C3E2D0310p+0795,  0x0.4F40519071E1B0p+0743 },   // 3.0e+0239
        { 0x1.EB6BAFD91596Bp+0795,  0x0.1455C215ED2CE8p+0743 },   // 4.0e+0239
        { 0x1.33234DE7AD7E2p+0796,  0x0.ECB5994DB43C10p+0744 },   // 5.0e+0239
        { 0x1.7090C3E2D0310p+0796,  0x0.4F40519071E1B0p+0744 },   // 6.0e+0239
        { 0x1.ADFE39DDF2E3Dp+0796,  0x0.B1CB09D32F8750p+0744 },   // 7.0e+0239
        { 0x1.EB6BAFD91596Bp+0796,  0x0.1455C215ED2CE8p+0744 },   // 8.0e+0239
        { 0x1.146C92EA1C24Cp+0797,  0x0.3B703D2C556940p+0745 },   // 9.0e+0239
        { 0x1.33234DE7AD7E2p+0797,  0x0.ECB5994DB43C10p+0745 },   // 1.0e+0240
        { 0x1.33234DE7AD7E2p+0798,  0x0.ECB5994DB43C10p+0746 },   // 2.0e+0240
        { 0x1.CCB4F4DB843D4p+0798,  0x0.631065F48E5A18p+0746 },   // 3.0e+0240
        { 0x1.33234DE7AD7E2p+0799,  0x0.ECB5994DB43C10p+0747 },   // 4.0e+0240
        { 0x1.7FEC216198DDBp+0799,  0x0.A7E2FFA1214B18p+0747 },   // 5.0e+0240
        { 0x1.CCB4F4DB843D4p+0799,  0x0.631065F48E5A18p+0747 },   // 6.0e+0240
        { 0x1.0CBEE42AB7CE6p+0800,  0x0.8F1EE623FDB490p+0748 },   // 7.0e+0240
        { 0x1.33234DE7AD7E2p+0800,  0x0.ECB5994DB43C10p+0748 },   // 8.0e+0240
        { 0x1.5987B7A4A32DFp+0800,  0x0.4A4C4C776AC390p+0748 },   // 9.0e+0240
        { 0x1.7FEC216198DDBp+0800,  0x0.A7E2FFA1214B18p+0748 },   // 1.0e+0241
        { 0x1.7FEC216198DDBp+0801,  0x0.A7E2FFA1214B18p+0749 },   // 2.0e+0241
        { 0x1.1FF1190932A64p+0802,  0x0.BDEA3FB8D8F850p+0750 },   // 3.0e+0241
        { 0x1.7FEC216198DDBp+0802,  0x0.A7E2FFA1214B18p+0750 },   // 4.0e+0241
        { 0x1.DFE729B9FF152p+0802,  0x0.91DBBF89699DE0p+0750 },   // 5.0e+0241
        { 0x1.1FF1190932A64p+0803,  0x0.BDEA3FB8D8F850p+0751 },   // 6.0e+0241
        { 0x1.4FEE9D3565C20p+0803,  0x0.32E69FACFD21B0p+0751 },   // 7.0e+0241
        { 0x1.7FEC216198DDBp+0803,  0x0.A7E2FFA1214B18p+0751 },   // 8.0e+0241
        { 0x1.AFE9A58DCBF97p+0803,  0x0.1CDF5F95457478p+0751 },   // 9.0e+0241
        { 0x1.DFE729B9FF152p+0803,  0x0.91DBBF89699DE0p+0751 },   // 1.0e+0242
        { 0x1.DFE729B9FF152p+0804,  0x0.91DBBF89699DE0p+0752 },   // 2.0e+0242
        { 0x1.67ED5F4B7F4FDp+0805,  0x0.ED64CFA70F3668p+0753 },   // 3.0e+0242
        { 0x1.DFE729B9FF152p+0805,  0x0.91DBBF89699DE0p+0753 },   // 4.0e+0242
        { 0x1.2BF07A143F6D3p+0806,  0x0.9B2957B5E202A8p+0754 },   // 5.0e+0242
        { 0x1.67ED5F4B7F4FDp+0806,  0x0.ED64CFA70F3668p+0754 },   // 6.0e+0242
        { 0x1.A3EA4482BF328p+0806,  0x0.3FA047983C6A20p+0754 },   // 7.0e+0242
        { 0x1.DFE729B9FF152p+0806,  0x0.91DBBF89699DE0p+0754 },   // 8.0e+0242
        { 0x1.0DF207789F7BEp+0807,  0x0.720B9BBD4B68C8p+0755 },   // 9.0e+0242
        { 0x1.2BF07A143F6D3p+0807,  0x0.9B2957B5E202A8p+0755 },   // 1.0e+0243
        { 0x1.2BF07A143F6D3p+0808,  0x0.9B2957B5E202A8p+0756 },   // 2.0e+0243
        { 0x1.C1E8B71E5F23Dp+0808,  0x0.68BE0390D30400p+0756 },   // 3.0e+0243
        { 0x1.2BF07A143F6D3p+0809,  0x0.9B2957B5E202A8p+0757 },   // 4.0e+0243
        { 0x1.76EC98994F488p+0809,  0x0.81F3ADA35A8350p+0757 },   // 5.0e+0243
        { 0x1.C1E8B71E5F23Dp+0809,  0x0.68BE0390D30400p+0757 },   // 6.0e+0243
        { 0x1.06726AD1B77F9p+0810,  0x0.27C42CBF25C250p+0758 },   // 7.0e+0243
        { 0x1.2BF07A143F6D3p+0810,  0x0.9B2957B5E202A8p+0758 },   // 8.0e+0243
        { 0x1.516E8956C75AEp+0810,  0x0.0E8E82AC9E4300p+0758 },   // 9.0e+0243
        { 0x1.76EC98994F488p+0810,  0x0.81F3ADA35A8350p+0758 },   // 1.0e+0244
        { 0x1.76EC98994F488p+0811,  0x0.81F3ADA35A8350p+0759 },   // 2.0e+0244
        { 0x1.19317272FB766p+0812,  0x0.6176C23A83E280p+0760 },   // 3.0e+0244
        { 0x1.76EC98994F488p+0812,  0x0.81F3ADA35A8350p+0760 },   // 4.0e+0244
        { 0x1.D4A7BEBFA31AAp+0812,  0x0.A270990C312428p+0760 },   // 5.0e+0244
        { 0x1.19317272FB766p+0813,  0x0.6176C23A83E280p+0761 },   // 6.0e+0244
        { 0x1.480F0586255F7p+0813,  0x0.71B537EEEF32E8p+0761 },   // 7.0e+0244
        { 0x1.76EC98994F488p+0813,  0x0.81F3ADA35A8350p+0761 },   // 8.0e+0244
        { 0x1.A5CA2BAC79319p+0813,  0x0.92322357C5D3C0p+0761 },   // 9.0e+0244
        { 0x1.D4A7BEBFA31AAp+0813,  0x0.A270990C312428p+0761 },   // 1.0e+0245
        { 0x1.D4A7BEBFA31AAp+0814,  0x0.A270990C312428p+0762 },   // 2.0e+0245
        { 0x1.5F7DCF0FBA53Fp+0815,  0x0.F9D472C924DB20p+0763 },   // 3.0e+0245
        { 0x1.D4A7BEBFA31AAp+0815,  0x0.A270990C312428p+0763 },   // 4.0e+0245
        { 0x1.24E8D737C5F0Ap+0816,  0x0.A5865FA79EB698p+0764 },   // 5.0e+0245
        { 0x1.5F7DCF0FBA53Fp+0816,  0x0.F9D472C924DB20p+0764 },   // 6.0e+0245
        { 0x1.9A12C6E7AEB75p+0816,  0x0.4E2285EAAAFFA8p+0764 },   // 7.0e+0245
        { 0x1.D4A7BEBFA31AAp+0816,  0x0.A270990C312428p+0764 },   // 8.0e+0245
        { 0x1.079E5B4BCBBEFp+0817,  0x0.FB5F5616DBA458p+0765 },   // 9.0e+0245
        { 0x1.24E8D737C5F0Ap+0817,  0x0.A5865FA79EB698p+0765 },   // 1.0e+0246
        { 0x1.24E8D737C5F0Ap+0818,  0x0.A5865FA79EB698p+0766 },   // 2.0e+0246
        { 0x1.B75D42D3A8E8Fp+0818,  0x0.F8498F7B6E11E8p+0766 },   // 3.0e+0246
        { 0x1.24E8D737C5F0Ap+0819,  0x0.A5865FA79EB698p+0767 },   // 4.0e+0246
        { 0x1.6E230D05B76CDp+0819,  0x0.4EE7F791866440p+0767 },   // 5.0e+0246
        { 0x1.B75D42D3A8E8Fp+0819,  0x0.F8498F7B6E11E8p+0767 },   // 6.0e+0246
        { 0x1.004BBC50CD329p+0820,  0x0.50D593B2AADFC8p+0768 },   // 7.0e+0246
        { 0x1.24E8D737C5F0Ap+0820,  0x0.A5865FA79EB698p+0768 },   // 8.0e+0246
        { 0x1.4985F21EBEAEBp+0820,  0x0.FA372B9C928D70p+0768 },   // 9.0e+0246
        { 0x1.6E230D05B76CDp+0820,  0x0.4EE7F791866440p+0768 },   // 1.0e+0247
        { 0x1.6E230D05B76CDp+0821,  0x0.4EE7F791866440p+0769 },   // 2.0e+0247
        { 0x1.129A49C449919p+0822,  0x0.FB2DF9AD24CB30p+0770 },   // 3.0e+0247
        { 0x1.6E230D05B76CDp+0822,  0x0.4EE7F791866440p+0770 },   // 4.0e+0247
        { 0x1.C9ABD04725480p+0822,  0x0.A2A1F575E7FD50p+0770 },   // 5.0e+0247
        { 0x1.129A49C449919p+0823,  0x0.FB2DF9AD24CB30p+0771 },   // 6.0e+0247
        { 0x1.405EAB65007F3p+0823,  0x0.A50AF89F5597B8p+0771 },   // 7.0e+0247
        { 0x1.6E230D05B76CDp+0823,  0x0.4EE7F791866440p+0771 },   // 8.0e+0247
        { 0x1.9BE76EA66E5A6p+0823,  0x0.F8C4F683B730C8p+0771 },   // 9.0e+0247
        { 0x1.C9ABD04725480p+0823,  0x0.A2A1F575E7FD50p+0771 },   // 1.0e+0248
        { 0x1.C9ABD04725480p+0824,  0x0.A2A1F575E7FD50p+0772 },   // 2.0e+0248
        { 0x1.5740DC355BF60p+0825,  0x0.79F978186DFDF8p+0773 },   // 3.0e+0248
        { 0x1.C9ABD04725480p+0825,  0x0.A2A1F575E7FD50p+0773 },   // 4.0e+0248
        { 0x1.1E0B622C774D0p+0826,  0x0.65A53969B0FE50p+0774 },   // 5.0e+0248
        { 0x1.5740DC355BF60p+0826,  0x0.79F978186DFDF8p+0774 },   // 6.0e+0248
        { 0x1.9076563E409F0p+0826,  0x0.8E4DB6C72AFDA8p+0774 },   // 7.0e+0248
        { 0x1.C9ABD04725480p+0826,  0x0.A2A1F575E7FD50p+0774 },   // 8.0e+0248
        { 0x1.0170A52804F88p+0827,  0x0.5B7B1A12527E78p+0775 },   // 9.0e+0248
        { 0x1.1E0B622C774D0p+0827,  0x0.65A53969B0FE50p+0775 },   // 1.0e+0249
        { 0x1.1E0B622C774D0p+0828,  0x0.65A53969B0FE50p+0776 },   // 2.0e+0249
        { 0x1.AD111342B2F38p+0828,  0x0.9877D61E897D78p+0776 },   // 3.0e+0249
        { 0x1.1E0B622C774D0p+0829,  0x0.65A53969B0FE50p+0777 },   // 4.0e+0249
        { 0x1.658E3AB795204p+0829,  0x0.7F0E87C41D3DE8p+0777 },   // 5.0e+0249
        { 0x1.AD111342B2F38p+0829,  0x0.9877D61E897D78p+0777 },   // 6.0e+0249
        { 0x1.F493EBCDD0C6Cp+0829,  0x0.B1E12478F5BD10p+0777 },   // 7.0e+0249
        { 0x1.1E0B622C774D0p+0830,  0x0.65A53969B0FE50p+0778 },   // 8.0e+0249
        { 0x1.41CCCE720636Ap+0830,  0x0.7259E096E71E18p+0778 },   // 9.0e+0249
        { 0x1.658E3AB795204p+0830,  0x0.7F0E87C41D3DE8p+0778 },   // 1.0e+0250
        { 0x1.658E3AB795204p+0831,  0x0.7F0E87C41D3DE8p+0779 },   // 2.0e+0250
        { 0x1.0C2AAC09AFD83p+0832,  0x0.5F4AE5D315EE68p+0780 },   // 3.0e+0250
        { 0x1.658E3AB795204p+0832,  0x0.7F0E87C41D3DE8p+0780 },   // 4.0e+0250
        { 0x1.BEF1C9657A685p+0832,  0x0.9ED229B5248D60p+0780 },   // 5.0e+0250
        { 0x1.0C2AAC09AFD83p+0833,  0x0.5F4AE5D315EE68p+0781 },   // 6.0e+0250
        { 0x1.38DC7360A27C3p+0833,  0x0.EF2CB6CB999628p+0781 },   // 7.0e+0250
        { 0x1.658E3AB795204p+0833,  0x0.7F0E87C41D3DE8p+0781 },   // 8.0e+0250
        { 0x1.9240020E87C45p+0833,  0x0.0EF058BCA0E5A0p+0781 },   // 9.0e+0250
        { 0x1.BEF1C9657A685p+0833,  0x0.9ED229B5248D60p+0781 },   // 1.0e+0251
        { 0x1.BEF1C9657A685p+0834,  0x0.9ED229B5248D60p+0782 },   // 2.0e+0251
        { 0x1.4F35570C1BCE4p+0835,  0x0.371D9F47DB6A08p+0783 },   // 3.0e+0251
        { 0x1.BEF1C9657A685p+0835,  0x0.9ED229B5248D60p+0783 },   // 4.0e+0251
        { 0x1.17571DDF6C813p+0836,  0x0.83435A1136D858p+0784 },   // 5.0e+0251
        { 0x1.4F35570C1BCE4p+0836,  0x0.371D9F47DB6A08p+0784 },   // 6.0e+0251
        { 0x1.87139038CB1B4p+0836,  0x0.EAF7E47E7FFBB8p+0784 },   // 7.0e+0251
        { 0x1.BEF1C9657A685p+0836,  0x0.9ED229B5248D60p+0784 },   // 8.0e+0251
        { 0x1.F6D0029229B56p+0836,  0x0.52AC6EEBC91F10p+0784 },   // 9.0e+0251
        { 0x1.17571DDF6C813p+0837,  0x0.83435A1136D858p+0785 },   // 1.0e+0252
        { 0x1.17571DDF6C813p+0838,  0x0.83435A1136D858p+0786 },   // 2.0e+0252
        { 0x1.A302ACCF22C1Dp+0838,  0x0.44E50719D24488p+0786 },   // 3.0e+0252
        { 0x1.17571DDF6C813p+0839,  0x0.83435A1136D858p+0787 },   // 4.0e+0252
        { 0x1.5D2CE55747A18p+0839,  0x0.64143095848E70p+0787 },   // 5.0e+0252
        { 0x1.A302ACCF22C1Dp+0839,  0x0.44E50719D24488p+0787 },   // 6.0e+0252
        { 0x1.E8D87446FDE22p+0839,  0x0.25B5DD9E1FFAA0p+0787 },   // 7.0e+0252
        { 0x1.17571DDF6C813p+0840,  0x0.83435A1136D858p+0788 },   // 8.0e+0252
        { 0x1.3A42019B5A115p+0840,  0x0.F3ABC5535DB368p+0788 },   // 9.0e+0252
        { 0x1.5D2CE55747A18p+0840,  0x0.64143095848E70p+0788 },   // 1.0e+0253
        { 0x1.5D2CE55747A18p+0841,  0x0.64143095848E70p+0789 },   // 2.0e+0253
        { 0x1.05E1AC0175B92p+0842,  0x0.4B0F2470236AD8p+0790 },   // 3.0e+0253
        { 0x1.5D2CE55747A18p+0842,  0x0.64143095848E70p+0790 },   // 4.0e+0253
        { 0x1.B4781EAD1989Ep+0842,  0x0.7D193CBAE5B210p+0790 },   // 5.0e+0253
        { 0x1.05E1AC0175B92p+0843,  0x0.4B0F2470236AD8p+0791 },   // 6.0e+0253
        { 0x1.318748AC5EAD5p+0843,  0x0.5791AA82D3FCA0p+0791 },   // 7.0e+0253
        { 0x1.5D2CE55747A18p+0843,  0x0.64143095848E70p+0791 },   // 8.0e+0253
        { 0x1.88D282023095Bp+0843,  0x0.7096B6A8352040p+0791 },   // 9.0e+0253
        { 0x1.B4781EAD1989Ep+0843,  0x0.7D193CBAE5B210p+0791 },   // 1.0e+0254
        { 0x1.B4781EAD1989Ep+0844,  0x0.7D193CBAE5B210p+0792 },   // 2.0e+0254
        { 0x1.475A1701D3276p+0845,  0x0.DDD2ED8C2C4588p+0793 },   // 3.0e+0254
        { 0x1.B4781EAD1989Ep+0845,  0x0.7D193CBAE5B210p+0793 },   // 4.0e+0254
        { 0x1.10CB132C2FF63p+0846,  0x0.0E2FC5F4CF8F48p+0794 },   // 5.0e+0254
        { 0x1.475A1701D3276p+0846,  0x0.DDD2ED8C2C4588p+0794 },   // 6.0e+0254
        { 0x1.7DE91AD77658Ap+0846,  0x0.AD76152388FBD0p+0794 },   // 7.0e+0254
        { 0x1.B4781EAD1989Ep+0846,  0x0.7D193CBAE5B210p+0794 },   // 8.0e+0254
        { 0x1.EB072282BCBB2p+0846,  0x0.4CBC6452426850p+0794 },   // 9.0e+0254
        { 0x1.10CB132C2FF63p+0847,  0x0.0E2FC5F4CF8F48p+0795 },   // 1.0e+0255
        { 0x1.10CB132C2FF63p+0848,  0x0.0E2FC5F4CF8F48p+0796 },   // 2.0e+0255
        { 0x1.99309CC247F14p+0848,  0x0.9547A8EF3756F0p+0796 },   // 3.0e+0255
        { 0x1.10CB132C2FF63p+0849,  0x0.0E2FC5F4CF8F48p+0797 },   // 4.0e+0255
        { 0x1.54FDD7F73BF3Bp+0849,  0x0.D1BBB772037318p+0797 },   // 5.0e+0255
        { 0x1.99309CC247F14p+0849,  0x0.9547A8EF3756F0p+0797 },   // 6.0e+0255
        { 0x1.DD63618D53EEDp+0849,  0x0.58D39A6C6B3AC0p+0797 },   // 7.0e+0255
        { 0x1.10CB132C2FF63p+0850,  0x0.0E2FC5F4CF8F48p+0798 },   // 8.0e+0255
        { 0x1.32E47591B5F4Fp+0850,  0x0.6FF5BEB3698130p+0798 },   // 9.0e+0255
        { 0x1.54FDD7F73BF3Bp+0850,  0x0.D1BBB772037318p+0798 },   // 1.0e+0256
        { 0x1.54FDD7F73BF3Bp+0851,  0x0.D1BBB772037318p+0799 },   // 2.0e+0256
        { 0x1.FF7CC3F2D9ED9p+0851,  0x0.BA99932B052CA8p+0799 },   // 3.0e+0256
        { 0x1.54FDD7F73BF3Bp+0852,  0x0.D1BBB772037318p+0800 },   // 4.0e+0256
        { 0x1.AA3D4DF50AF0Ap+0852,  0x0.C62AA54E844FE0p+0800 },   // 5.0e+0256
        { 0x1.FF7CC3F2D9ED9p+0852,  0x0.BA99932B052CA8p+0800 },   // 6.0e+0256
        { 0x1.2A5E1CF854754p+0853,  0x0.57844083C304B8p+0801 },   // 7.0e+0256
        { 0x1.54FDD7F73BF3Bp+0853,  0x0.D1BBB772037318p+0801 },   // 8.0e+0256
        { 0x1.7F9D92F623723p+0853,  0x0.4BF32E6043E180p+0801 },   // 9.0e+0256
        { 0x1.AA3D4DF50AF0Ap+0853,  0x0.C62AA54E844FE0p+0801 },   // 1.0e+0257
        { 0x1.AA3D4DF50AF0Ap+0854,  0x0.C62AA54E844FE0p+0802 },   // 2.0e+0257
        { 0x1.3FADFA77C8348p+0855,  0x0.149FFBFAE33BE8p+0803 },   // 3.0e+0257
        { 0x1.AA3D4DF50AF0Ap+0855,  0x0.C62AA54E844FE0p+0803 },   // 4.0e+0257
        { 0x1.0A6650B926D66p+0856,  0x0.BBDAA75112B1F0p+0804 },   // 5.0e+0257
        { 0x1.3FADFA77C8348p+0856,  0x0.149FFBFAE33BE8p+0804 },   // 6.0e+0257
        { 0x1.74F5A43669929p+0856,  0x0.6D6550A4B3C5E8p+0804 },   // 7.0e+0257
        { 0x1.AA3D4DF50AF0Ap+0856,  0x0.C62AA54E844FE0p+0804 },   // 8.0e+0257
        { 0x1.DF84F7B3AC4ECp+0856,  0x0.1EEFF9F854D9E0p+0804 },   // 9.0e+0257
        { 0x1.0A6650B926D66p+0857,  0x0.BBDAA75112B1F0p+0805 },   // 1.0e+0258
        { 0x1.0A6650B926D66p+0858,  0x0.BBDAA75112B1F0p+0806 },   // 2.0e+0258
        { 0x1.8F997915BA41Ap+0858,  0x0.19C7FAF99C0AE8p+0806 },   // 3.0e+0258
        { 0x1.0A6650B926D66p+0859,  0x0.BBDAA75112B1F0p+0807 },   // 4.0e+0258
        { 0x1.4CFFE4E7708C0p+0859,  0x0.6AD15125575E68p+0807 },   // 5.0e+0258
        { 0x1.8F997915BA41Ap+0859,  0x0.19C7FAF99C0AE8p+0807 },   // 6.0e+0258
        { 0x1.D2330D4403F73p+0859,  0x0.C8BEA4CDE0B760p+0807 },   // 7.0e+0258
        { 0x1.0A6650B926D66p+0860,  0x0.BBDAA75112B1F0p+0808 },   // 8.0e+0258
        { 0x1.2BB31AD04BB13p+0860,  0x0.9355FC3B350828p+0808 },   // 9.0e+0258
        { 0x1.4CFFE4E7708C0p+0860,  0x0.6AD15125575E68p+0808 },   // 1.0e+0259
        { 0x1.4CFFE4E7708C0p+0861,  0x0.6AD15125575E68p+0809 },   // 2.0e+0259
        { 0x1.F37FD75B28D20p+0861,  0x0.A039F9B8030DA0p+0809 },   // 3.0e+0259
        { 0x1.4CFFE4E7708C0p+0862,  0x0.6AD15125575E68p+0810 },   // 4.0e+0259
        { 0x1.A03FDE214CAF0p+0862,  0x0.8585A56EAD3608p+0810 },   // 5.0e+0259
        { 0x1.F37FD75B28D20p+0862,  0x0.A039F9B8030DA0p+0810 },   // 6.0e+0259
        { 0x1.235FE84A827A8p+0863,  0x0.5D772700AC7298p+0811 },   // 7.0e+0259
        { 0x1.4CFFE4E7708C0p+0863,  0x0.6AD15125575E68p+0811 },   // 8.0e+0259
        { 0x1.769FE1845E9D8p+0863,  0x0.782B7B4A024A38p+0811 },   // 9.0e+0259
        { 0x1.A03FDE214CAF0p+0863,  0x0.8585A56EAD3608p+0811 },   // 1.0e+0260
        { 0x1.A03FDE214CAF0p+0864,  0x0.8585A56EAD3608p+0812 },   // 2.0e+0260
        { 0x1.382FE698F9834p+0865,  0x0.64243C1301E880p+0813 },   // 3.0e+0260
        { 0x1.A03FDE214CAF0p+0865,  0x0.8585A56EAD3608p+0813 },   // 4.0e+0260
        { 0x1.0427EAD4CFED6p+0866,  0x0.537387652C41C0p+0814 },   // 5.0e+0260
        { 0x1.382FE698F9834p+0866,  0x0.64243C1301E880p+0814 },   // 6.0e+0260
        { 0x1.6C37E25D23192p+0866,  0x0.74D4F0C0D78F40p+0814 },   // 7.0e+0260
        { 0x1.A03FDE214CAF0p+0866,  0x0.8585A56EAD3608p+0814 },   // 8.0e+0260
        { 0x1.D447D9E57644Ep+0866,  0x0.96365A1C82DCC8p+0814 },   // 9.0e+0260
        { 0x1.0427EAD4CFED6p+0867,  0x0.537387652C41C0p+0815 },   // 1.0e+0261
        { 0x1.0427EAD4CFED6p+0868,  0x0.537387652C41C0p+0816 },   // 2.0e+0261
        { 0x1.863BE03F37E41p+0868,  0x0.7D2D4B17C262A0p+0816 },   // 3.0e+0261
        { 0x1.0427EAD4CFED6p+0869,  0x0.537387652C41C0p+0817 },   // 4.0e+0261
        { 0x1.4531E58A03E8Bp+0869,  0x0.E850693E775230p+0817 },   // 5.0e+0261
        { 0x1.863BE03F37E41p+0869,  0x0.7D2D4B17C262A0p+0817 },   // 6.0e+0261
        { 0x1.C745DAF46BDF7p+0869,  0x0.120A2CF10D7318p+0817 },   // 7.0e+0261
        { 0x1.0427EAD4CFED6p+0870,  0x0.537387652C41C0p+0818 },   // 8.0e+0261
        { 0x1.24ACE82F69EB1p+0870,  0x0.1DE1F851D1C9F8p+0818 },   // 9.0e+0261
        { 0x1.4531E58A03E8Bp+0870,  0x0.E850693E775230p+0818 },   // 1.0e+0262
        { 0x1.4531E58A03E8Bp+0871,  0x0.E850693E775230p+0819 },   // 2.0e+0262
        { 0x1.E7CAD84F05DD1p+0871,  0x0.DC789DDDB2FB50p+0819 },   // 3.0e+0262
        { 0x1.4531E58A03E8Bp+0872,  0x0.E850693E775230p+0820 },   // 4.0e+0262
        { 0x1.967E5EEC84E2Ep+0872,  0x0.E264838E1526C0p+0820 },   // 5.0e+0262
        { 0x1.E7CAD84F05DD1p+0872,  0x0.DC789DDDB2FB50p+0820 },   // 6.0e+0262
        { 0x1.1C8BA8D8C36BAp+0873,  0x0.6B465C16A867E8p+0821 },   // 7.0e+0262
        { 0x1.4531E58A03E8Bp+0873,  0x0.E850693E775230p+0821 },   // 8.0e+0262
        { 0x1.6DD8223B4465Dp+0873,  0x0.655A7666463C78p+0821 },   // 9.0e+0262
        { 0x1.967E5EEC84E2Ep+0873,  0x0.E264838E1526C0p+0821 },   // 1.0e+0263
        { 0x1.967E5EEC84E2Ep+0874,  0x0.E264838E1526C0p+0822 },   // 2.0e+0263
        { 0x1.30DEC73163AA3p+0875,  0x0.29CB62AA8FDD10p+0823 },   // 3.0e+0263
        { 0x1.967E5EEC84E2Ep+0875,  0x0.E264838E1526C0p+0823 },   // 4.0e+0263
        { 0x1.FC1DF6A7A61BAp+0875,  0x0.9AFDA4719A7070p+0823 },   // 5.0e+0263
        { 0x1.30DEC73163AA3p+0876,  0x0.29CB62AA8FDD10p+0824 },   // 6.0e+0263
        { 0x1.63AE930EF4469p+0876,  0x0.0617F31C5281E8p+0824 },   // 7.0e+0263
        { 0x1.967E5EEC84E2Ep+0876,  0x0.E264838E1526C0p+0824 },   // 8.0e+0263
        { 0x1.C94E2ACA157F4p+0876,  0x0.BEB113FFD7CB98p+0824 },   // 9.0e+0263
        { 0x1.FC1DF6A7A61BAp+0876,  0x0.9AFDA4719A7070p+0824 },   // 1.0e+0264
        { 0x1.FC1DF6A7A61BAp+0877,  0x0.9AFDA4719A7070p+0825 },   // 2.0e+0264
        { 0x1.7D1678FDBC94Bp+0878,  0x0.F43E3B5533D450p+0826 },   // 3.0e+0264
        { 0x1.FC1DF6A7A61BAp+0878,  0x0.9AFDA4719A7070p+0826 },   // 4.0e+0264
        { 0x1.3D92BA28C7D14p+0879,  0x0.A0DE86C7008648p+0827 },   // 5.0e+0264
        { 0x1.7D1678FDBC94Bp+0879,  0x0.F43E3B5533D450p+0827 },   // 6.0e+0264
        { 0x1.BC9A37D2B1583p+0879,  0x0.479DEFE3672260p+0827 },   // 7.0e+0264
        { 0x1.FC1DF6A7A61BAp+0879,  0x0.9AFDA4719A7070p+0827 },   // 8.0e+0264
        { 0x1.1DD0DABE4D6F8p+0880,  0x0.F72EAC7FE6DF40p+0828 },   // 9.0e+0264
        { 0x1.3D92BA28C7D14p+0880,  0x0.A0DE86C7008648p+0828 },   // 1.0e+0265
        { 0x1.3D92BA28C7D14p+0881,  0x0.A0DE86C7008648p+0829 },   // 2.0e+0265
        { 0x1.DC5C173D2BB9Ep+0881,  0x0.F14DCA2A80C968p+0829 },   // 3.0e+0265
        { 0x1.3D92BA28C7D14p+0882,  0x0.A0DE86C7008648p+0830 },   // 4.0e+0265
        { 0x1.8CF768B2F9C59p+0882,  0x0.C9162878C0A7D8p+0830 },   // 5.0e+0265
        { 0x1.DC5C173D2BB9Ep+0882,  0x0.F14DCA2A80C968p+0830 },   // 6.0e+0265
        { 0x1.15E062E3AED72p+0883,  0x0.0CC2B5EE207580p+0831 },   // 7.0e+0265
        { 0x1.3D92BA28C7D14p+0883,  0x0.A0DE86C7008648p+0831 },   // 8.0e+0265
        { 0x1.6545116DE0CB7p+0883,  0x0.34FA579FE09710p+0831 },   // 9.0e+0265
        { 0x1.8CF768B2F9C59p+0883,  0x0.C9162878C0A7D8p+0831 },   // 1.0e+0266
        { 0x1.8CF768B2F9C59p+0884,  0x0.C9162878C0A7D8p+0832 },   // 2.0e+0266
        { 0x1.29B98E863B543p+0885,  0x0.56D09E5A907DE0p+0833 },   // 3.0e+0266
        { 0x1.8CF768B2F9C59p+0885,  0x0.C9162878C0A7D8p+0833 },   // 4.0e+0266
        { 0x1.F03542DFB8370p+0885,  0x0.3B5BB296F0D1D0p+0833 },   // 5.0e+0266
        { 0x1.29B98E863B543p+0886,  0x0.56D09E5A907DE0p+0834 },   // 6.0e+0266
        { 0x1.5B587B9C9A8CEp+0886,  0x0.8FF36369A892E0p+0834 },   // 7.0e+0266
        { 0x1.8CF768B2F9C59p+0886,  0x0.C9162878C0A7D8p+0834 },   // 8.0e+0266
        { 0x1.BE9655C958FE5p+0886,  0x0.0238ED87D8BCD0p+0834 },   // 9.0e+0266
        { 0x1.F03542DFB8370p+0886,  0x0.3B5BB296F0D1D0p+0834 },   // 1.0e+0267
        { 0x1.F03542DFB8370p+0887,  0x0.3B5BB296F0D1D0p+0835 },   // 2.0e+0267
        { 0x1.7427F227CA294p+0888,  0x0.2C84C5F1349D58p+0836 },   // 3.0e+0267
        { 0x1.F03542DFB8370p+0888,  0x0.3B5BB296F0D1D0p+0836 },   // 4.0e+0267
        { 0x1.362149CBD3226p+0889,  0x0.25194F9E568320p+0837 },   // 5.0e+0267
        { 0x1.7427F227CA294p+0889,  0x0.2C84C5F1349D58p+0837 },   // 6.0e+0267
        { 0x1.B22E9A83C1302p+0889,  0x0.33F03C4412B798p+0837 },   // 7.0e+0267
        { 0x1.F03542DFB8370p+0889,  0x0.3B5BB296F0D1D0p+0837 },   // 8.0e+0267
        { 0x1.171DF59DD79EFp+0890,  0x0.21639474E77600p+0838 },   // 9.0e+0267
        { 0x1.362149CBD3226p+0890,  0x0.25194F9E568320p+0838 },   // 1.0e+0268
        { 0x1.362149CBD3226p+0891,  0x0.25194F9E568320p+0839 },   // 2.0e+0268
        { 0x1.D131EEB1BCB39p+0891,  0x0.37A5F76D81C4B0p+0839 },   // 3.0e+0268
        { 0x1.362149CBD3226p+0892,  0x0.25194F9E568320p+0840 },   // 4.0e+0268
        { 0x1.83A99C3EC7EAFp+0892,  0x0.AE5FA385EC23E8p+0840 },   // 5.0e+0268
        { 0x1.D131EEB1BCB39p+0892,  0x0.37A5F76D81C4B0p+0840 },   // 6.0e+0268
        { 0x1.0F5D209258BE1p+0893,  0x0.607625AA8BB2B8p+0841 },   // 7.0e+0268
        { 0x1.362149CBD3226p+0893,  0x0.25194F9E568320p+0841 },   // 8.0e+0268
        { 0x1.5CE573054D86Ap+0893,  0x0.E9BC7992215388p+0841 },   // 9.0e+0268
        { 0x1.83A99C3EC7EAFp+0893,  0x0.AE5FA385EC23E8p+0841 },   // 1.0e+0269
        { 0x1.83A99C3EC7EAFp+0894,  0x0.AE5FA385EC23E8p+0842 },   // 2.0e+0269
        { 0x1.22BF352F15F03p+0895,  0x0.C2C7BAA4711AF0p+0843 },   // 3.0e+0269
        { 0x1.83A99C3EC7EAFp+0895,  0x0.AE5FA385EC23E8p+0843 },   // 4.0e+0269
        { 0x1.E494034E79E5Bp+0895,  0x0.99F78C67672CE0p+0843 },   // 5.0e+0269
        { 0x1.22BF352F15F03p+0896,  0x0.C2C7BAA4711AF0p+0844 },   // 6.0e+0269
        { 0x1.533468B6EEED9p+0896,  0x0.B893AF152E9F68p+0844 },   // 7.0e+0269
        { 0x1.83A99C3EC7EAFp+0896,  0x0.AE5FA385EC23E8p+0844 },   // 8.0e+0269
        { 0x1.B41ECFC6A0E85p+0896,  0x0.A42B97F6A9A868p+0844 },   // 9.0e+0269
        { 0x1.E494034E79E5Bp+0896,  0x0.99F78C67672CE0p+0844 },   // 1.0e+0270
        { 0x1.E494034E79E5Bp+0897,  0x0.99F78C67672CE0p+0845 },   // 2.0e+0270
        { 0x1.6B6F027ADB6C4p+0898,  0x0.B379A94D8D61A8p+0846 },   // 3.0e+0270
        { 0x1.E494034E79E5Bp+0898,  0x0.99F78C67672CE0p+0846 },   // 4.0e+0270
        { 0x1.2EDC82110C2F9p+0899,  0x0.403AB7C0A07C10p+0847 },   // 5.0e+0270
        { 0x1.6B6F027ADB6C4p+0899,  0x0.B379A94D8D61A8p+0847 },   // 6.0e+0270
        { 0x1.A80182E4AAA90p+0899,  0x0.26B89ADA7A4748p+0847 },   // 7.0e+0270
        { 0x1.E494034E79E5Bp+0899,  0x0.99F78C67672CE0p+0847 },   // 8.0e+0270
        { 0x1.109341DC24913p+0900,  0x0.869B3EFA2A0940p+0848 },   // 9.0e+0270
        { 0x1.2EDC82110C2F9p+0900,  0x0.403AB7C0A07C10p+0848 },   // 1.0e+0271
        { 0x1.2EDC82110C2F9p+0901,  0x0.403AB7C0A07C10p+0849 },   // 2.0e+0271
        { 0x1.C64AC31992475p+0901,  0x0.E05813A0F0BA18p+0849 },   // 3.0e+0271
        { 0x1.2EDC82110C2F9p+0902,  0x0.403AB7C0A07C10p+0850 },   // 4.0e+0271
        { 0x1.7A93A2954F3B7p+0902,  0x0.904965B0C89B10p+0850 },   // 5.0e+0271
        { 0x1.C64AC31992475p+0902,  0x0.E05813A0F0BA18p+0850 },   // 6.0e+0271
        { 0x1.0900F1CEEAA9Ap+0903,  0x0.183360C88C6C88p+0851 },   // 7.0e+0271
        { 0x1.2EDC82110C2F9p+0903,  0x0.403AB7C0A07C10p+0851 },   // 8.0e+0271
        { 0x1.54B812532DB58p+0903,  0x0.68420EB8B48B90p+0851 },   // 9.0e+0271
        { 0x1.7A93A2954F3B7p+0903,  0x0.904965B0C89B10p+0851 },   // 1.0e+0272
        { 0x1.7A93A2954F3B7p+0904,  0x0.904965B0C89B10p+0852 },   // 2.0e+0272
        { 0x1.1BEEB9EFFB6C9p+0905,  0x0.AC370C44967448p+0853 },   // 3.0e+0272
        { 0x1.7A93A2954F3B7p+0905,  0x0.904965B0C89B10p+0853 },   // 4.0e+0272
        { 0x1.D9388B3AA30A5p+0905,  0x0.745BBF1CFAC1D8p+0853 },   // 5.0e+0272
        { 0x1.1BEEB9EFFB6C9p+0906,  0x0.AC370C44967448p+0854 },   // 6.0e+0272
        { 0x1.4B412E42A5540p+0906,  0x0.9E4038FAAF87B0p+0854 },   // 7.0e+0272
        { 0x1.7A93A2954F3B7p+0906,  0x0.904965B0C89B10p+0854 },   // 8.0e+0272
        { 0x1.A9E616E7F922Ep+0906,  0x0.82529266E1AE70p+0854 },   // 9.0e+0272
        { 0x1.D9388B3AA30A5p+0906,  0x0.745BBF1CFAC1D8p+0854 },   // 1.0e+0273
        { 0x1.D9388B3AA30A5p+0907,  0x0.745BBF1CFAC1D8p+0855 },   // 2.0e+0273
        { 0x1.62EA686BFA47Cp+0908,  0x0.1744CF55BC1160p+0856 },   // 3.0e+0273
        { 0x1.D9388B3AA30A5p+0908,  0x0.745BBF1CFAC1D8p+0856 },   // 4.0e+0273
        { 0x1.27C35704A5E67p+0909,  0x0.68B957721CB928p+0857 },   // 5.0e+0273
        { 0x1.62EA686BFA47Cp+0909,  0x0.1744CF55BC1160p+0857 },   // 6.0e+0273
        { 0x1.9E1179D34EA90p+0909,  0x0.C5D047395B6998p+0857 },   // 7.0e+0273
        { 0x1.D9388B3AA30A5p+0909,  0x0.745BBF1CFAC1D8p+0857 },   // 8.0e+0273
        { 0x1.0A2FCE50FBB5Dp+0910,  0x0.11739B804D0D08p+0858 },   // 9.0e+0273
        { 0x1.27C35704A5E67p+0910,  0x0.68B957721CB928p+0858 },   // 1.0e+0274
        { 0x1.27C35704A5E67p+0911,  0x0.68B957721CB928p+0859 },   // 2.0e+0274
        { 0x1.BBA50286F8D9Bp+0911,  0x0.1D16032B2B15B8p+0859 },   // 3.0e+0274
        { 0x1.27C35704A5E67p+0912,  0x0.68B957721CB928p+0860 },   // 4.0e+0274
        { 0x1.71B42CC5CF601p+0912,  0x0.42E7AD4EA3E770p+0860 },   // 5.0e+0274
        { 0x1.BBA50286F8D9Bp+0912,  0x0.1D16032B2B15B8p+0860 },   // 6.0e+0274
        { 0x1.02CAEC241129Ap+0913,  0x0.7BA22C83D92200p+0861 },   // 7.0e+0274
        { 0x1.27C35704A5E67p+0913,  0x0.68B957721CB928p+0861 },   // 8.0e+0274
        { 0x1.4CBBC1E53AA34p+0913,  0x0.55D08260605048p+0861 },   // 9.0e+0274
        { 0x1.71B42CC5CF601p+0913,  0x0.42E7AD4EA3E770p+0861 },   // 1.0e+0275
        { 0x1.71B42CC5CF601p+0914,  0x0.42E7AD4EA3E770p+0862 },   // 2.0e+0275
        { 0x1.154721945B880p+0915,  0x0.F22DC1FAFAED90p+0863 },   // 3.0e+0275
        { 0x1.71B42CC5CF601p+0915,  0x0.42E7AD4EA3E770p+0863 },   // 4.0e+0275
        { 0x1.CE2137F743381p+0915,  0x0.93A198A24CE148p+0863 },   // 5.0e+0275
        { 0x1.154721945B880p+0916,  0x0.F22DC1FAFAED90p+0864 },   // 6.0e+0275
        { 0x1.437DA72D15741p+0916,  0x0.1A8AB7A4CF6A80p+0864 },   // 7.0e+0275
        { 0x1.71B42CC5CF601p+0916,  0x0.42E7AD4EA3E770p+0864 },   // 8.0e+0275
        { 0x1.9FEAB25E894C1p+0916,  0x0.6B44A2F8786460p+0864 },   // 9.0e+0275
        { 0x1.CE2137F743381p+0916,  0x0.93A198A24CE148p+0864 },   // 1.0e+0276
        { 0x1.CE2137F743381p+0917,  0x0.93A198A24CE148p+0865 },   // 2.0e+0276
        { 0x1.5A98E9F9726A1p+0918,  0x0.2EB93279B9A8F8p+0866 },   // 3.0e+0276
        { 0x1.CE2137F743381p+0918,  0x0.93A198A24CE148p+0866 },   // 4.0e+0276
        { 0x1.20D4C2FA8A030p+0919,  0x0.FC44FF65700CD0p+0867 },   // 5.0e+0276
        { 0x1.5A98E9F9726A1p+0919,  0x0.2EB93279B9A8F8p+0867 },   // 6.0e+0276
        { 0x1.945D10F85AD11p+0919,  0x0.612D658E034520p+0867 },   // 7.0e+0276
        { 0x1.CE2137F743381p+0919,  0x0.93A198A24CE148p+0867 },   // 8.0e+0276
        { 0x1.03F2AF7B15CF8p+0920,  0x0.E30AE5DB4B3EB8p+0868 },   // 9.0e+0276
        { 0x1.20D4C2FA8A030p+0920,  0x0.FC44FF65700CD0p+0868 },   // 1.0e+0277
        { 0x1.20D4C2FA8A030p+0921,  0x0.FC44FF65700CD0p+0869 },   // 2.0e+0277
        { 0x1.B13F2477CF049p+0921,  0x0.7A677F18281338p+0869 },   // 3.0e+0277
        { 0x1.20D4C2FA8A030p+0922,  0x0.FC44FF65700CD0p+0870 },   // 4.0e+0277
        { 0x1.6909F3B92C83Dp+0922,  0x0.3B563F3ECC1000p+0870 },   // 5.0e+0277
        { 0x1.B13F2477CF049p+0922,  0x0.7A677F18281338p+0870 },   // 6.0e+0277
        { 0x1.F974553671855p+0922,  0x0.B978BEF1841668p+0870 },   // 7.0e+0277
        { 0x1.20D4C2FA8A030p+0923,  0x0.FC44FF65700CD0p+0871 },   // 8.0e+0277
        { 0x1.44EF5B59DB437p+0923,  0x0.1BCD9F521E0E68p+0871 },   // 9.0e+0277
        { 0x1.6909F3B92C83Dp+0923,  0x0.3B563F3ECC1000p+0871 },   // 1.0e+0278
        { 0x1.6909F3B92C83Dp+0924,  0x0.3B563F3ECC1000p+0872 },   // 2.0e+0278
        { 0x1.0EC776CAE162Dp+0925,  0x0.EC80AF6F190C00p+0873 },   // 3.0e+0278
        { 0x1.6909F3B92C83Dp+0925,  0x0.3B563F3ECC1000p+0873 },   // 4.0e+0278
        { 0x1.C34C70A777A4Cp+0925,  0x0.8A2BCF0E7F1400p+0873 },   // 5.0e+0278
        { 0x1.0EC776CAE162Dp+0926,  0x0.EC80AF6F190C00p+0874 },   // 6.0e+0278
        { 0x1.3BE8B54206F35p+0926,  0x0.93EB7756F28E00p+0874 },   // 7.0e+0278
        { 0x1.6909F3B92C83Dp+0926,  0x0.3B563F3ECC1000p+0874 },   // 8.0e+0278
        { 0x1.962B323052144p+0926,  0x0.E2C10726A59200p+0874 },   // 9.0e+0278
        { 0x1.C34C70A777A4Cp+0926,  0x0.8A2BCF0E7F1400p+0874 },   // 1.0e+0279
        { 0x1.C34C70A777A4Cp+0927,  0x0.8A2BCF0E7F1400p+0875 },   // 2.0e+0279
        { 0x1.5279547D99BB9p+0928,  0x0.67A0DB4ADF4F00p+0876 },   // 3.0e+0279
        { 0x1.C34C70A777A4Cp+0928,  0x0.8A2BCF0E7F1400p+0876 },   // 4.0e+0279
        { 0x1.1A0FC668AAC6Fp+0929,  0x0.D65B61690F6C80p+0877 },   // 5.0e+0279
        { 0x1.5279547D99BB9p+0929,  0x0.67A0DB4ADF4F00p+0877 },   // 6.0e+0279
        { 0x1.8AE2E29288B02p+0929,  0x0.F8E6552CAF3180p+0877 },   // 7.0e+0279
        { 0x1.C34C70A777A4Cp+0929,  0x0.8A2BCF0E7F1400p+0877 },   // 8.0e+0279
        { 0x1.FBB5FEBC66996p+0929,  0x0.1B7148F04EF688p+0877 },   // 9.0e+0279
        { 0x1.1A0FC668AAC6Fp+0930,  0x0.D65B61690F6C80p+0878 },   // 1.0e+0280
        { 0x1.1A0FC668AAC6Fp+0931,  0x0.D65B61690F6C80p+0879 },   // 2.0e+0280
        { 0x1.A717A99D002A7p+0931,  0x0.C189121D9722C0p+0879 },   // 3.0e+0280
        { 0x1.1A0FC668AAC6Fp+0932,  0x0.D65B61690F6C80p+0880 },   // 4.0e+0280
        { 0x1.6093B802D578Bp+0932,  0x0.CBF239C35347A0p+0880 },   // 5.0e+0280
        { 0x1.A717A99D002A7p+0932,  0x0.C189121D9722C0p+0880 },   // 6.0e+0280
        { 0x1.ED9B9B372ADC3p+0932,  0x0.B71FEA77DAFDE0p+0880 },   // 7.0e+0280
        { 0x1.1A0FC668AAC6Fp+0933,  0x0.D65B61690F6C80p+0881 },   // 8.0e+0280
        { 0x1.3D51BF35C01FDp+0933,  0x0.D126CD96315A10p+0881 },   // 9.0e+0280
        { 0x1.6093B802D578Bp+0933,  0x0.CBF239C35347A0p+0881 },   // 1.0e+0281
        { 0x1.6093B802D578Bp+0934,  0x0.CBF239C35347A0p+0882 },   // 2.0e+0281
        { 0x1.086ECA02201A8p+0935,  0x0.D8F5AB527E75B8p+0883 },   // 3.0e+0281
        { 0x1.6093B802D578Bp+0935,  0x0.CBF239C35347A0p+0883 },   // 4.0e+0281
        { 0x1.B8B8A6038AD6Ep+0935,  0x0.BEEEC834281988p+0883 },   // 5.0e+0281
        { 0x1.086ECA02201A8p+0936,  0x0.D8F5AB527E75B8p+0884 },   // 6.0e+0281
        { 0x1.348141027AC9Ap+0936,  0x0.5273F28AE8DEB0p+0884 },   // 7.0e+0281
        { 0x1.6093B802D578Bp+0936,  0x0.CBF239C35347A0p+0884 },   // 8.0e+0281
        { 0x1.8CA62F033027Dp+0936,  0x0.457080FBBDB098p+0884 },   // 9.0e+0281
        { 0x1.B8B8A6038AD6Ep+0936,  0x0.BEEEC834281988p+0884 },   // 1.0e+0282
        { 0x1.B8B8A6038AD6Ep+0937,  0x0.BEEEC834281988p+0885 },   // 2.0e+0282
        { 0x1.4A8A7C82A8213p+0938,  0x0.0F3316271E1328p+0886 },   // 3.0e+0282
        { 0x1.B8B8A6038AD6Ep+0938,  0x0.BEEEC834281988p+0886 },   // 4.0e+0282
        { 0x1.137367C236C65p+0939,  0x0.37553D20990FF8p+0887 },   // 5.0e+0282
        { 0x1.4A8A7C82A8213p+0939,  0x0.0F3316271E1328p+0887 },   // 6.0e+0282
        { 0x1.81A19143197C0p+0939,  0x0.E710EF2DA31658p+0887 },   // 7.0e+0282
        { 0x1.B8B8A6038AD6Ep+0939,  0x0.BEEEC834281988p+0887 },   // 8.0e+0282
        { 0x1.EFCFBAC3FC31Cp+0939,  0x0.96CCA13AAD1CC0p+0887 },   // 9.0e+0282
        { 0x1.137367C236C65p+0940,  0x0.37553D20990FF8p+0888 },   // 1.0e+0283
        { 0x1.137367C236C65p+0941,  0x0.37553D20990FF8p+0889 },   // 2.0e+0283
        { 0x1.9D2D1BA352297p+0941,  0x0.D2FFDBB0E597F0p+0889 },   // 3.0e+0283
        { 0x1.137367C236C65p+0942,  0x0.37553D20990FF8p+0890 },   // 4.0e+0283
        { 0x1.585041B2C477Ep+0942,  0x0.852A8C68BF53F0p+0890 },   // 5.0e+0283
        { 0x1.9D2D1BA352297p+0942,  0x0.D2FFDBB0E597F0p+0890 },   // 6.0e+0283
        { 0x1.E209F593DFDB1p+0942,  0x0.20D52AF90BDBF0p+0890 },   // 7.0e+0283
        { 0x1.137367C236C65p+0943,  0x0.37553D20990FF8p+0891 },   // 8.0e+0283
        { 0x1.35E1D4BA7D9F1p+0943,  0x0.DE3FE4C4AC31F8p+0891 },   // 9.0e+0283
        { 0x1.585041B2C477Ep+0943,  0x0.852A8C68BF53F0p+0891 },   // 1.0e+0284
        { 0x1.585041B2C477Ep+0944,  0x0.852A8C68BF53F0p+0892 },   // 2.0e+0284
        { 0x1.023C31461359Ep+0945,  0x0.E3DFE94E8F7EF8p+0893 },   // 3.0e+0284
        { 0x1.585041B2C477Ep+0945,  0x0.852A8C68BF53F0p+0893 },   // 4.0e+0284
        { 0x1.AE64521F7595Ep+0945,  0x0.26752F82EF28F0p+0893 },   // 5.0e+0284
        { 0x1.023C31461359Ep+0946,  0x0.E3DFE94E8F7EF8p+0894 },   // 6.0e+0284
        { 0x1.2D46397C6BE8Ep+0946,  0x0.B4853ADBA76978p+0894 },   // 7.0e+0284
        { 0x1.585041B2C477Ep+0946,  0x0.852A8C68BF53F0p+0894 },   // 8.0e+0284
        { 0x1.835A49E91D06Ep+0946,  0x0.55CFDDF5D73E70p+0894 },   // 9.0e+0284
        { 0x1.AE64521F7595Ep+0946,  0x0.26752F82EF28F0p+0894 },   // 1.0e+0285
        { 0x1.AE64521F7595Ep+0947,  0x0.26752F82EF28F0p+0895 },   // 2.0e+0285
        { 0x1.42CB3D9798306p+0948,  0x0.9CD7E3A2335EB8p+0896 },   // 3.0e+0285
        { 0x1.AE64521F7595Ep+0948,  0x0.26752F82EF28F0p+0896 },   // 4.0e+0285
        { 0x1.0CFEB353A97DAp+0949,  0x0.D8093DB1D57998p+0897 },   // 5.0e+0285
        { 0x1.42CB3D9798306p+0949,  0x0.9CD7E3A2335EB8p+0897 },   // 6.0e+0285
        { 0x1.7897C7DB86E32p+0949,  0x0.61A689929143D0p+0897 },   // 7.0e+0285
        { 0x1.AE64521F7595Ep+0949,  0x0.26752F82EF28F0p+0897 },   // 8.0e+0285
        { 0x1.E430DC6364489p+0949,  0x0.EB43D5734D0E10p+0897 },   // 9.0e+0285
        { 0x1.0CFEB353A97DAp+0950,  0x0.D8093DB1D57998p+0898 },   // 1.0e+0286
        { 0x1.0CFEB353A97DAp+0951,  0x0.D8093DB1D57998p+0899 },   // 2.0e+0286
        { 0x1.937E0CFD7E3C8p+0951,  0x0.440DDC8AC03660p+0899 },   // 3.0e+0286
        { 0x1.0CFEB353A97DAp+0952,  0x0.D8093DB1D57998p+0900 },   // 4.0e+0286
        { 0x1.503E602893DD1p+0952,  0x0.8E0B8D1E4AD7F8p+0900 },   // 5.0e+0286
        { 0x1.937E0CFD7E3C8p+0952,  0x0.440DDC8AC03660p+0900 },   // 6.0e+0286
        { 0x1.D6BDB9D2689BEp+0952,  0x0.FA102BF73594C8p+0900 },   // 7.0e+0286
        { 0x1.0CFEB353A97DAp+0953,  0x0.D8093DB1D57998p+0901 },   // 8.0e+0286
        { 0x1.2E9E89BE1EAD6p+0953,  0x0.330A65681028C8p+0901 },   // 9.0e+0286
        { 0x1.503E602893DD1p+0953,  0x0.8E0B8D1E4AD7F8p+0901 },   // 1.0e+0287
        { 0x1.503E602893DD1p+0954,  0x0.8E0B8D1E4AD7F8p+0902 },   // 2.0e+0287
        { 0x1.F85D903CDDCBAp+0954,  0x0.551153AD7043F8p+0902 },   // 3.0e+0287
        { 0x1.503E602893DD1p+0955,  0x0.8E0B8D1E4AD7F8p+0903 },   // 4.0e+0287
        { 0x1.A44DF832B8D45p+0955,  0x0.F18E7065DD8DF8p+0903 },   // 5.0e+0287
        { 0x1.F85D903CDDCBAp+0955,  0x0.551153AD7043F8p+0903 },   // 6.0e+0287
        { 0x1.2636942381617p+0956,  0x0.5C4A1B7A817CF8p+0904 },   // 7.0e+0287
        { 0x1.503E602893DD1p+0956,  0x0.8E0B8D1E4AD7F8p+0904 },   // 8.0e+0287
        { 0x1.7A462C2DA658Bp+0956,  0x0.BFCCFEC21432F8p+0904 },   // 9.0e+0287
        { 0x1.A44DF832B8D45p+0956,  0x0.F18E7065DD8DF8p+0904 },   // 1.0e+0288
        { 0x1.A44DF832B8D45p+0957,  0x0.F18E7065DD8DF8p+0905 },   // 2.0e+0288
        { 0x1.3B3A7A260A9F4p+0958,  0x0.752AD44C662A78p+0906 },   // 3.0e+0288
        { 0x1.A44DF832B8D45p+0958,  0x0.F18E7065DD8DF8p+0906 },   // 4.0e+0288
        { 0x1.06B0BB1FB384Bp+0959,  0x0.B6F9063FAA78B8p+0907 },   // 5.0e+0288
        { 0x1.3B3A7A260A9F4p+0959,  0x0.752AD44C662A78p+0907 },   // 6.0e+0288
        { 0x1.6FC4392C61B9Dp+0959,  0x0.335CA25921DC38p+0907 },   // 7.0e+0288
        { 0x1.A44DF832B8D45p+0959,  0x0.F18E7065DD8DF8p+0907 },   // 8.0e+0288
        { 0x1.D8D7B7390FEEEp+0959,  0x0.AFC03E72993FB8p+0907 },   // 9.0e+0288
        { 0x1.06B0BB1FB384Bp+0960,  0x0.B6F9063FAA78B8p+0908 },   // 1.0e+0289
        { 0x1.06B0BB1FB384Bp+0961,  0x0.B6F9063FAA78B8p+0909 },   // 2.0e+0289
        { 0x1.8A0918AF8D471p+0961,  0x0.9275895F7FB518p+0909 },   // 3.0e+0289
        { 0x1.06B0BB1FB384Bp+0962,  0x0.B6F9063FAA78B8p+0910 },   // 4.0e+0289
        { 0x1.485CE9E7A065Ep+0962,  0x0.A4B747CF9516E8p+0910 },   // 5.0e+0289
        { 0x1.8A0918AF8D471p+0962,  0x0.9275895F7FB518p+0910 },   // 6.0e+0289
        { 0x1.CBB547777A284p+0962,  0x0.8033CAEF6A5348p+0910 },   // 7.0e+0289
        { 0x1.06B0BB1FB384Bp+0963,  0x0.B6F9063FAA78B8p+0911 },   // 8.0e+0289
        { 0x1.2786D283A9F55p+0963,  0x0.2DD827079FC7D0p+0911 },   // 9.0e+0289
        { 0x1.485CE9E7A065Ep+0963,  0x0.A4B747CF9516E8p+0911 },   // 1.0e+0290
        { 0x1.485CE9E7A065Ep+0964,  0x0.A4B747CF9516E8p+0912 },   // 2.0e+0290
        { 0x1.EC8B5EDB7098Dp+0964,  0x0.F712EBB75FA260p+0912 },   // 3.0e+0290
        { 0x1.485CE9E7A065Ep+0965,  0x0.A4B747CF9516E8p+0913 },   // 4.0e+0290
        { 0x1.9A742461887F6p+0965,  0x0.4DE519C37A5CA8p+0913 },   // 5.0e+0290
        { 0x1.EC8B5EDB7098Dp+0965,  0x0.F712EBB75FA260p+0913 },   // 6.0e+0290
        { 0x1.1F514CAAAC592p+0966,  0x0.D0205ED5A27410p+0914 },   // 7.0e+0290
        { 0x1.485CE9E7A065Ep+0966,  0x0.A4B747CF9516E8p+0914 },   // 8.0e+0290
        { 0x1.716887249472Ap+0966,  0x0.794E30C987B9C8p+0914 },   // 9.0e+0290
        { 0x1.9A742461887F6p+0966,  0x0.4DE519C37A5CA8p+0914 },   // 1.0e+0291
        { 0x1.9A742461887F6p+0967,  0x0.4DE519C37A5CA8p+0915 },   // 2.0e+0291
        { 0x1.33D71B49265F8p+0968,  0x0.BA6BD3529BC580p+0916 },   // 3.0e+0291
        { 0x1.9A742461887F6p+0968,  0x0.4DE519C37A5CA8p+0916 },   // 4.0e+0291
        { 0x1.008896BCF54F9p+0969,  0x0.F0AF301A2C79E8p+0917 },   // 5.0e+0291
        { 0x1.33D71B49265F8p+0969,  0x0.BA6BD3529BC580p+0917 },   // 6.0e+0291
        { 0x1.67259FD5576F7p+0969,  0x0.8428768B0B1110p+0917 },   // 7.0e+0291
        { 0x1.9A742461887F6p+0969,  0x0.4DE519C37A5CA8p+0917 },   // 8.0e+0291
        { 0x1.CDC2A8EDB98F5p+0969,  0x0.17A1BCFBE9A840p+0917 },   // 9.0e+0291
        { 0x1.008896BCF54F9p+0970,  0x0.F0AF301A2C79E8p+0918 },   // 1.0e+0292
        { 0x1.008896BCF54F9p+0971,  0x0.F0AF301A2C79E8p+0919 },   // 2.0e+0292
        { 0x1.80CCE21B6FF76p+0971,  0x0.E906C82742B6E0p+0919 },   // 3.0e+0292
        { 0x1.008896BCF54F9p+0972,  0x0.F0AF301A2C79E8p+0920 },   // 4.0e+0292
        { 0x1.40AABC6C32A38p+0972,  0x0.6CDAFC20B79860p+0920 },   // 5.0e+0292
        { 0x1.80CCE21B6FF76p+0972,  0x0.E906C82742B6E0p+0920 },   // 6.0e+0292
        { 0x1.C0EF07CAAD4B5p+0972,  0x0.6532942DCDD558p+0920 },   // 7.0e+0292
        { 0x1.008896BCF54F9p+0973,  0x0.F0AF301A2C79E8p+0921 },   // 8.0e+0292
        { 0x1.2099A99493F99p+0973,  0x0.2EC5161D720928p+0921 },   // 9.0e+0292
        { 0x1.40AABC6C32A38p+0973,  0x0.6CDAFC20B79860p+0921 },   // 1.0e+0293
        { 0x1.40AABC6C32A38p+0974,  0x0.6CDAFC20B79860p+0922 },   // 2.0e+0293
        { 0x1.E1001AA24BF54p+0974,  0x0.A3487A31136498p+0922 },   // 3.0e+0293
        { 0x1.40AABC6C32A38p+0975,  0x0.6CDAFC20B79860p+0923 },   // 4.0e+0293
        { 0x1.90D56B873F4C6p+0975,  0x0.8811BB28E57E78p+0923 },   // 5.0e+0293
        { 0x1.E1001AA24BF54p+0975,  0x0.A3487A31136498p+0923 },   // 6.0e+0293
        { 0x1.189564DEAC4F1p+0976,  0x0.5F3F9C9CA0A558p+0924 },   // 7.0e+0293
        { 0x1.40AABC6C32A38p+0976,  0x0.6CDAFC20B79860p+0924 },   // 8.0e+0293
        { 0x1.68C013F9B8F7Fp+0976,  0x0.7A765BA4CE8B70p+0924 },   // 9.0e+0293
        { 0x1.90D56B873F4C6p+0976,  0x0.8811BB28E57E78p+0924 },   // 1.0e+0294
        { 0x1.90D56B873F4C6p+0977,  0x0.8811BB28E57E78p+0925 },   // 2.0e+0294
        { 0x1.2CA010A56F794p+0978,  0x0.E60D4C5EAC1ED8p+0926 },   // 3.0e+0294
        { 0x1.90D56B873F4C6p+0978,  0x0.8811BB28E57E78p+0926 },   // 4.0e+0294
        { 0x1.F50AC6690F1F8p+0978,  0x0.2A1629F31EDE18p+0926 },   // 5.0e+0294
        { 0x1.2CA010A56F794p+0979,  0x0.E60D4C5EAC1ED8p+0927 },   // 6.0e+0294
        { 0x1.5EBABE165762Dp+0979,  0x0.B70F83C3C8CEA8p+0927 },   // 7.0e+0294
        { 0x1.90D56B873F4C6p+0979,  0x0.8811BB28E57E78p+0927 },   // 8.0e+0294
        { 0x1.C2F018F82735Fp+0979,  0x0.5913F28E022E48p+0927 },   // 9.0e+0294
        { 0x1.F50AC6690F1F8p+0979,  0x0.2A1629F31EDE18p+0927 },   // 1.0e+0295
        { 0x1.F50AC6690F1F8p+0980,  0x0.2A1629F31EDE18p+0928 },   // 2.0e+0295
        { 0x1.77C814CECB57Ap+0981,  0x0.1F909F76572690p+0929 },   // 3.0e+0295
        { 0x1.F50AC6690F1F8p+0981,  0x0.2A1629F31EDE18p+0929 },   // 4.0e+0295
        { 0x1.3926BC01A973Bp+0982,  0x0.1A4DDA37F34AD0p+0930 },   // 5.0e+0295
        { 0x1.77C814CECB57Ap+0982,  0x0.1F909F76572690p+0930 },   // 6.0e+0295
        { 0x1.B6696D9BED3B9p+0982,  0x0.24D364B4BB0258p+0930 },   // 7.0e+0295
        { 0x1.F50AC6690F1F8p+0982,  0x0.2A1629F31EDE18p+0930 },   // 8.0e+0295
        { 0x1.19D60F9B1881Bp+0983,  0x0.97AC7798C15CF0p+0931 },   // 9.0e+0295
        { 0x1.3926BC01A973Bp+0983,  0x0.1A4DDA37F34AD0p+0931 },   // 1.0e+0296
        { 0x1.3926BC01A973Bp+0984,  0x0.1A4DDA37F34AD0p+0932 },   // 2.0e+0296
        { 0x1.D5BA1A027E2D8p+0984,  0x0.A774C753ECF038p+0932 },   // 3.0e+0296
        { 0x1.3926BC01A973Bp+0985,  0x0.1A4DDA37F34AD0p+0933 },   // 4.0e+0296
        { 0x1.87706B0213D09p+0985,  0x0.E0E150C5F01D88p+0933 },   // 5.0e+0296
        { 0x1.D5BA1A027E2D8p+0985,  0x0.A774C753ECF038p+0933 },   // 6.0e+0296
        { 0x1.1201E48174453p+0986,  0x0.B7041EF0F4E178p+0934 },   // 7.0e+0296
        { 0x1.3926BC01A973Bp+0986,  0x0.1A4DDA37F34AD0p+0934 },   // 8.0e+0296
        { 0x1.604B9381DEA22p+0986,  0x0.7D97957EF1B428p+0934 },   // 9.0e+0296
        { 0x1.87706B0213D09p+0986,  0x0.E0E150C5F01D88p+0934 },   // 1.0e+0297
        { 0x1.87706B0213D09p+0987,  0x0.E0E150C5F01D88p+0935 },   // 2.0e+0297
        { 0x1.259450418EDC7p+0988,  0x0.68A8FC94741620p+0936 },   // 3.0e+0297
        { 0x1.87706B0213D09p+0988,  0x0.E0E150C5F01D88p+0936 },   // 4.0e+0297
        { 0x1.E94C85C298C4Cp+0988,  0x0.5919A4F76C24E8p+0936 },   // 5.0e+0297
        { 0x1.259450418EDC7p+0989,  0x0.68A8FC94741620p+0937 },   // 6.0e+0297
        { 0x1.56825DA1D1568p+0989,  0x0.A4C526AD3219D0p+0937 },   // 7.0e+0297
        { 0x1.87706B0213D09p+0989,  0x0.E0E150C5F01D88p+0937 },   // 8.0e+0297
        { 0x1.B85E7862564ABp+0989,  0x0.1CFD7ADEAE2138p+0937 },   // 9.0e+0297
        { 0x1.E94C85C298C4Cp+0989,  0x0.5919A4F76C24E8p+0937 },   // 1.0e+0298
        { 0x1.E94C85C298C4Cp+0990,  0x0.5919A4F76C24E8p+0938 },   // 2.0e+0298
        { 0x1.6EF96451F2939p+0991,  0x0.42D33BB9911BB0p+0939 },   // 3.0e+0298
        { 0x1.E94C85C298C4Cp+0991,  0x0.5919A4F76C24E8p+0939 },   // 4.0e+0298
        { 0x1.31CFD3999F7AFp+0992,  0x0.B7B0071AA39710p+0940 },   // 5.0e+0298
        { 0x1.6EF96451F2939p+0992,  0x0.42D33BB9911BB0p+0940 },   // 6.0e+0298
        { 0x1.AC22F50A45AC2p+0992,  0x0.CDF670587EA048p+0940 },   // 7.0e+0298
        { 0x1.E94C85C298C4Cp+0992,  0x0.5919A4F76C24E8p+0940 },   // 8.0e+0298
        { 0x1.133B0B3D75EEAp+0993,  0x0.F21E6CCB2CD4C0p+0941 },   // 9.0e+0298
        { 0x1.31CFD3999F7AFp+0993,  0x0.B7B0071AA39710p+0941 },   // 1.0e+0299
        { 0x1.31CFD3999F7AFp+0994,  0x0.B7B0071AA39710p+0942 },   // 2.0e+0299
        { 0x1.CAB7BD666F387p+0994,  0x0.93880AA7F56298p+0942 },   // 3.0e+0299
        { 0x1.31CFD3999F7AFp+0995,  0x0.B7B0071AA39710p+0943 },   // 4.0e+0299
        { 0x1.7E43C8800759Bp+0995,  0x0.A59C08E14C7CD0p+0943 },   // 5.0e+0299
        { 0x1.CAB7BD666F387p+0995,  0x0.93880AA7F56298p+0943 },   // 6.0e+0299
        { 0x1.0B95D9266B8B9p+0996,  0x0.C0BA06374F2430p+0944 },   // 7.0e+0299
        { 0x1.31CFD3999F7AFp+0996,  0x0.B7B0071AA39710p+0944 },   // 8.0e+0299
        { 0x1.5809CE0CD36A5p+0996,  0x0.AEA607FDF809F0p+0944 },   // 9.0e+0299
        { 0x1.7E43C8800759Bp+0996,  0x0.A59C08E14C7CD0p+0944 },   // 1.0e+0300
        { 0x1.7E43C8800759Bp+0997,  0x0.A59C08E14C7CD0p+0945 },   // 2.0e+0300
        { 0x1.1EB2D66005834p+0998,  0x0.BC3506A8F95DA0p+0946 },   // 3.0e+0300
        { 0x1.7E43C8800759Bp+0998,  0x0.A59C08E14C7CD0p+0946 },   // 4.0e+0300
        { 0x1.DDD4BAA009302p+0998,  0x0.8F030B199F9C08p+0946 },   // 5.0e+0300
        { 0x1.1EB2D66005834p+0999,  0x0.BC3506A8F95DA0p+0947 },   // 6.0e+0300
        { 0x1.4E7B4F70066E8p+0999,  0x0.30E887C522ED38p+0947 },   // 7.0e+0300
        { 0x1.7E43C8800759Bp+0999,  0x0.A59C08E14C7CD0p+0947 },   // 8.0e+0300
        { 0x1.AE0C41900844Fp+0999,  0x0.1A4F89FD760C70p+0947 },   // 9.0e+0300
        { 0x1.DDD4BAA009302p+0999,  0x0.8F030B199F9C08p+0947 },   // 1.0e+0301
        { 0x1.DDD4BAA009302p+1000,  0x0.8F030B199F9C08p+0948 },   // 2.0e+0301
        { 0x1.665F8BF806E41p+1001,  0x0.EB42485337B508p+0949 },   // 3.0e+0301
        { 0x1.DDD4BAA009302p+1001,  0x0.8F030B199F9C08p+0949 },   // 4.0e+0301
        { 0x1.2AA4F4A405BE1p+1002,  0x0.9961E6F003C188p+0950 },   // 5.0e+0301
        { 0x1.665F8BF806E41p+1002,  0x0.EB42485337B508p+0950 },   // 6.0e+0301
        { 0x1.A21A234C080A2p+1002,  0x0.3D22A9B66BA888p+0950 },   // 7.0e+0301
        { 0x1.DDD4BAA009302p+1002,  0x0.8F030B199F9C08p+0950 },   // 8.0e+0301
        { 0x1.0CC7A8FA052B1p+1003,  0x0.7071B63E69C7C0p+0951 },   // 9.0e+0301
        { 0x1.2AA4F4A405BE1p+1003,  0x0.9961E6F003C188p+0951 },   // 1.0e+0302
        { 0x1.2AA4F4A405BE1p+1004,  0x0.9961E6F003C188p+0952 },   // 2.0e+0302
        { 0x1.BFF76EF6089D2p+1004,  0x0.6612DA6805A248p+0952 },   // 3.0e+0302
        { 0x1.2AA4F4A405BE1p+1005,  0x0.9961E6F003C188p+0953 },   // 4.0e+0302
        { 0x1.754E31CD072D9p+1005,  0x0.FFBA60AC04B1E8p+0953 },   // 5.0e+0302
        { 0x1.BFF76EF6089D2p+1005,  0x0.6612DA6805A248p+0953 },   // 6.0e+0302
        { 0x1.0550560F85065p+1006,  0x0.6635AA12034950p+0954 },   // 7.0e+0302
        { 0x1.2AA4F4A405BE1p+1006,  0x0.9961E6F003C188p+0954 },   // 8.0e+0302
        { 0x1.4FF993388675Dp+1006,  0x0.CC8E23CE0439B8p+0954 },   // 9.0e+0302
        { 0x1.754E31CD072D9p+1006,  0x0.FFBA60AC04B1E8p+0954 },   // 1.0e+0303
        { 0x1.754E31CD072D9p+1007,  0x0.FFBA60AC04B1E8p+0955 },   // 2.0e+0303
        { 0x1.17FAA559C5623p+1008,  0x0.7FCBC881038570p+0956 },   // 3.0e+0303
        { 0x1.754E31CD072D9p+1008,  0x0.FFBA60AC04B1E8p+0956 },   // 4.0e+0303
        { 0x1.D2A1BE4048F90p+1008,  0x0.7FA8F8D705DE60p+0956 },   // 5.0e+0303
        { 0x1.17FAA559C5623p+1009,  0x0.7FCBC881038570p+0957 },   // 6.0e+0303
        { 0x1.46A46B936647Ep+1009,  0x0.BFC31496841BA8p+0957 },   // 7.0e+0303
        { 0x1.754E31CD072D9p+1009,  0x0.FFBA60AC04B1E8p+0957 },   // 8.0e+0303
        { 0x1.A3F7F806A8135p+1009,  0x0.3FB1ACC1854820p+0957 },   // 9.0e+0303
        { 0x1.D2A1BE4048F90p+1009,  0x0.7FA8F8D705DE60p+0957 },   // 1.0e+0304
        { 0x1.D2A1BE4048F90p+1010,  0x0.7FA8F8D705DE60p+0958 },   // 2.0e+0304
        { 0x1.5DF94EB036BACp+1011,  0x0.5FBEBAA14466C8p+0959 },   // 3.0e+0304
        { 0x1.D2A1BE4048F90p+1011,  0x0.7FA8F8D705DE60p+0959 },   // 4.0e+0304
        { 0x1.23A516E82D9BAp+1012,  0x0.4FC99B8663AAF8p+0960 },   // 5.0e+0304
        { 0x1.5DF94EB036BACp+1012,  0x0.5FBEBAA14466C8p+0960 },   // 6.0e+0304
        { 0x1.984D86783FD9Ep+1012,  0x0.6FB3D9BC252298p+0960 },   // 7.0e+0304
        { 0x1.D2A1BE4048F90p+1012,  0x0.7FA8F8D705DE60p+0960 },   // 8.0e+0304
        { 0x1.067AFB04290C1p+1013,  0x0.47CF0BF8F34D18p+0961 },   // 9.0e+0304
        { 0x1.23A516E82D9BAp+1013,  0x0.4FC99B8663AAF8p+0961 },   // 1.0e+0305
        { 0x1.23A516E82D9BAp+1014,  0x0.4FC99B8663AAF8p+0962 },   // 2.0e+0305
        { 0x1.B577A25C44697p+1014,  0x0.77AE6949958078p+0962 },   // 3.0e+0305
        { 0x1.23A516E82D9BAp+1015,  0x0.4FC99B8663AAF8p+0963 },   // 4.0e+0305
        { 0x1.6C8E5CA239028p+1015,  0x0.E3BC0267FC95B8p+0963 },   // 5.0e+0305
        { 0x1.B577A25C44697p+1015,  0x0.77AE6949958078p+0963 },   // 6.0e+0305
        { 0x1.FE60E8164FD06p+1015,  0x0.0BA0D02B2E6B38p+0963 },   // 7.0e+0305
        { 0x1.23A516E82D9BAp+1016,  0x0.4FC99B8663AAF8p+0964 },   // 8.0e+0305
        { 0x1.4819B9C5334F1p+1016,  0x0.99C2CEF7302058p+0964 },   // 9.0e+0305
        { 0x1.6C8E5CA239028p+1016,  0x0.E3BC0267FC95B8p+0964 },   // 1.0e+0306
        { 0x1.6C8E5CA239028p+1017,  0x0.E3BC0267FC95B8p+0965 },   // 2.0e+0306
        { 0x1.116AC579AAC1Ep+1018,  0x0.AACD01CDFD7048p+0966 },   // 3.0e+0306
        { 0x1.6C8E5CA239028p+1018,  0x0.E3BC0267FC95B8p+0966 },   // 4.0e+0306
        { 0x1.C7B1F3CAC7433p+1018,  0x0.1CAB0301FBBB28p+0966 },   // 5.0e+0306
        { 0x1.116AC579AAC1Ep+1019,  0x0.AACD01CDFD7048p+0967 },   // 6.0e+0306
        { 0x1.3EFC910DF1E23p+1019,  0x0.C744821AFD0300p+0967 },   // 7.0e+0306
        { 0x1.6C8E5CA239028p+1019,  0x0.E3BC0267FC95B8p+0967 },   // 8.0e+0306
        { 0x1.9A2028368022Ep+1019,  0x0.003382B4FC2870p+0967 },   // 9.0e+0306
        { 0x1.C7B1F3CAC7433p+1019,  0x0.1CAB0301FBBB28p+0967 },   // 1.0e+0307
        { 0x1.C7B1F3CAC7433p+1020,  0x0.1CAB0301FBBB28p+0968 },   // 2.0e+0307
        { 0x1.55C576D815726p+1021,  0x0.558042417CCC60p+0969 },   // 3.0e+0307
        { 0x1.C7B1F3CAC7433p+1021,  0x0.1CAB0301FBBB28p+0969 },   // 4.0e+0307
        { 0x1.1CCF385EBC89Fp+1022,  0x0.F1EAE1E13D54F8p+0970 },   // 5.0e+0307
        { 0x1.55C576D815726p+1022,  0x0.558042417CCC60p+0970 },   // 6.0e+0307
        { 0x1.8EBBB5516E5ACp+1022,  0x0.B915A2A1BC43C8p+0970 },   // 7.0e+0307
        { 0x1.C7B1F3CAC7433p+1022,  0x0.1CAB0301FBBB28p+0970 },   // 8.0e+0307
        { 0x1.005419221015Cp+1023,  0x0.C02031B11D9948p+0971 },   // 9.0e+0307
        { 0x1.1CCF385EBC89Fp+1023,  0x0.F1EAE1E13D54F8p+0971 },   // 1.0e+0308
        {                HUGE_VAL,                  HUGE_VAL },   // 2.0e+0308
        {                HUGE_VAL,                  HUGE_VAL },   // 3.0e+0308
        {                HUGE_VAL,                  HUGE_VAL },   // 4.0e+0308
        {                HUGE_VAL,                  HUGE_VAL },   // 5.0e+0308
        {                HUGE_VAL,                  HUGE_VAL },   // 6.0e+0308
        {                HUGE_VAL,                  HUGE_VAL },   // 7.0e+0308
        {                HUGE_VAL,                  HUGE_VAL },   // 8.0e+0308
        {                HUGE_VAL,                  HUGE_VAL },   // 9.0e+0308
      };
    // Look at the table:
    // 0) There are 2 zeroes at the beginning.
    // 1) There are 7 numbers with an exponent of -324.
    // 2) There are 9*631 numbers with an exponent in the range [-323,+307].
    // 3) There is 1 number with an exponent of +308.
    // 4) There are 8 infinity values at the end.
    // Hence, there are 5697 numbers in this table.
    static_assert(noadl::countof(s_decbounds_F) == 5697, "??");

    ptrdiff_t do_xbisect_decbounds(ptrdiff_t start, ptrdiff_t count, const double& value) noexcept
      {
        ROCKET_ASSERT(count > 0);
        // Locate the last number in the table that is <= `value`.
        // This is equivalent to `::std::count_bound(start, start + count, value) - start - 1`.
        // Note that this function may return a negative offset.
        ptrdiff_t bpos = start;
        ptrdiff_t epos = start + count;
        while(bpos != epos) {
          // Get the median.
          // We know `epos - bpos` cannot be negative, so `>> 1` is a bit faster than `/ 2`.
          ptrdiff_t mpos = bpos + ((epos - bpos) >> 1);
          const double& med = s_decbounds_F[mpos][0];
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
        ptrdiff_t dpos = do_xbisect_decbounds(0, 5697, reg);
        if(ROCKET_UNEXPECT(dpos < 0)) {
          // Return zero.
          exp = 0;
          mant = 0;
          return;
        }
        // Set `dbase` to the beginning of a sequence of 9 numbers with the same exponent.
        // This also calculates the exponent on the way.
        ptrdiff_t dbase = static_cast<ptrdiff_t>(do_cast_U(dpos) / 9);
        exp = static_cast<int>(dbase - 324);
        dbase *= 9;
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
        // After shifting the most significant digit out, accumulate the rounding
        // error from the other end here. This error is very tiny when compared with
        // the origin value (it's about `0x1p-52` times) so only apply this fix when
        // there have been enough digits.
        double com = 0;
        // Shift some digits into `ireg`.
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
            reg -= s_decbounds_F[dpos][0];
            com += s_decbounds_F[dpos][1];
          }
          // Fix the rounding error when there have been 12 digits.
          if(dcnt == 12) {
            reg -= com;
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
        char* tbp = begin(temps);
        while(reg != 0) {
          // Shift a digit out.
          size_t dval = static_cast<size_t>(reg % 10);
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

ascii_numput& ascii_numput::put_DF(double value) noexcept
  {
    this->clear();
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
      else {
        // ... in plain format; the decimal point is inserted in the middle.
        do_xput_M_dec(ep, mant, ep + 1 + do_cast_U(exp));
      }
      // Append a null terminator.
      *ep = 0;
    }
    // Set the string. The internal storage is used for finite values only.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

ascii_numput& ascii_numput::put_DE(double value) noexcept
  {
    this->clear();
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
      // Append a null terminator.
      *ep = 0;
    }
    // Set the string. The internal storage is used for finite values only.
    this->m_bptr = bp;
    this->m_eptr = ep;
    return *this;
  }

}  // namespace rocket
