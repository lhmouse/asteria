// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "ascii_numget.hpp"
#include "assert.hpp"
#include <cmath>

namespace rocket {

ascii_numget& ascii_numget::parse_B(const char*& bptr, const char* eptr) noexcept
  {
    this->clear();
    const char* rp = bptr;
    // Check for "0", "1", "false" or "true".
    bool value;
    if(rp == eptr) {
      return *this;
    }
    switch(rp[0]) {
    case '0':
      {
        // Accept a "0".
        value = 0;
        rp += 1;
        break;
      }
    case '1':
      {
        // Accept a "1".
        value = 1;
        rp += 1;
        break;
      }
    case 'f':
      {
        // Check whether the string starts with "false".
        // Compare characters in a quadruple which might be optimized better.
        if(eptr - rp < 5) {
          return *this;
        }
        if(::std::memcmp(rp + 1, "false" + 1, 4) != 0) {
          return *this;
        }
        // Accept a "false".
        value = 0;
        rp += 5;
        break;
      }
    case 't':
      {
        // Check whether the string starts with "false".
        // Compare characters in a quadruple which might be optimized better.
        if(eptr - rp < 4) {
          return *this;
        }
        if(::std::memcmp(rp, "true", 4) != 0) {
          return *this;
        }
        // Accept a "true".
        value = 1;
        rp += 4;
        break;
      }
    default:
      // The character could not be consumed.
      return *this;
    }
    // Set the value.
    this->m_base = 2;
    this->m_mant = value;
    // Report success and advance the read pointer.
    this->m_succ = true;
    bptr = rp;
    return *this;
  }

    namespace {

    bool do_get_sign(const char*& rp, const char* eptr) noexcept
      {
        // Look ahead for at most 1 character.
        if(eptr - rp < 1) {
          return 0;
        }
        switch(rp[0]) {
        case '+':
          {
            // Skip the plus sign.
            rp += 1;
            return 0;
          }
        case '-':
          {
            // Skip the minus sign.
            rp += 1;
            return 1;
          }
        }
        // Assume the number is non-negative.
        return 0;
      }

    constexpr bool do_match_char_ci(char ch, char cmp) noexcept
      {
        return static_cast<uint8_t>(ch | 0x20) == static_cast<uint8_t>(cmp);
      }

    uint8_t do_get_base(const char*& rp, const char* eptr) noexcept
      {
        // Look ahead for at most 2 characters.
        if(eptr - rp < 2) {
          return 10;
        }
        if(rp[0] != '0') {
          return 10;
        }
        // Check for binary and hexadecimal prefixes.
        if(do_match_char_ci(rp[1], 'b')) {
          rp += 2;
          return 2;
        }
        if(do_match_char_ci(rp[1], 'x')) {
          rp += 2;
          return 16;
        }
        // Assume the number is decimal.
        return 10;
      }

    struct mantissa
      {
        uint64_t value;  // significant figures
        uint8_t xvadd;  // bitwise OR of all figures overflowed
        size_t novfl;  // number of significant figures overflowed
      };

    constexpr uint8_t s_digits[] =
      {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
          0,   1,   2,   3,   4,   5,   6,   7,   8,   9, 255, 255, 255, 255, 255, 255,  // 0-9
        255,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  // A-O
         25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35, 255, 255, 255, 255, 255,  // P-Z
        255,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  // a-o
         25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35, 255, 255, 255, 255, 255,  // p-z
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      };

    size_t do_collect_U(mantissa& m, const char*& rp, const char* eptr,
                        uint8_t base, uint64_t limit) noexcept
      {
        size_t nread = 0;
        for(;;) {
          // Get a digit.
          if(rp == eptr) {
            break;
          }
          uint8_t dval = s_digits[static_cast<uint8_t>(rp[0])];
          if(dval >= base) {
            break;
          }
          nread += 1;
          rp += 1;
          // Ignore leading zeroes.
          if((dval == 0) && (m.value == 0)) {
            continue;
          }
          if(m.value <= (limit - dval) / base) {
            // Append this digit to `value`.
            m.value *= base;
            m.value += dval;
          }
          else {
            // Record an overflowed digit.
            m.xvadd |= dval;
            m.novfl += 1;
          }
        }
        return nread;
      }

    }

ascii_numget& ascii_numget::parse_P(const char*& bptr, const char* eptr) noexcept
  {
    this->clear();
    const char* rp = bptr;
    // Check for the "0x" prefix, which is required.
    uint8_t base = do_get_base(rp, eptr);
    if(base != 16) {
      return *this;
    }
    // Get the mantissa.
    mantissa m = { };
    if(do_collect_U(m, rp, eptr, base, UINT64_MAX) == 0) {
      return *this;
    }
    if(m.novfl != 0) {
      // Set an infinity if not all significant figures could be collected.
      this->m_vcls = 2;  // infinity
    }
    else {
      // Set the value.
      this->m_base = base;
      this->m_mant = m.value;
    }
    // Report success and advance the read pointer.
    this->m_succ = true;
    bptr = rp;
    return *this;
  }

ascii_numget& ascii_numget::parse_U(const char*& bptr, const char* eptr) noexcept
  {
    this->clear();
    const char* rp = bptr;
    // Check for the base prefix, which is optional.
    uint8_t base = do_get_base(rp, eptr);
    // Get the mantissa.
    mantissa m = { };
    if(do_collect_U(m, rp, eptr, base, UINT64_MAX) == 0) {
      return *this;
    }
    if(m.novfl != 0) {
      // Set an infinity if not all significant figures could be collected.
      this->m_vcls = 2;  // infinity
    }
    else {
      // Set the value.
      this->m_base = base;
      this->m_mant = m.value;
    }
    // Report success and advance the read pointer.
    this->m_succ = true;
    bptr = rp;
    return *this;
  }

ascii_numget& ascii_numget::parse_I(const char*& bptr, const char* eptr) noexcept
  {
    this->clear();
    const char* rp = bptr;
    // Check for the sign.
    bool sign = do_get_sign(rp, eptr);
    // Check for the base prefix, which is optional.
    uint8_t base = do_get_base(rp, eptr);
    // Get the mantissa.
    mantissa m = { };
    if(do_collect_U(m, rp, eptr, base, UINT64_MAX) == 0) {
      return *this;
    }
    if(m.novfl != 0) {
      // Set an infinity if not all significant figures could be collected.
      this->m_vcls = 2;  // infinity
    }
    else {
      // Set the value.
      this->m_base = base;
      this->m_mant = m.value;
    }
    this->m_sign = sign;
    // Report success and advance the read pointer.
    this->m_succ = true;
    bptr = rp;
    return *this;
  }

ascii_numget& ascii_numget::parse_F(const char*& bptr, const char* eptr) noexcept
  {
    this->clear();
    const char* rp = bptr;
    // Check for the sign.
    bool sign = do_get_sign(rp, eptr);
    // Check for "infinity".
    if((eptr - rp >= 8) && (::std::memcmp(rp, "infinity", 8) == 0)) {
      // Skip the string.
      rp += 8;
      // Set an infinity.
      this->m_vcls = 2;  // infinity
    }
    else if((eptr - rp >= 3) && (::std::memcmp(rp, "nan", 3) == 0)) {
      // Skip the string.
      rp += 3;
      // Set a QNaN.
      this->m_vcls = 3;  // quiet NaN
    }
    else {
      // Check for the base prefix, which is optional.
      uint8_t base = do_get_base(rp, eptr);
      // Get the mantissa.
      mantissa m = { };
      if(do_collect_U(m, rp, eptr, base, UINT64_MAX) == 0) {
        return *this;
      }
      // Check for the radix point, which is optional.
      // If the radix point exists, the fractional part shall follow.
      size_t nfrac = 0;
      if((eptr - rp >= 1) && (rp[0] == '.')) {
        // Skip the radix point.
        rp += 1;
        // Get the fractional part, which is required.
        nfrac = do_collect_U(m, rp, eptr, base, UINT64_MAX);
        if(nfrac == 0)
          return *this;
      }
      // Initialize the exponent.
      int64_t expo = static_cast<int64_t>(m.novfl) - static_cast<int64_t>(nfrac);
      bool erdx = true;
      bool has_expo = false;
      // Check for the exponent.
      switch(base) {
      case 16:
        {
          expo *= 4;  // log2(16)
          // Fallthrough
      case 2:
          erdx = false;
          // A binary exponent is expected.
          if((eptr - rp >= 1) && do_match_char_ci(rp[0], 'p')) {
            // Skip the exponent initiator.
            rp += 1;
            has_expo = true;
          }
          break;
        }
      case 10:
        {
          erdx = true;
          // A decimal exponent is expected.
          if((base == 10) && do_match_char_ci(rp[0], 'e')) {
            // Skip the exponent initiator.
            rp += 1;
            has_expo = true;
          }
          break;
        }
      }
      if(has_expo) {
        // Get the sign of the exponent, which is optional.
        bool esign = do_get_sign(rp, eptr);
        // Get the exponent.
        mantissa em = { };
        if(do_collect_U(em, rp, eptr, 10, UINT32_MAX) == 0) {
          return *this;
        }
        // This shall not overflow.
        if(esign)
          expo -= static_cast<int64_t>(em.value);
        else
          expo += static_cast<int64_t>(em.value);
      }
      if(m.value == 0) {
        // Zero out the exponent if the mantissa is zero.
        expo = 0;
      }
      // Normalize the exponent.
      if(expo < -0x0FFF'FFFF) {  // 28 bits
        // Set an infinitesimal if the exponent is too small.
        this->m_vcls = 1;  // infinitesimal
      }
      else if(expo > +0x0FFF'FFFF) {  // 28 bits
        // Set an infinity if the exponent is too large.
        this->m_vcls = 2;  // infinity
      }
      else {
        // Set the value.
        this->m_erdx = erdx;
        this->m_madd = m.xvadd != 0;
        this->m_base = base;
        this->m_expo = static_cast<int32_t>(expo);
        this->m_mant = m.value;
      }
    }
    this->m_sign = sign;
    // Report success and advance the read pointer.
    this->m_succ = true;
    bptr = rp;
    return *this;
  }

ascii_numget& ascii_numget::cast_U(uint64_t& value, uint64_t lower, uint64_t upper) noexcept
  {
    this->m_stat = 0;
    // Try casting the value.
    switch(this->m_vcls) {
    case 0:  // finite
      {
        uint64_t ireg = this->m_mant;
        if(ireg == 0) {
          // The value is effectively zero.
          value = 0;
          break;
        }
        if(this->m_sign) {
          // Negative values are always out of range.
          value = 0;
          // For unsigned integers this always overflows and is always inexact.
          this->m_ovfl = true;
          this->m_inxc = true;
          break;
        }
        // Get the base.
        uint8_t base = 2;
        if(this->m_erdx) {
          base = this->m_base;
        }
        // Raise the mantissa accordingly.
        if(this->m_expo > 0) {
          for(int32_t i = 0; i != this->m_expo; ++i) {
            uint64_t next = ireg * base;
            // TODO: Overflow checks can be performed using intrinsics.
            if(next / base != ireg) {
              ireg = UINT64_MAX;
              this->m_ovfl = true;
              break;
            }
            ireg = next;
          }
        }
        else {
          for(int32_t i = 0; i != this->m_expo; --i) {
            uint64_t next = ireg / base;
            // Set the inexact flag if a non-zero digit was shifted out.
            if(ireg % base != 0) {
              this->m_inxc = true;
            }
            // TODO: Overflow checks can be performed using intrinsics.
            if(next == 0) {
              ireg = 0;
              this->m_udfl = true;
              break;
            }
            ireg = next;
          }
        }
        // Set the value.
        value = ireg;
        break;
      }
    case 1:  // infinitesimal
      {
        // Truncate the value to zero.
        value = 0;
        // If the value is negative we still have to set the overflow flag.
        if(this->m_sign) {
          this->m_ovfl = true;
        }
        // For integers this always underflows and is always inexact.
        this->m_udfl = true;
        this->m_inxc = true;
        break;
      }
    case 2:  // infinity
      {
        // Return the maximum value.
        value = numeric_limits<uint64_t>::max();
        // For integers this always overflows and is always inexact.
        this->m_ovfl = true;
        this->m_inxc = true;
        break;
      }
    default:  // quiet NaN
      {
        // Return zero.
        value = 0;
        // For integers this is always inexact.
        this->m_inxc = true;
        break;
      }
    }
    // Set the overflow flag if the value is out of range.
    if(value < lower) {
      value = lower;
      this->m_ovfl = true;
    }
    else if(value > upper) {
      value = upper;
      this->m_ovfl = true;
    }
    // Report success if no error occurred.
    if(this->m_stat != 0) {
      return *this;
    }
    this->m_succ = true;
    return *this;
  }

ascii_numget& ascii_numget::cast_I(int64_t& value, int64_t lower, int64_t upper) noexcept
  {
    this->m_stat = 0;
    // Try casting the value.
    switch(this->m_vcls) {
    case 0:  // finite
      {
        uint64_t ireg = this->m_mant;
        if(ireg == 0) {
          // The value is effectively zero.
          value = 0;
          break;
        }
        // Get the base.
        uint8_t base = 2;
        if(this->m_erdx) {
          base = this->m_base;
        }
        // Raise the mantissa accordingly.
        if(this->m_expo > 0) {
          for(int32_t i = 0; i != this->m_expo; ++i) {
            uint64_t next = ireg * base;
            // TODO: Overflow checks can be performed using intrinsics.
            if(next / base != ireg) {
              ireg = UINT64_MAX;
              this->m_ovfl = true;
              break;
            }
            ireg = next;
          }
        }
        else {
          for(int32_t i = 0; i != this->m_expo; --i) {
            uint64_t next = ireg / base;
            // Set the inexact flag if a non-zero digit was shifted out.
            if(ireg % base != 0) {
              this->m_inxc = true;
            }
            // TODO: Overflow checks can be performed using intrinsics.
            if(next == 0) {
              ireg = 0;
              this->m_udfl = true;
              break;
            }
            ireg = next;
          }
        }
        // Set the value.
        if(this->m_sign) {
          // The value is negative.
          constexpr uint64_t imax = 0x8000'0000'0000'0000;
          if(ireg > imax) {
            ireg = imax;
            this->m_ovfl = true;
          }
          value = static_cast<int64_t>(-ireg);
        }
        else {
          // The value is positive.
          constexpr uint64_t imax = 0x7FFF'FFFF'FFFF'FFFF;
          if(ireg > imax) {
            ireg = imax;
            this->m_ovfl = true;
          }
          value = static_cast<int64_t>(+ireg);
        }
        break;
      }
    case 1:  // infinitesimal
      {
        // Truncate the value to zero.
        value = 0;
        // For integers this always underflows and is always inexact.
        this->m_udfl = true;
        this->m_inxc = true;
        break;
      }
    case 2:  // infinity
      {
        // Return the maximum value.
        value = numeric_limits<int64_t>::max();
        // For integers this always overflows and is always inexact.
        this->m_ovfl = true;
        this->m_inxc = true;
        break;
      }
    default:  // quiet NaN
      {
        // Return zero.
        value = 0;
        // For integers this is always inexact.
        this->m_inxc = true;
        break;
      }
    }
    // Set the overflow flag if the value is out of range.
    if(value < lower) {
      value = lower;
      this->m_ovfl = true;
    }
    else if(value > upper) {
      value = upper;
      this->m_ovfl = true;
    }
    // Report success if no error occurred.
    if(this->m_stat != 0) {
      return *this;
    }
    this->m_succ = true;
    return *this;
  }

    namespace {

#if 0
/* This program is used to generate the multiplier table for decimal
 * numbers. Each multiplier for the mantissa is split into two parts.
 * The base of exponents is 2.
 *
 * Compile with:
 *   gcc -std=c99 -W{all,extra,{sign-,}conversion} table.c -lquadmath
 */

#include <quadmath.h>
#include <stdio.h>

void do_print_one(int e)
  {
    __float128 value, frac;
    int bexp;
    long long mant;

    // Calculate the multiplier.
    value = powq(10, e);
    // Break it down into the fraction and exponent.
    frac = frexpq(value, &bexp);
    // Truncate the fraction to 63 bits. Do not round it.
    frac = ldexpq(frac, 63);
    mant = (long long)frac;
    bexp = bexp + 1;
    // Print the mantissa in fixed-point format.
    printf("\t{ 0x%.16llX, ", mant);
    // Print the exponent in binary.
    printf("%+5d },", bexp);
    // Print some comments.
    printf("  // 1.0e%+.3d\n", e);
  }

int main(void)
  {
    int e;

    for(e = -343; e <= +308; ++e)
      do_print_one(e);

    return 0;
  }
#endif  // 0

    // These are generated data. Do not edit by hand!
    struct decmult_F
      {
        uint64_t mant;
        int bexp;
      }
    constexpr s_decmult_F[] =
      {
        { 0x5F94EE55D417EF57, -1138 },  // 1.0e-343
        { 0x777A29EB491DEB2D, -1135 },  // 1.0e-342
        { 0x4AAC5A330DB2B2FC, -1131 },  // 1.0e-341
        { 0x5D5770BFD11F5FBB, -1128 },  // 1.0e-340
        { 0x74AD4CEFC56737A9, -1125 },  // 1.0e-339
        { 0x48EC5015DB6082CA, -1121 },  // 1.0e-338
        { 0x5B27641B5238A37C, -1118 },  // 1.0e-337
        { 0x71F13D2226C6CC5B, -1115 },  // 1.0e-336
        { 0x4736C635583C3FB9, -1111 },  // 1.0e-335
        { 0x590477C2AE4B4FA7, -1108 },  // 1.0e-334
        { 0x6F4595B359DE2391, -1105 },  // 1.0e-333
        { 0x458B7D90182AD63B, -1101 },  // 1.0e-332
        { 0x56EE5CF41E358BC9, -1098 },  // 1.0e-331
        { 0x6CA9F43125C2EEBC, -1095 },  // 1.0e-330
        { 0x43EA389EB799D535, -1091 },  // 1.0e-329
        { 0x54E4C6C665804A83, -1088 },  // 1.0e-328
        { 0x6A1DF877FEE05D24, -1085 },  // 1.0e-327
        { 0x4252BB4AFF4C3A36, -1081 },  // 1.0e-326
        { 0x52E76A1DBF1F48C4, -1078 },  // 1.0e-325
        { 0x67A144A52EE71AF5, -1075 },  // 1.0e-324
        { 0x40C4CAE73D5070D9, -1071 },  // 1.0e-323
        { 0x50F5FDA10CA48D0F, -1068 },  // 1.0e-322
        { 0x65337D094FCDB053, -1065 },  // 1.0e-321
        { 0x7E805C4BA3C11C68, -1062 },  // 1.0e-320
        { 0x4F1039AF4658B1C1, -1058 },  // 1.0e-319
        { 0x62D4481B17EEDE31, -1055 },  // 1.0e-318
        { 0x7B895A21DDEA95BD, -1052 },  // 1.0e-317
        { 0x4D35D8552AB29D96, -1048 },  // 1.0e-316
        { 0x60834E6A755F44FC, -1045 },  // 1.0e-315
        { 0x78A4220512B7163B, -1042 },  // 1.0e-314
        { 0x4B6695432BB26DE5, -1038 },  // 1.0e-313
        { 0x5E403A93F69F095E, -1035 },  // 1.0e-312
        { 0x75D04938F446CBB5, -1032 },  // 1.0e-311
        { 0x49A22DC398AC3F51, -1028 },  // 1.0e-310
        { 0x5C0AB9347ED74F26, -1025 },  // 1.0e-309
        { 0x730D67819E8D22EF, -1022 },  // 1.0e-308
        { 0x47E860B1031835D5, -1018 },  // 1.0e-307
        { 0x59E278DD43DE434B, -1015 },  // 1.0e-306
        { 0x705B171494D5D41E, -1012 },  // 1.0e-305
        { 0x4638EE6CDD05A492, -1008 },  // 1.0e-304
        { 0x57C72A0814470DB7, -1005 },  // 1.0e-303
        { 0x6DB8F48A1958D125, -1002 },  // 1.0e-302
        { 0x449398D64FD782B7,  -998 },  // 1.0e-301
        { 0x55B87F0BE3CD6365,  -995 },  // 1.0e-300
        { 0x6B269ECEDCC0BC3E,  -992 },  // 1.0e-299
        { 0x42F8234149F875A7,  -988 },  // 1.0e-298
        { 0x53B62C119C769310,  -985 },  // 1.0e-297
        { 0x68A3B716039437D5,  -982 },  // 1.0e-296
        { 0x4166526DC23CA2E5,  -978 },  // 1.0e-295
        { 0x51BFE70932CBCB9E,  -975 },  // 1.0e-294
        { 0x662FE0CB7F7EBE86,  -972 },  // 1.0e-293
        { 0x7FBBD8FE5F5E6E27,  -969 },  // 1.0e-292
        { 0x4FD5679EFB9B04D8,  -965 },  // 1.0e-291
        { 0x63CAC186BA81C60E,  -962 },  // 1.0e-290
        { 0x7CBD71E869223792,  -959 },  // 1.0e-289
        { 0x4DF6673141B562BB,  -955 },  // 1.0e-288
        { 0x617400FD9222BB6A,  -952 },  // 1.0e-287
        { 0x79D1013CF6AB6A45,  -949 },  // 1.0e-286
        { 0x4C22A0C61A2B226B,  -945 },  // 1.0e-285
        { 0x5F2B48F7A0B5EB06,  -942 },  // 1.0e-284
        { 0x76F61B3588E365C7,  -939 },  // 1.0e-283
        { 0x4A59D101758E1F9C,  -935 },  // 1.0e-282
        { 0x5CF04541D2F1A783,  -932 },  // 1.0e-281
        { 0x742C569247AE1164,  -929 },  // 1.0e-280
        { 0x489BB61B6CCCCADF,  -925 },  // 1.0e-279
        { 0x5AC2A3A247FFFD96,  -922 },  // 1.0e-278
        { 0x71734C8AD9FFFCFC,  -919 },  // 1.0e-277
        { 0x46E80FD6C83FFE1D,  -915 },  // 1.0e-276
        { 0x58A213CC7A4FFDA5,  -912 },  // 1.0e-275
        { 0x6ECA98BF98E3FD0E,  -909 },  // 1.0e-274
        { 0x453E9F77BF8E7E29,  -905 },  // 1.0e-273
        { 0x568E4755AF721DB3,  -902 },  // 1.0e-272
        { 0x6C31D92B1B4EA520,  -899 },  // 1.0e-271
        { 0x439F27BAF1112734,  -895 },  // 1.0e-270
        { 0x5486F1A9AD557101,  -892 },  // 1.0e-269
        { 0x69A8AE1418AACD41,  -889 },  // 1.0e-268
        { 0x42096CCC8F6AC048,  -885 },  // 1.0e-267
        { 0x528BC7FFB345705B,  -882 },  // 1.0e-266
        { 0x672EB9FFA016CC71,  -879 },  // 1.0e-265
        { 0x407D343FC40E3FC7,  -875 },  // 1.0e-264
        { 0x509C814FB511CFB9,  -872 },  // 1.0e-263
        { 0x64C3A1A3A25643A7,  -869 },  // 1.0e-262
        { 0x7DF48A0C8AEBD491,  -866 },  // 1.0e-261
        { 0x4EB8D647D6D364DA,  -862 },  // 1.0e-260
        { 0x62670BD9CC883E11,  -859 },  // 1.0e-259
        { 0x7B00CED03FAA4D95,  -856 },  // 1.0e-258
        { 0x4CE0814227CA707D,  -852 },  // 1.0e-257
        { 0x6018A192B1BD0C9C,  -849 },  // 1.0e-256
        { 0x781EC9F75E2C4FC4,  -846 },  // 1.0e-255
        { 0x4B133E3A9ADBB1DA,  -842 },  // 1.0e-254
        { 0x5DD80DC941929E51,  -839 },  // 1.0e-253
        { 0x754E113B91F745E5,  -836 },  // 1.0e-252
        { 0x4950CAC53B3A8BAF,  -832 },  // 1.0e-251
        { 0x5BA4FD768A092E9B,  -829 },  // 1.0e-250
        { 0x728E3CD42C8B7A42,  -826 },  // 1.0e-249
        { 0x4798E6049BD72C69,  -822 },  // 1.0e-248
        { 0x597F1F85C2CCF783,  -819 },  // 1.0e-247
        { 0x6FDEE76733803564,  -816 },  // 1.0e-246
        { 0x45EB50A08030215E,  -812 },  // 1.0e-245
        { 0x576624C8A03C29B6,  -809 },  // 1.0e-244
        { 0x6D3FADFAC84B3424,  -806 },  // 1.0e-243
        { 0x4447CCBCBD2F0096,  -802 },  // 1.0e-242
        { 0x5559BFEBEC7AC0BC,  -799 },  // 1.0e-241
        { 0x6AB02FE6E79970EB,  -796 },  // 1.0e-240
        { 0x42AE1DF050BFE693,  -792 },  // 1.0e-239
        { 0x5359A56C64EFE037,  -789 },  // 1.0e-238
        { 0x68300EC77E2BD845,  -786 },  // 1.0e-237
        { 0x411E093CAEDB672B,  -782 },  // 1.0e-236
        { 0x51658B8BDA9240F6,  -779 },  // 1.0e-235
        { 0x65BEEE6ED136D134,  -776 },  // 1.0e-234
        { 0x7F2EAA0A85848581,  -773 },  // 1.0e-233
        { 0x4F7D2A469372D370,  -769 },  // 1.0e-232
        { 0x635C74D8384F884D,  -766 },  // 1.0e-231
        { 0x7C33920E46636A60,  -763 },  // 1.0e-230
        { 0x4DA03B48EBFE227C,  -759 },  // 1.0e-229
        { 0x61084A1B26FDAB1B,  -756 },  // 1.0e-228
        { 0x794A5CA1F0BD15E2,  -753 },  // 1.0e-227
        { 0x4BCE79E536762DAD,  -749 },  // 1.0e-226
        { 0x5EC2185E8413B918,  -746 },  // 1.0e-225
        { 0x76729E762518A75E,  -743 },  // 1.0e-224
        { 0x4A07A309D72F689B,  -739 },  // 1.0e-223
        { 0x5C898BCC4CFB42C2,  -736 },  // 1.0e-222
        { 0x73ABEEBF603A1372,  -733 },  // 1.0e-221
        { 0x484B75379C244C27,  -729 },  // 1.0e-220
        { 0x5A5E5285832D5F31,  -726 },  // 1.0e-219
        { 0x70F5E726E3F8B6FD,  -723 },  // 1.0e-218
        { 0x4699B0784E7B725E,  -719 },  // 1.0e-217
        { 0x58401C96621A4EF6,  -716 },  // 1.0e-216
        { 0x6E5023BBFAA0E2B3,  -713 },  // 1.0e-215
        { 0x44F216557CA48DB0,  -709 },  // 1.0e-214
        { 0x562E9BEADBCDB11C,  -706 },  // 1.0e-213
        { 0x6BBA42E592C11D63,  -703 },  // 1.0e-212
        { 0x435469CF7BB8B25E,  -699 },  // 1.0e-211
        { 0x542984435AA6DEF5,  -696 },  // 1.0e-210
        { 0x6933E554315096B3,  -693 },  // 1.0e-209
        { 0x41C06F549ED25E30,  -689 },  // 1.0e-208
        { 0x52308B29C686F5BC,  -686 },  // 1.0e-207
        { 0x66BCADF43828B32B,  -683 },  // 1.0e-206
        { 0x4035ECB8A3196FFB,  -679 },  // 1.0e-205
        { 0x504367E6CBDFCBF9,  -676 },  // 1.0e-204
        { 0x645441E07ED7BEF8,  -673 },  // 1.0e-203
        { 0x7D6952589E8DAEB6,  -670 },  // 1.0e-202
        { 0x4E61D37763188D31,  -666 },  // 1.0e-201
        { 0x61FA48553BDEB07E,  -663 },  // 1.0e-200
        { 0x7A78DA6A8AD65C9D,  -660 },  // 1.0e-199
        { 0x4C8B888296C5F9E2,  -656 },  // 1.0e-198
        { 0x5FAE6AA33C77785B,  -653 },  // 1.0e-197
        { 0x779A054C0B955672,  -650 },  // 1.0e-196
        { 0x4AC0434F873D5607,  -646 },  // 1.0e-195
        { 0x5D705423690CAB89,  -643 },  // 1.0e-194
        { 0x74CC692C434FD66B,  -640 },  // 1.0e-193
        { 0x48FFC1BBAA11E603,  -636 },  // 1.0e-192
        { 0x5B3FB22A94965F84,  -633 },  // 1.0e-191
        { 0x720F9EB539BBF765,  -630 },  // 1.0e-190
        { 0x4749C33144157A9F,  -626 },  // 1.0e-189
        { 0x591C33FD951AD946,  -623 },  // 1.0e-188
        { 0x6F6340FCFA618F98,  -620 },  // 1.0e-187
        { 0x459E089E1C7CF9BF,  -616 },  // 1.0e-186
        { 0x57058AC5A39C382F,  -613 },  // 1.0e-185
        { 0x6CC6ED770C83463B,  -610 },  // 1.0e-184
        { 0x43FC546A67D20BE4,  -606 },  // 1.0e-183
        { 0x54FB698501C68EDE,  -603 },  // 1.0e-182
        { 0x6A3A43E642383295,  -600 },  // 1.0e-181
        { 0x42646A6FE9631F9D,  -596 },  // 1.0e-180
        { 0x52FD850BE3BBE784,  -593 },  // 1.0e-179
        { 0x67BCE64EDCAAE166,  -590 },  // 1.0e-178
        { 0x40D60FF149EACCDF,  -586 },  // 1.0e-177
        { 0x510B93ED9C658017,  -583 },  // 1.0e-176
        { 0x654E78E9037EE01D,  -580 },  // 1.0e-175
        { 0x7EA21723445E9825,  -577 },  // 1.0e-174
        { 0x4F254E760ABB1F17,  -573 },  // 1.0e-173
        { 0x62EEA2138D69E6DD,  -570 },  // 1.0e-172
        { 0x7BAA4A9870C46094,  -567 },  // 1.0e-171
        { 0x4D4A6E9F467ABC5C,  -563 },  // 1.0e-170
        { 0x609D0A4718196B73,  -560 },  // 1.0e-169
        { 0x78C44CD8DE1FC650,  -557 },  // 1.0e-168
        { 0x4B7AB0078AD3DBF2,  -553 },  // 1.0e-167
        { 0x5E595C096D88D2EF,  -550 },  // 1.0e-166
        { 0x75EFB30BC8EB07AB,  -547 },  // 1.0e-165
        { 0x49B5CFE75D92E4CA,  -543 },  // 1.0e-164
        { 0x5C2343E134F79DFD,  -540 },  // 1.0e-163
        { 0x732C14D98235857D,  -537 },  // 1.0e-162
        { 0x47FB8D07F161736E,  -533 },  // 1.0e-161
        { 0x59FA7049EDB9D049,  -530 },  // 1.0e-160
        { 0x70790C5C6928445C,  -527 },  // 1.0e-159
        { 0x464BA7B9C1B92AB9,  -523 },  // 1.0e-158
        { 0x57DE91A832277567,  -520 },  // 1.0e-157
        { 0x6DD636123EB152C1,  -517 },  // 1.0e-156
        { 0x44A5E1CB672ED3B9,  -513 },  // 1.0e-155
        { 0x55CF5A3E40FA88A7,  -510 },  // 1.0e-154
        { 0x6B4330CDD1392AD1,  -507 },  // 1.0e-153
        { 0x4309FE80A2C3BAC2,  -503 },  // 1.0e-152
        { 0x53CC7E20CB74A973,  -500 },  // 1.0e-151
        { 0x68BF9DA8FE51D3D0,  -497 },  // 1.0e-150
        { 0x4177C2899EF32462,  -493 },  // 1.0e-149
        { 0x51D5B32C06AFED7A,  -490 },  // 1.0e-148
        { 0x664B1FF7085BE8D9,  -487 },  // 1.0e-147
        { 0x7FDDE7F4CA72E30F,  -484 },  // 1.0e-146
        { 0x4FEAB0F8FE87CDE9,  -480 },  // 1.0e-145
        { 0x63E55D373E29C164,  -477 },  // 1.0e-144
        { 0x7CDEB4850DB431BD,  -474 },  // 1.0e-143
        { 0x4E0B30D328909F16,  -470 },  // 1.0e-142
        { 0x618DFD07F2B4C6DC,  -467 },  // 1.0e-141
        { 0x79F17C49EF61F893,  -464 },  // 1.0e-140
        { 0x4C36EDAE359D3B5B,  -460 },  // 1.0e-139
        { 0x5F44A919C3048A32,  -457 },  // 1.0e-138
        { 0x7715D36033C5ACBF,  -454 },  // 1.0e-137
        { 0x4A6DA41C205B8BF7,  -450 },  // 1.0e-136
        { 0x5D090D2328726EF5,  -447 },  // 1.0e-135
        { 0x744B506BF28F0AB3,  -444 },  // 1.0e-134
        { 0x48AF1243779966B0,  -440 },  // 1.0e-133
        { 0x5ADAD6D4557FC05C,  -437 },  // 1.0e-132
        { 0x71918C896ADFB073,  -434 },  // 1.0e-131
        { 0x46FAF7D5E2CBCE47,  -430 },  // 1.0e-130
        { 0x58B9B5CB5B7EC1D9,  -427 },  // 1.0e-129
        { 0x6EE8233E325E7250,  -424 },  // 1.0e-128
        { 0x45511606DF7B0772,  -420 },  // 1.0e-127
        { 0x56A55B889759C94E,  -417 },  // 1.0e-126
        { 0x6C4EB26ABD303BA2,  -414 },  // 1.0e-125
        { 0x43B12F82B63E2545,  -410 },  // 1.0e-124
        { 0x549D7B6363CDAE96,  -407 },  // 1.0e-123
        { 0x69C4DA3C3CC11A3C,  -404 },  // 1.0e-122
        { 0x421B0865A5F8B065,  -400 },  // 1.0e-121
        { 0x52A1CA7F0F76DC7F,  -397 },  // 1.0e-120
        { 0x674A3D1ED354939F,  -394 },  // 1.0e-119
        { 0x408E66334414DC43,  -390 },  // 1.0e-118
        { 0x50B1FFC0151A1354,  -387 },  // 1.0e-117
        { 0x64DE7FB01A609829,  -384 },  // 1.0e-116
        { 0x7E161F9C20F8BE33,  -381 },  // 1.0e-115
        { 0x4ECDD3C1949B76E0,  -377 },  // 1.0e-114
        { 0x628148B1F9C25498,  -374 },  // 1.0e-113
        { 0x7B219ADE7832E9BE,  -371 },  // 1.0e-112
        { 0x4CF500CB0B1FD217,  -367 },  // 1.0e-111
        { 0x603240FDCDE7C69C,  -364 },  // 1.0e-110
        { 0x783ED13D4161B844,  -361 },  // 1.0e-109
        { 0x4B2742C648DD132A,  -357 },  // 1.0e-108
        { 0x5DF11377DB1457F5,  -354 },  // 1.0e-107
        { 0x756D5855D1D96DF2,  -351 },  // 1.0e-106
        { 0x49645735A327E4B7,  -347 },  // 1.0e-105
        { 0x5BBD6D030BF1DDE5,  -344 },  // 1.0e-104
        { 0x72ACC843CEEE555E,  -341 },  // 1.0e-103
        { 0x47ABFD2A6154F55B,  -337 },  // 1.0e-102
        { 0x5996FC74F9AA32B2,  -334 },  // 1.0e-101
        { 0x6FFCBB923814BF5E,  -331 },  // 1.0e-100
        { 0x45FDF53B630CF79B,  -327 },  // 1.0e-099
        { 0x577D728A3BD03581,  -324 },  // 1.0e-098
        { 0x6D5CCF2CCAC442E2,  -321 },  // 1.0e-097
        { 0x445A017BFEBAA9CD,  -317 },  // 1.0e-096
        { 0x557081DAFE695440,  -314 },  // 1.0e-095
        { 0x6ACCA251BE03A951,  -311 },  // 1.0e-094
        { 0x42BFE57316C249D2,  -307 },  // 1.0e-093
        { 0x536FDECFDC72DC47,  -304 },  // 1.0e-092
        { 0x684BD683D38F9359,  -301 },  // 1.0e-091
        { 0x412F66126439BC17,  -297 },  // 1.0e-090
        { 0x517B3F96FD482B1D,  -294 },  // 1.0e-089
        { 0x65DA0F7CBC9A35E5,  -291 },  // 1.0e-088
        { 0x7F50935BEBC0C35E,  -288 },  // 1.0e-087
        { 0x4F925C1973587A1B,  -284 },  // 1.0e-086
        { 0x6376F31FD02E98A1,  -281 },  // 1.0e-085
        { 0x7C54AFE7C43A3ECA,  -278 },  // 1.0e-084
        { 0x4DB4EDF0DAA4673E,  -274 },  // 1.0e-083
        { 0x6122296D114D810D,  -271 },  // 1.0e-082
        { 0x796AB3C855A0E151,  -268 },  // 1.0e-081
        { 0x4BE2B05D35848CD2,  -264 },  // 1.0e-080
        { 0x5EDB5C7482E5B007,  -261 },  // 1.0e-079
        { 0x76923391A39F1C09,  -258 },  // 1.0e-078
        { 0x4A1B603B06437185,  -254 },  // 1.0e-077
        { 0x5CA23849C7D44DE7,  -251 },  // 1.0e-076
        { 0x73CAC65C39C96161,  -248 },  // 1.0e-075
        { 0x485EBBF9A41DDCDC,  -244 },  // 1.0e-074
        { 0x5A766AF80D255414,  -241 },  // 1.0e-073
        { 0x711405B6106EA919,  -238 },  // 1.0e-072
        { 0x46AC8391CA4529AF,  -234 },  // 1.0e-071
        { 0x5857A4763CD6741B,  -231 },  // 1.0e-070
        { 0x6E6D8D93CC0C1122,  -228 },  // 1.0e-069
        { 0x4504787C5F878AB5,  -224 },  // 1.0e-068
        { 0x5645969B77696D62,  -221 },  // 1.0e-067
        { 0x6BD6FC425543C8BB,  -218 },  // 1.0e-066
        { 0x43665DA9754A5D75,  -214 },  // 1.0e-065
        { 0x543FF513D29CF4D2,  -211 },  // 1.0e-064
        { 0x694FF258C7443207,  -208 },  // 1.0e-063
        { 0x41D1F7777C8A9F44,  -204 },  // 1.0e-062
        { 0x524675555BAD4715,  -201 },  // 1.0e-061
        { 0x66D812AAB29898DB,  -198 },  // 1.0e-060
        { 0x40470BAAAF9F5F88,  -194 },  // 1.0e-059
        { 0x5058CE955B87376B,  -191 },  // 1.0e-058
        { 0x646F023AB2690545,  -188 },  // 1.0e-057
        { 0x7D8AC2C95F034697,  -185 },  // 1.0e-056
        { 0x4E76B9BDDB620C1E,  -181 },  // 1.0e-055
        { 0x6214682D523A8F26,  -178 },  // 1.0e-054
        { 0x7A998238A6C932EF,  -175 },  // 1.0e-053
        { 0x4C9FF163683DBFD5,  -171 },  // 1.0e-052
        { 0x5FC7EDBC424D2FCB,  -168 },  // 1.0e-051
        { 0x77B9E92B52E07BBE,  -165 },  // 1.0e-050
        { 0x4AD431BB13CC4D56,  -161 },  // 1.0e-049
        { 0x5D893E29D8BF60AC,  -158 },  // 1.0e-048
        { 0x74EB8DB44EEF38D7,  -155 },  // 1.0e-047
        { 0x49133890B1558386,  -151 },  // 1.0e-046
        { 0x5B5806B4DDAAE468,  -148 },  // 1.0e-045
        { 0x722E086215159D82,  -145 },  // 1.0e-044
        { 0x475CC53D4D2D8271,  -141 },  // 1.0e-043
        { 0x5933F68CA078E30E,  -138 },  // 1.0e-042
        { 0x6F80F42FC8971BD1,  -135 },  // 1.0e-041
        { 0x45B0989DDD5E7163,  -131 },  // 1.0e-040
        { 0x571CBEC554B60DBB,  -128 },  // 1.0e-039
        { 0x6CE3EE76A9E3912A,  -125 },  // 1.0e-038
        { 0x440E750A2A2E3ABA,  -121 },  // 1.0e-037
        { 0x5512124CB4B9C969,  -118 },  // 1.0e-036
        { 0x6A5696DFE1E83BC3,  -115 },  // 1.0e-035
        { 0x42761E4BED31255A,  -111 },  // 1.0e-034
        { 0x5313A5DEE87D6EB0,  -108 },  // 1.0e-033
        { 0x67D88F56A29CCA5D,  -105 },  // 1.0e-032
        { 0x40E7599625A1FE7A,  -101 },  // 1.0e-031
        { 0x51212FFBAF0A7E18,   -98 },  // 1.0e-030
        { 0x65697BFA9ACD1D9F,   -95 },  // 1.0e-029
        { 0x7EC3DAF941806506,   -92 },  // 1.0e-028
        { 0x4F3A68DBC8F03F24,   -88 },  // 1.0e-027
        { 0x63090312BB2C4EED,   -85 },  // 1.0e-026
        { 0x7BCB43D769F762A8,   -82 },  // 1.0e-025
        { 0x4D5F0A66A23A9DA9,   -78 },  // 1.0e-024
        { 0x60B6CD004AC94513,   -75 },  // 1.0e-023
        { 0x78E480405D7B9658,   -72 },  // 1.0e-022
        { 0x4B8ED0283A6D3DF7,   -68 },  // 1.0e-021
        { 0x5E72843249088D75,   -65 },  // 1.0e-020
        { 0x760F253EDB4AB0D2,   -62 },  // 1.0e-019
        { 0x49C97747490EAE83,   -58 },  // 1.0e-018
        { 0x5C3BD5191B525A24,   -55 },  // 1.0e-017
        { 0x734ACA5F6226F0AD,   -52 },  // 1.0e-016
        { 0x480EBE7B9D58566C,   -48 },  // 1.0e-015
        { 0x5A126E1A84AE6C07,   -45 },  // 1.0e-014
        { 0x709709A125DA0709,   -42 },  // 1.0e-013
        { 0x465E6604B7A84465,   -38 },  // 1.0e-012
        { 0x57F5FF85E592557F,   -35 },  // 1.0e-011
        { 0x6DF37F675EF6EADF,   -32 },  // 1.0e-010
        { 0x44B82FA09B5A52CB,   -28 },  // 1.0e-009
        { 0x55E63B88C230E77E,   -25 },  // 1.0e-008
        { 0x6B5FCA6AF2BD215E,   -22 },  // 1.0e-007
        { 0x431BDE82D7B634DA,   -18 },  // 1.0e-006
        { 0x53E2D6238DA3C211,   -15 },  // 1.0e-005
        { 0x68DB8BAC710CB295,   -12 },  // 1.0e-004
        { 0x4189374BC6A7EF9D,    -8 },  // 1.0e-003
        { 0x51EB851EB851EB85,    -5 },  // 1.0e-002
        { 0x6666666666666666,    -2 },  // 1.0e-001
        { 0x4000000000000000,    +2 },  // 1.0e+000
        { 0x5000000000000000,    +5 },  // 1.0e+001
        { 0x6400000000000000,    +8 },  // 1.0e+002
        { 0x7D00000000000000,   +11 },  // 1.0e+003
        { 0x4E20000000000000,   +15 },  // 1.0e+004
        { 0x61A8000000000000,   +18 },  // 1.0e+005
        { 0x7A12000000000000,   +21 },  // 1.0e+006
        { 0x4C4B400000000000,   +25 },  // 1.0e+007
        { 0x5F5E100000000000,   +28 },  // 1.0e+008
        { 0x7735940000000000,   +31 },  // 1.0e+009
        { 0x4A817C8000000000,   +35 },  // 1.0e+010
        { 0x5D21DBA000000000,   +38 },  // 1.0e+011
        { 0x746A528800000000,   +41 },  // 1.0e+012
        { 0x48C2739500000000,   +45 },  // 1.0e+013
        { 0x5AF3107A40000000,   +48 },  // 1.0e+014
        { 0x71AFD498D0000000,   +51 },  // 1.0e+015
        { 0x470DE4DF82000000,   +55 },  // 1.0e+016
        { 0x58D15E1762800000,   +58 },  // 1.0e+017
        { 0x6F05B59D3B200000,   +61 },  // 1.0e+018
        { 0x4563918244F40000,   +65 },  // 1.0e+019
        { 0x56BC75E2D6310000,   +68 },  // 1.0e+020
        { 0x6C6B935B8BBD4000,   +71 },  // 1.0e+021
        { 0x43C33C1937564800,   +75 },  // 1.0e+022
        { 0x54B40B1F852BDA00,   +78 },  // 1.0e+023
        { 0x69E10DE76676D080,   +81 },  // 1.0e+024
        { 0x422CA8B0A00A4250,   +85 },  // 1.0e+025
        { 0x52B7D2DCC80CD2E4,   +88 },  // 1.0e+026
        { 0x6765C793FA10079D,   +91 },  // 1.0e+027
        { 0x409F9CBC7C4A04C2,   +95 },  // 1.0e+028
        { 0x50C783EB9B5C85F2,   +98 },  // 1.0e+029
        { 0x64F964E68233A76F,  +101 },  // 1.0e+030
        { 0x7E37BE2022C0914B,  +104 },  // 1.0e+031
        { 0x4EE2D6D415B85ACE,  +108 },  // 1.0e+032
        { 0x629B8C891B267182,  +111 },  // 1.0e+033
        { 0x7B426FAB61F00DE3,  +114 },  // 1.0e+034
        { 0x4D0985CB1D3608AE,  +118 },  // 1.0e+035
        { 0x604BE73DE4838AD9,  +121 },  // 1.0e+036
        { 0x785EE10D5DA46D90,  +124 },  // 1.0e+037
        { 0x4B3B4CA85A86C47A,  +128 },  // 1.0e+038
        { 0x5E0A1FD271287598,  +131 },  // 1.0e+039
        { 0x758CA7C70D7292FE,  +134 },  // 1.0e+040
        { 0x4977E8DC68679BDF,  +138 },  // 1.0e+041
        { 0x5BD5E313828182D6,  +141 },  // 1.0e+042
        { 0x72CB5BD86321E38C,  +144 },  // 1.0e+043
        { 0x47BF19673DF52E37,  +148 },  // 1.0e+044
        { 0x59AEDFC10D7279C5,  +151 },  // 1.0e+045
        { 0x701A97B150CF1837,  +154 },  // 1.0e+046
        { 0x46109ECED2816F22,  +158 },  // 1.0e+047
        { 0x5794C6828721CAEB,  +161 },  // 1.0e+048
        { 0x6D79F82328EA3DA6,  +164 },  // 1.0e+049
        { 0x446C3B15F9926687,  +168 },  // 1.0e+050
        { 0x558749DB77F70029,  +171 },  // 1.0e+051
        { 0x6AE91C5255F4C034,  +174 },  // 1.0e+052
        { 0x42D1B1B375B8F820,  +178 },  // 1.0e+053
        { 0x53861E2053273628,  +181 },  // 1.0e+054
        { 0x6867A5A867F103B2,  +184 },  // 1.0e+055
        { 0x4140C78940F6A24F,  +188 },  // 1.0e+056
        { 0x5190F96B91344AE3,  +191 },  // 1.0e+057
        { 0x65F537C675815D9C,  +194 },  // 1.0e+058
        { 0x7F7285B812E1B504,  +197 },  // 1.0e+059
        { 0x4FA793930BCD1122,  +201 },  // 1.0e+060
        { 0x63917877CEC0556B,  +204 },  // 1.0e+061
        { 0x7C75D695C2706AC5,  +207 },  // 1.0e+062
        { 0x4DC9A61D998642BB,  +211 },  // 1.0e+063
        { 0x613C0FA4FFE7D36A,  +214 },  // 1.0e+064
        { 0x798B138E3FE1C845,  +217 },  // 1.0e+065
        { 0x4BF6EC38E7ED1D2B,  +221 },  // 1.0e+066
        { 0x5EF4A74721E86476,  +224 },  // 1.0e+067
        { 0x76B1D118EA627D93,  +227 },  // 1.0e+068
        { 0x4A2F22AF927D8E7C,  +231 },  // 1.0e+069
        { 0x5CBAEB5B771CF21B,  +234 },  // 1.0e+070
        { 0x73E9A63254E42EA2,  +237 },  // 1.0e+071
        { 0x487207DF750E9D25,  +241 },  // 1.0e+072
        { 0x5A8E89D75252446E,  +244 },  // 1.0e+073
        { 0x71322C4D26E6D58A,  +247 },  // 1.0e+074
        { 0x46BF5BB038504576,  +251 },  // 1.0e+075
        { 0x586F329C466456D4,  +254 },  // 1.0e+076
        { 0x6E8AFF4357FD6C89,  +257 },  // 1.0e+077
        { 0x4516DF8A16FE63D5,  +261 },  // 1.0e+078
        { 0x565C976C9CBDFCCB,  +264 },  // 1.0e+079
        { 0x6BF3BD47C3ED7BFD,  +267 },  // 1.0e+080
        { 0x4378564CDA746D7E,  +271 },  // 1.0e+081
        { 0x54566BE0111188DE,  +274 },  // 1.0e+082
        { 0x696C06D81555EB15,  +277 },  // 1.0e+083
        { 0x41E384470D55B2ED,  +281 },  // 1.0e+084
        { 0x525C6558D0AB1FA9,  +284 },  // 1.0e+085
        { 0x66F37EAF04D5E793,  +287 },  // 1.0e+086
        { 0x40582F2D6305B0BC,  +291 },  // 1.0e+087
        { 0x506E3AF8BBC71CEB,  +294 },  // 1.0e+088
        { 0x6489C9B6EAB8E426,  +297 },  // 1.0e+089
        { 0x7DAC3C24A5671D2F,  +300 },  // 1.0e+090
        { 0x4E8BA596E760723D,  +304 },  // 1.0e+091
        { 0x622E8EFCA1388ECD,  +307 },  // 1.0e+092
        { 0x7ABA32BBC986B280,  +310 },  // 1.0e+093
        { 0x4CB45FB55DF42F90,  +314 },  // 1.0e+094
        { 0x5FE177A2B5713B74,  +317 },  // 1.0e+095
        { 0x77D9D58B62CD8A51,  +320 },  // 1.0e+096
        { 0x4AE825771DC07672,  +324 },  // 1.0e+097
        { 0x5DA22ED4E530940F,  +327 },  // 1.0e+098
        { 0x750ABA8A1E7CB913,  +330 },  // 1.0e+099
        { 0x4926B496530DF3AC,  +334 },  // 1.0e+100
        { 0x5B7061BBE7D17097,  +337 },  // 1.0e+101
        { 0x724C7A2AE1C5CCBD,  +340 },  // 1.0e+102
        { 0x476FCC5ACD1B9FF6,  +344 },  // 1.0e+103
        { 0x594BBF71806287F3,  +347 },  // 1.0e+104
        { 0x6F9EAF4DE07B29F0,  +350 },  // 1.0e+105
        { 0x45C32D90AC4CFA36,  +354 },  // 1.0e+106
        { 0x5733F8F4D76038C3,  +357 },  // 1.0e+107
        { 0x6D00F7320D3846F4,  +360 },  // 1.0e+108
        { 0x44209A7F48432C59,  +364 },  // 1.0e+109
        { 0x5528C11F1A53F76F,  +367 },  // 1.0e+110
        { 0x6A72F166E0E8F54B,  +370 },  // 1.0e+111
        { 0x4287D6E04C91994F,  +374 },  // 1.0e+112
        { 0x5329CC985FB5FFA2,  +377 },  // 1.0e+113
        { 0x67F43FBE77A37F8B,  +380 },  // 1.0e+114
        { 0x40F8A7D70AC62FB7,  +384 },  // 1.0e+115
        { 0x5136D1CCCD77BBA4,  +387 },  // 1.0e+116
        { 0x6584864000D5AA8E,  +390 },  // 1.0e+117
        { 0x7EE5A7D0010B1531,  +393 },  // 1.0e+118
        { 0x4F4F88E200A6ED3F,  +397 },  // 1.0e+119
        { 0x63236B1A80D0A88E,  +400 },  // 1.0e+120
        { 0x7BEC45E12104D2B2,  +403 },  // 1.0e+121
        { 0x4D73ABACB4A303AF,  +407 },  // 1.0e+122
        { 0x60D09697E1CBC49B,  +410 },  // 1.0e+123
        { 0x7904BC3DDA3EB5C2,  +413 },  // 1.0e+124
        { 0x4BA2F5A6A8673199,  +417 },  // 1.0e+125
        { 0x5E8BB3105280FDFF,  +420 },  // 1.0e+126
        { 0x762E9FD467213D7F,  +423 },  // 1.0e+127
        { 0x49DD23E4C074C66F,  +427 },  // 1.0e+128
        { 0x5C546CDDF091F80B,  +430 },  // 1.0e+129
        { 0x736988156CB6760E,  +433 },  // 1.0e+130
        { 0x4821F50D63F209C9,  +437 },  // 1.0e+131
        { 0x5A2A7250BCEE8C3B,  +440 },  // 1.0e+132
        { 0x70B50EE4EC2A2F4A,  +443 },  // 1.0e+133
        { 0x4671294F139A5D8E,  +447 },  // 1.0e+134
        { 0x580D73A2D880F4F2,  +450 },  // 1.0e+135
        { 0x6E10D08B8EA1322E,  +453 },  // 1.0e+136
        { 0x44CA82573924BF5D,  +457 },  // 1.0e+137
        { 0x55FD22ED076DEF34,  +460 },  // 1.0e+138
        { 0x6B7C6BA849496B01,  +463 },  // 1.0e+139
        { 0x432DC3492DCDE2E1,  +467 },  // 1.0e+140
        { 0x53F9341B79415B99,  +470 },  // 1.0e+141
        { 0x68F781225791B27F,  +473 },  // 1.0e+142
        { 0x419AB0B576BB0F8F,  +477 },  // 1.0e+143
        { 0x52015CE2D469D373,  +480 },  // 1.0e+144
        { 0x6681B41B89844850,  +483 },  // 1.0e+145
        { 0x4011109135F2AD32,  +487 },  // 1.0e+146
        { 0x501554B5836F587E,  +490 },  // 1.0e+147
        { 0x641AA9E2E44B2E9E,  +493 },  // 1.0e+148
        { 0x7D21545B9D5DFA46,  +496 },  // 1.0e+149
        { 0x4E34D4B9425ABC6B,  +500 },  // 1.0e+150
        { 0x61C209E792F16B86,  +503 },  // 1.0e+151
        { 0x7A328C6177ADC668,  +506 },  // 1.0e+152
        { 0x4C5F97BCEACC9C01,  +510 },  // 1.0e+153
        { 0x5F777DAC257FC301,  +513 },  // 1.0e+154
        { 0x77555D172EDFB3C2,  +516 },  // 1.0e+155
        { 0x4A955A2E7D4BD059,  +520 },  // 1.0e+156
        { 0x5D3AB0BA1C9EC46F,  +523 },  // 1.0e+157
        { 0x74895CE8A3C6758B,  +526 },  // 1.0e+158
        { 0x48D5DA11665C0977,  +530 },  // 1.0e+159
        { 0x5B0B5095BFF30BD5,  +533 },  // 1.0e+160
        { 0x71CE24BB2FEFCECA,  +536 },  // 1.0e+161
        { 0x4720D6F4FDF5E13E,  +540 },  // 1.0e+162
        { 0x58E90CB23D73598E,  +543 },  // 1.0e+163
        { 0x6F234FDECCD02FF1,  +546 },  // 1.0e+164
        { 0x457611EB40021DF7,  +550 },  // 1.0e+165
        { 0x56D396661002A574,  +553 },  // 1.0e+166
        { 0x6C887BFF94034ED2,  +556 },  // 1.0e+167
        { 0x43D54D7FBC821143,  +560 },  // 1.0e+168
        { 0x54CAA0DFABA29594,  +563 },  // 1.0e+169
        { 0x69FD4917968B3AF9,  +566 },  // 1.0e+170
        { 0x423E4DAEBE1704DB,  +570 },  // 1.0e+171
        { 0x52CDE11A6D9CC612,  +573 },  // 1.0e+172
        { 0x678159610903F797,  +576 },  // 1.0e+173
        { 0x40B0D7DCA5A27ABE,  +580 },  // 1.0e+174
        { 0x50DD0DD3CF0B196E,  +583 },  // 1.0e+175
        { 0x65145148C2CDDFC9,  +586 },  // 1.0e+176
        { 0x7E59659AF38157BC,  +589 },  // 1.0e+177
        { 0x4EF7DF80D830D6D5,  +593 },  // 1.0e+178
        { 0x62B5D7610E3D0C8B,  +596 },  // 1.0e+179
        { 0x7B634D3951CC4FAD,  +599 },  // 1.0e+180
        { 0x4D1E1043D31FB1CC,  +603 },  // 1.0e+181
        { 0x60659454C7E79E3F,  +606 },  // 1.0e+182
        { 0x787EF969F9E185CF,  +609 },  // 1.0e+183
        { 0x4B4F5BE23C2CF3A1,  +613 },  // 1.0e+184
        { 0x5E2332DACB38308A,  +616 },  // 1.0e+185
        { 0x75ABFF917E063CAC,  +619 },  // 1.0e+186
        { 0x498B7FBAEEC3E5EC,  +623 },  // 1.0e+187
        { 0x5BEE5FA9AA74DF67,  +626 },  // 1.0e+188
        { 0x72E9F79415121740,  +629 },  // 1.0e+189
        { 0x47D23ABC8D2B4E88,  +633 },  // 1.0e+190
        { 0x59C6C96BB076222A,  +636 },  // 1.0e+191
        { 0x70387BC69C93AAB5,  +639 },  // 1.0e+192
        { 0x46234D5C21DC4AB1,  +643 },  // 1.0e+193
        { 0x57AC20B32A535D5D,  +646 },  // 1.0e+194
        { 0x6D9728DFF4E834B5,  +649 },  // 1.0e+195
        { 0x447E798BF91120F1,  +653 },  // 1.0e+196
        { 0x559E17EEF755692D,  +656 },  // 1.0e+197
        { 0x6B059DEAB52AC378,  +659 },  // 1.0e+198
        { 0x42E382B2B13ABA2B,  +663 },  // 1.0e+199
        { 0x539C635F5D8968B6,  +666 },  // 1.0e+200
        { 0x68837C3734EBC2E3,  +669 },  // 1.0e+201
        { 0x41522DA2811359CE,  +673 },  // 1.0e+202
        { 0x51A6B90B21583042,  +676 },  // 1.0e+203
        { 0x6610674DE9AE3C52,  +679 },  // 1.0e+204
        { 0x7F9481216419CB67,  +682 },  // 1.0e+205
        { 0x4FBCD0B4DE901F20,  +686 },  // 1.0e+206
        { 0x63AC04E2163426E8,  +689 },  // 1.0e+207
        { 0x7C97061A9BC130A2,  +692 },  // 1.0e+208
        { 0x4DDE63D0A158BE65,  +696 },  // 1.0e+209
        { 0x6155FCC4C9AEEDFF,  +699 },  // 1.0e+210
        { 0x79AB7BF5FC1AA97F,  +702 },  // 1.0e+211
        { 0x4C0B2D79BD90A9EF,  +706 },  // 1.0e+212
        { 0x5F0DF8D82CF4D46B,  +709 },  // 1.0e+213
        { 0x76D1770E38320986,  +712 },  // 1.0e+214
        { 0x4A42EA68E31F45F3,  +716 },  // 1.0e+215
        { 0x5CD3A5031BE71770,  +719 },  // 1.0e+216
        { 0x74088E43E2E0DD4C,  +722 },  // 1.0e+217
        { 0x488558EA6DCC8A50,  +726 },  // 1.0e+218
        { 0x5AA6AF25093FACE4,  +729 },  // 1.0e+219
        { 0x71505AEE4B8F981D,  +732 },  // 1.0e+220
        { 0x46D238D4EF39BF12,  +736 },  // 1.0e+221
        { 0x5886C70A2B082ED6,  +739 },  // 1.0e+222
        { 0x6EA878CCB5CA3A8C,  +742 },  // 1.0e+223
        { 0x45294B7FF19E6497,  +746 },  // 1.0e+224
        { 0x56739E5FEE05FDBD,  +749 },  // 1.0e+225
        { 0x6C1085F7E9877D2D,  +752 },  // 1.0e+226
        { 0x438A53BAF1F4AE3C,  +756 },  // 1.0e+227
        { 0x546CE8A9AE71D9CB,  +759 },  // 1.0e+228
        { 0x698822D41A0E503E,  +762 },  // 1.0e+229
        { 0x41F515C49048F226,  +766 },  // 1.0e+230
        { 0x52725B35B45B2EB0,  +769 },  // 1.0e+231
        { 0x670EF2032171FA5C,  +772 },  // 1.0e+232
        { 0x40695741F4E73C79,  +776 },  // 1.0e+233
        { 0x5083AD1272210B98,  +779 },  // 1.0e+234
        { 0x64A498570EA94E7E,  +782 },  // 1.0e+235
        { 0x7DCDBE6CD253A21E,  +785 },  // 1.0e+236
        { 0x4EA0970403744552,  +789 },  // 1.0e+237
        { 0x6248BCC5045156A7,  +792 },  // 1.0e+238
        { 0x7ADAEBF64565AC51,  +795 },  // 1.0e+239
        { 0x4CC8D379EB5F8BB2,  +799 },  // 1.0e+240
        { 0x5FFB085866376E9F,  +802 },  // 1.0e+241
        { 0x77F9CA6E7FC54A47,  +805 },  // 1.0e+242
        { 0x4AFC1E850FDB4E6C,  +809 },  // 1.0e+243
        { 0x5DBB262653D22207,  +812 },  // 1.0e+244
        { 0x7529EFAFE8C6AA89,  +815 },  // 1.0e+245
        { 0x493A35CDF17C2A96,  +819 },  // 1.0e+246
        { 0x5B88C3416DDB353B,  +822 },  // 1.0e+247
        { 0x726AF411C952028A,  +825 },  // 1.0e+248
        { 0x4782D88B1DD34196,  +829 },  // 1.0e+249
        { 0x59638EADE54811FC,  +832 },  // 1.0e+250
        { 0x6FBC72595E9A167B,  +835 },  // 1.0e+251
        { 0x45D5C777DB204E0D,  +839 },  // 1.0e+252
        { 0x574B3955D1E86190,  +842 },  // 1.0e+253
        { 0x6D1E07AB466279F4,  +845 },  // 1.0e+254
        { 0x4432C4CB0BFD8C38,  +849 },  // 1.0e+255
        { 0x553F75FDCEFCEF46,  +852 },  // 1.0e+256
        { 0x6A8F537D42BC2B18,  +855 },  // 1.0e+257
        { 0x4299942E49B59AEF,  +859 },  // 1.0e+258
        { 0x533FF939DC2301AB,  +862 },  // 1.0e+259
        { 0x680FF788532BC216,  +865 },  // 1.0e+260
        { 0x4109FAB533FB594D,  +869 },  // 1.0e+261
        { 0x514C796280FA2FA1,  +872 },  // 1.0e+262
        { 0x659F97BB2138BB89,  +875 },  // 1.0e+263
        { 0x7F077DA9E986EA6B,  +878 },  // 1.0e+264
        { 0x4F64AE8A31F45283,  +882 },  // 1.0e+265
        { 0x633DDA2CBE716724,  +885 },  // 1.0e+266
        { 0x7C0D50B7EE0DC0ED,  +888 },  // 1.0e+267
        { 0x4D885272F4C89894,  +892 },  // 1.0e+268
        { 0x60EA670FB1FABEB9,  +895 },  // 1.0e+269
        { 0x792500D39E796E67,  +898 },  // 1.0e+270
        { 0x4BB72084430BE500,  +902 },  // 1.0e+271
        { 0x5EA4E8A553CEDE41,  +905 },  // 1.0e+272
        { 0x764E22CEA8C295D1,  +908 },  // 1.0e+273
        { 0x49F0D5C129799DA2,  +912 },  // 1.0e+274
        { 0x5C6D0B3173D8050B,  +915 },  // 1.0e+275
        { 0x73884DFDD0CE064E,  +918 },  // 1.0e+276
        { 0x483530BEA280C3F1,  +922 },  // 1.0e+277
        { 0x5A427CEE4B20F4ED,  +925 },  // 1.0e+278
        { 0x70D31C29DDE93228,  +928 },  // 1.0e+279
        { 0x4683F19A2AB1BF59,  +932 },  // 1.0e+280
        { 0x5824EE00B55E2F2F,  +935 },  // 1.0e+281
        { 0x6E2E2980E2B5BAFB,  +938 },  // 1.0e+282
        { 0x44DCD9F08DB194DD,  +942 },  // 1.0e+283
        { 0x5614106CB11DFA14,  +945 },  // 1.0e+284
        { 0x6B991487DD657899,  +948 },  // 1.0e+285
        { 0x433FACD4EA5F6B60,  +952 },  // 1.0e+286
        { 0x540F980A24F74638,  +955 },  // 1.0e+287
        { 0x69137E0CAE3517C6,  +958 },  // 1.0e+288
        { 0x41AC2EC7ECE12EDB,  +962 },  // 1.0e+289
        { 0x52173A79E8197A92,  +965 },  // 1.0e+290
        { 0x669D0918621FD937,  +968 },  // 1.0e+291
        { 0x402225AF3D53E7C2,  +972 },  // 1.0e+292
        { 0x502AAF1B0CA8E1B3,  +975 },  // 1.0e+293
        { 0x64355AE1CFD31A20,  +978 },  // 1.0e+294
        { 0x7D42B19A43C7E0A8,  +981 },  // 1.0e+295
        { 0x4E49AF006A5CEC69,  +985 },  // 1.0e+296
        { 0x61DC1AC084F42783,  +988 },  // 1.0e+297
        { 0x7A532170A6313164,  +991 },  // 1.0e+298
        { 0x4C73F4E667DEBEDE,  +995 },  // 1.0e+299
        { 0x5F90F22001D66E96,  +998 },  // 1.0e+300
        { 0x77752EA8024C0A3C, +1001 },  // 1.0e+301
        { 0x4AA93D29016F8665, +1005 },  // 1.0e+302
        { 0x5D538C7341CB67FE, +1008 },  // 1.0e+303
        { 0x74A86F90123E41FE, +1011 },  // 1.0e+304
        { 0x48E945BA0B66E93F, +1015 },  // 1.0e+305
        { 0x5B2397288E40A38E, +1018 },  // 1.0e+306
        { 0x71EC7CF2B1D0CC72, +1021 },  // 1.0e+307
        { 0x4733CE17AF227FC7, +1025 },  // 1.0e+308
      };
    static_assert(noadl::countof(s_decmult_F) == 652, "");

    }

ascii_numget& ascii_numget::cast_F(double& value, double lower, double upper) noexcept
  {
    this->m_stat = 0;
    // Store the sign bit into a `double`.
    double sign = -(this->m_sign);
    // Try casting the value.
    switch(this->m_vcls) {
    case 0:  // finite
      {
        uint64_t ireg = this->m_mant;
        if(ireg == 0) {
          // The value is effectively zero.
          value = ::std::copysign(0.0, sign);
          break;
        }
        // Get the base.
        uint8_t base = 2;
        if(this->m_erdx) {
          base = this->m_base;
        }
        // Raise the mantissa accordingly.
        double freg;
        switch(base) {
        case 2:
          {
            // Convert the mantissa to a floating-point number. The result is exact.
            if(ireg >> 62) {
              // Drop two bits from the right.
              ireg = (ireg >> 2) | (((ireg >> 1) | ireg) & 1) | this->m_madd;
              freg = static_cast<double>(static_cast<int64_t>(ireg));
              freg = ::std::ldexp(freg, this->m_expo + 2);
            }
            else {
              // Shift overflowed digits into the right.
              ireg = (ireg << 1) | this->m_madd;
              freg = static_cast<double>(static_cast<int64_t>(ireg));
              freg = ::std::ldexp(freg, this->m_expo - 1);
            }
            break;
          }
        case 10:
          {
            // Get the multiplier.
            if(this->m_expo < -343) {
              freg = 0;
              break;
            }
            if(this->m_expo > +308) {
              freg = numeric_limits<double>::infinity();
              break;
            }
            const auto& mult = s_decmult_F[static_cast<uint32_t>(this->m_expo + 343)];
            // Adjust `ireg` such that its MSB is non-zero.
            // TODO: Modern CPUs have intrinsics for LZCNT.
            int32_t lzcnt = 0;
            for(int i = 32; i != 0; i /= 2) {
              if(ireg >> (64 - i))
                continue;
              ireg <<= i;
              lzcnt += i;
            }
            // Multiply two 64-bit values and get the high-order half.
            // TODO: Modern CPUs have intrinsics for this.
            uint64_t xhi = ireg >> 32;
            uint64_t xlo = ireg & UINT32_MAX;
            uint64_t yhi = mult.mant >> 32;
            uint64_t ylo = mult.mant & UINT32_MAX;
            ireg = xhi * yhi + (xlo * yhi >> 32) + (xhi * ylo >> 32);
            // Convert the mantissa to a floating-point number.
            // Set the LSB in case of rounding.
            ROCKET_ASSERT(ireg <= INT64_MAX);
            ireg |= 1;
            freg = static_cast<double>(static_cast<int64_t>(ireg));
            freg = ::std::ldexp(freg, mult.bexp - lzcnt);
            break;
          }
        default:
          ROCKET_ASSERT_MSG(false, "non-decimal floating-point parsing not implemented");
        }
        // Examine the value. Note that `ireg` is non-zero.
        // If the result becomes infinity, it must have overflowed.
        // If the result becomes zero, it must have underflowed.
        int cls = ::std::fpclassify(freg);
        if(cls == FP_INFINITE) {
          this->m_ovfl = true;
        }
        else if(cls == FP_ZERO) {
          this->m_udfl = true;
        }
        // Set the value.
        value = ::std::copysign(freg, sign);
        break;
      }
    case 1:  // infinitesimal
      {
        // Truncate the value to zero.
        value = ::std::copysign(0.0, sign);
        // For floating-point numbers this always underflows.
        this->m_udfl = true;
        break;
      }
    case 2:  // infinity
      {
        // Return a signed infinity.
        value = ::std::copysign(numeric_limits<double>::infinity(), sign);
        break;
      }
    default:  // quiet NaN
      {
        // Return a signed QNaN.
        value = ::std::copysign(numeric_limits<double>::quiet_NaN(), sign);
        break;
      }
    }
    // Set the overflow flag if the value is out of range.
    // Watch out for NaNs.
    if(::std::isless(value, lower)) {
      value = lower;
      this->m_ovfl = true;
    }
    else if(::std::isgreater(value, upper)) {
      value = upper;
      this->m_ovfl = true;
    }
    // Report success if no error occurred.
    if(this->m_stat != 0) {
      return *this;
    }
    this->m_succ = true;
    return *this;
  }

}  // namespace rocket
