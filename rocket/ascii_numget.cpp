// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "ascii_numget.hpp"
#include "assert.hpp"
namespace rocket {
namespace {

ROCKET_ALWAYS_INLINE
bool
do_subseq(const char*& rptr, const char* eptr, const char* cstr)
  {
    size_t n = ::strlen(cstr);
    if(((size_t) (eptr - rptr) >= n) && (::memcmp(rptr, cstr, n) == 0)) {
      // `n` should always be a constant but can this be enhanced?
      rptr += (ptrdiff_t) n;
      return true;
    }
    return false;
  }

inline
bool
do_get_sign(const char*& rptr, const char* eptr)
  {
    if(do_subseq(rptr, eptr, "+")) {
      return 0;
    }
    else if(do_subseq(rptr, eptr, "-")) {
      return 1;
    }
    else
      return 0;  // default; implicitly positive
  }

enum value_class : uint8_t
  {
    value_class_empty          = 0,
    value_class_infinitesimal  = 1,
    value_class_infinity       = 2,
    value_class_nan            = 3,
    value_class_finite         = 4,
  };

inline
value_class
do_get_special_value(const char*& rptr, const char* eptr)
  {
    if(do_subseq(rptr, eptr, "nan") || do_subseq(rptr, eptr, "NaN")) {
      return value_class_nan;
    }
    else if(do_subseq(rptr, eptr, "infinity") || do_subseq(rptr, eptr, "Infinity")) {
      return value_class_infinity;
    }
    else if(do_subseq(rptr, eptr, "inf") || do_subseq(rptr, eptr, "Inf")) {
      return value_class_infinity;
    }
    else
      return value_class_finite;
  }

inline
void
do_skip_base_prefix(const char*& rptr, const char* eptr, char base_char)
  {
    if((eptr - rptr >= 2) && (rptr[0] == '0') && ((rptr[1] | 0x20) == base_char))
      rptr += 2;
  }

struct xuint
  {
    int64_t exp;
    uint64_t mant;
    bool has_point;
    bool valid;
    bool more;
  };

constexpr int8_t s_digit_values[256] =
  {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,  // 0-9
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,  // A-O
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,  // P-Z
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,  // a-o
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,  // p-z
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  };

inline
xuint
do_collect_digits(const char*& rptr, const char* eptr, uint32_t base, int rdxp)
  {
    xuint xu;
    xu.exp = 0;
    xu.mant = 0;
    xu.has_point = false;
    xu.valid = false;
    xu.more = false;

    // Collect significant figures.
    bool mant_ovfl = false;
    uint64_t next_mant;

    while(rptr != eptr) {
      // The significand part allows digits with at most one radix point.
      if(!xu.has_point && ((uint8_t) *rptr == rdxp)) {
        rptr ++;
        xu.has_point = true;
        continue;
      }

      // Check whether it is a digit.
      uint32_t digit = (uint32_t) s_digit_values[(uint8_t) *rptr];
      if(digit >= base)
        break;

      // Accumulate this digit.
      rptr ++;
      xu.valid = true;

      if(!mant_ovfl)
        mant_ovfl = ROCKET_MUL_ADD_OVERFLOW(xu.mant, base, digit, &next_mant);

      if(mant_ovfl) {
        xu.exp ++;
        xu.more |= digit != 0;
      }
      else
        xu.mant = next_mant;

      if(xu.has_point)
        xu.exp --;
    }

    // The base of exponent of hexadecimal floating-point numbers is
    // two. For binary and decimal numbers, they are the same as their
    // significant figures.
    if(base == 16)
      xu.exp <<= 2;

    // Look for the exponent. If no exponent exists, finish.
    if((eptr - rptr >= 2) && ((rptr[0] | 0x20) == ((base == 10) ? 'e' : 'p'))
                          && ((rptr[1] >= '0') && (rptr[1] <= '9'))) {
      rptr += 1;
    }
    else if((eptr - rptr >= 3) && ((rptr[0] | 0x20) == ((base == 10) ? 'e' : 'p'))
                               && ((rptr[1] == '+') || (rptr[1] == '-'))
                               && ((rptr[2] >= '0') && (rptr[2] <= '9'))) {
      rptr += 2;
    }
    else
      return xu;

    // Collect exponent digits.
    int64_t exp_sign = (rptr[-1] == '-') ? -1LL : 0LL;
    bool exp_ovfl = false;
    int64_t exp_orig = xu.exp;

    while(rptr != eptr) {
      // The exponent part only allows decimal digits.
      uint32_t digit = (uint32_t) s_digit_values[(uint8_t) *rptr];
      if(digit >= 10)
        break;

      // Accumulate this digit.
      rptr ++;

      if(!exp_ovfl)
        exp_ovfl = ROCKET_MUL_ADD_OVERFLOW(xu.exp - exp_orig, 10,
                       ((int64_t) digit ^ exp_sign) - exp_sign + exp_orig,
                       &(xu.exp));
    }

    if(exp_ovfl)
      xu.exp = INT64_MAX ^ exp_sign;

    return xu;
  }

#if 0
/* This program is used to generate the multiplier table for decimal
 * numbers. Each multiplier for the mantissa is split into two parts.
 * The base of exponents is 2.
 *
 * Compile with:
 *   gcc -std=c99 -W{all,extra,{sign-,}conversion} table.c -lquadmath
**/

#include <quadmath.h>
#include <stdio.h>

void
do_print_one(int e)
  {
    __float128 value, frac;
    int exp2;
    unsigned long long mant;

    // Calculate the multiplier.
    value = powq(10, e);

    // Break it down into the fraction and exponent.
    frac = frexpq(value, &exp2);

    // Round the fraction towards positive infinity.
    frac = ldexpq(frac, 64);
    mant = (unsigned long long) ceilq(frac);

    // Print the mantissa in fixed-point format.
    printf("    { 0x%.16llX, ", mant);

    // Print the binary exponent.
    printf("%+5d },", exp2);

    // Print the decimal exponent in comments.
    printf("  // 1.0e%+.3d\n", e);
  }

int
main(void)
  {
    int e;

    for(e = -343; e <= +308; ++e)
      do_print_one(e);

    return 0;
  }
#endif

// These are generated data. Do not edit by hand!
struct decimal_multiplier
  {
    uint64_t mant;
    int exp2;
  }
constexpr s_decimal_multipliers[] =
  {
    { 0xBF29DCABA82FDEAF, -1139 },  // 1.0e-343
    { 0xEEF453D6923BD65B, -1136 },  // 1.0e-342
    { 0x9558B4661B6565F9, -1132 },  // 1.0e-341
    { 0xBAAEE17FA23EBF77, -1129 },  // 1.0e-340
    { 0xE95A99DF8ACE6F54, -1126 },  // 1.0e-339
    { 0x91D8A02BB6C10595, -1122 },  // 1.0e-338
    { 0xB64EC836A47146FA, -1119 },  // 1.0e-337
    { 0xE3E27A444D8D98B8, -1116 },  // 1.0e-336
    { 0x8E6D8C6AB0787F73, -1112 },  // 1.0e-335
    { 0xB208EF855C969F50, -1109 },  // 1.0e-334
    { 0xDE8B2B66B3BC4724, -1106 },  // 1.0e-333
    { 0x8B16FB203055AC77, -1102 },  // 1.0e-332
    { 0xADDCB9E83C6B1794, -1099 },  // 1.0e-331
    { 0xD953E8624B85DD79, -1096 },  // 1.0e-330
    { 0x87D4713D6F33AA6C, -1092 },  // 1.0e-329
    { 0xA9C98D8CCB009507, -1089 },  // 1.0e-328
    { 0xD43BF0EFFDC0BA49, -1086 },  // 1.0e-327
    { 0x84A57695FE98746E, -1082 },  // 1.0e-326
    { 0xA5CED43B7E3E9189, -1079 },  // 1.0e-325
    { 0xCF42894A5DCE35EB, -1076 },  // 1.0e-324
    { 0x818995CE7AA0E1B3, -1072 },  // 1.0e-323
    { 0xA1EBFB4219491A20, -1069 },  // 1.0e-322
    { 0xCA66FA129F9B60A7, -1066 },  // 1.0e-321
    { 0xFD00B897478238D1, -1063 },  // 1.0e-320
    { 0x9E20735E8CB16383, -1059 },  // 1.0e-319
    { 0xC5A890362FDDBC63, -1056 },  // 1.0e-318
    { 0xF712B443BBD52B7C, -1053 },  // 1.0e-317
    { 0x9A6BB0AA55653B2E, -1049 },  // 1.0e-316
    { 0xC1069CD4EABE89F9, -1046 },  // 1.0e-315
    { 0xF148440A256E2C77, -1043 },  // 1.0e-314
    { 0x96CD2A865764DBCB, -1039 },  // 1.0e-313
    { 0xBC807527ED3E12BD, -1036 },  // 1.0e-312
    { 0xEBA09271E88D976C, -1033 },  // 1.0e-311
    { 0x93445B8731587EA4, -1029 },  // 1.0e-310
    { 0xB8157268FDAE9E4D, -1026 },  // 1.0e-309
    { 0xE61ACF033D1A45E0, -1023 },  // 1.0e-308
    { 0x8FD0C16206306BAC, -1019 },  // 1.0e-307
    { 0xB3C4F1BA87BC8697, -1016 },  // 1.0e-306
    { 0xE0B62E2929ABA83D, -1013 },  // 1.0e-305
    { 0x8C71DCD9BA0B4926, -1009 },  // 1.0e-304
    { 0xAF8E5410288E1B70, -1006 },  // 1.0e-303
    { 0xDB71E91432B1A24B, -1003 },  // 1.0e-302
    { 0x892731AC9FAF056F,  -999 },  // 1.0e-301
    { 0xAB70FE17C79AC6CB,  -996 },  // 1.0e-300
    { 0xD64D3D9DB981787E,  -993 },  // 1.0e-299
    { 0x85F0468293F0EB4F,  -989 },  // 1.0e-298
    { 0xA76C582338ED2622,  -986 },  // 1.0e-297
    { 0xD1476E2C07286FAB,  -983 },  // 1.0e-296
    { 0x82CCA4DB847945CB,  -979 },  // 1.0e-295
    { 0xA37FCE126597973D,  -976 },  // 1.0e-294
    { 0xCC5FC196FEFD7D0D,  -973 },  // 1.0e-293
    { 0xFF77B1FCBEBCDC50,  -970 },  // 1.0e-292
    { 0x9FAACF3DF73609B2,  -966 },  // 1.0e-291
    { 0xC795830D75038C1E,  -963 },  // 1.0e-290
    { 0xF97AE3D0D2446F26,  -960 },  // 1.0e-289
    { 0x9BECCE62836AC578,  -956 },  // 1.0e-288
    { 0xC2E801FB244576D6,  -953 },  // 1.0e-287
    { 0xF3A20279ED56D48B,  -950 },  // 1.0e-286
    { 0x9845418C345644D7,  -946 },  // 1.0e-285
    { 0xBE5691EF416BD60D,  -943 },  // 1.0e-284
    { 0xEDEC366B11C6CB90,  -940 },  // 1.0e-283
    { 0x94B3A202EB1C3F3A,  -936 },  // 1.0e-282
    { 0xB9E08A83A5E34F08,  -933 },  // 1.0e-281
    { 0xE858AD248F5C22CA,  -930 },  // 1.0e-280
    { 0x91376C36D99995BF,  -926 },  // 1.0e-279
    { 0xB58547448FFFFB2E,  -923 },  // 1.0e-278
    { 0xE2E69915B3FFF9FA,  -920 },  // 1.0e-277
    { 0x8DD01FAD907FFC3C,  -916 },  // 1.0e-276
    { 0xB1442798F49FFB4B,  -913 },  // 1.0e-275
    { 0xDD95317F31C7FA1E,  -910 },  // 1.0e-274
    { 0x8A7D3EEF7F1CFC53,  -906 },  // 1.0e-273
    { 0xAD1C8EAB5EE43B67,  -903 },  // 1.0e-272
    { 0xD863B256369D4A41,  -900 },  // 1.0e-271
    { 0x873E4F75E2224E69,  -896 },  // 1.0e-270
    { 0xA90DE3535AAAE203,  -893 },  // 1.0e-269
    { 0xD3515C2831559A84,  -890 },  // 1.0e-268
    { 0x8412D9991ED58092,  -886 },  // 1.0e-267
    { 0xA5178FFF668AE0B7,  -883 },  // 1.0e-266
    { 0xCE5D73FF402D98E4,  -880 },  // 1.0e-265
    { 0x80FA687F881C7F8F,  -876 },  // 1.0e-264
    { 0xA139029F6A239F73,  -873 },  // 1.0e-263
    { 0xC987434744AC874F,  -870 },  // 1.0e-262
    { 0xFBE9141915D7A923,  -867 },  // 1.0e-261
    { 0x9D71AC8FADA6C9B6,  -863 },  // 1.0e-260
    { 0xC4CE17B399107C23,  -860 },  // 1.0e-259
    { 0xF6019DA07F549B2C,  -857 },  // 1.0e-258
    { 0x99C102844F94E0FC,  -853 },  // 1.0e-257
    { 0xC0314325637A193A,  -850 },  // 1.0e-256
    { 0xF03D93EEBC589F89,  -847 },  // 1.0e-255
    { 0x96267C7535B763B6,  -843 },  // 1.0e-254
    { 0xBBB01B9283253CA3,  -840 },  // 1.0e-253
    { 0xEA9C227723EE8BCC,  -837 },  // 1.0e-252
    { 0x92A1958A76751760,  -833 },  // 1.0e-251
    { 0xB749FAED14125D37,  -830 },  // 1.0e-250
    { 0xE51C79A85916F485,  -827 },  // 1.0e-249
    { 0x8F31CC0937AE58D3,  -823 },  // 1.0e-248
    { 0xB2FE3F0B8599EF08,  -820 },  // 1.0e-247
    { 0xDFBDCECE67006ACA,  -817 },  // 1.0e-246
    { 0x8BD6A141006042BE,  -813 },  // 1.0e-245
    { 0xAECC49914078536E,  -810 },  // 1.0e-244
    { 0xDA7F5BF590966849,  -807 },  // 1.0e-243
    { 0x888F99797A5E012E,  -803 },  // 1.0e-242
    { 0xAAB37FD7D8F58179,  -800 },  // 1.0e-241
    { 0xD5605FCDCF32E1D7,  -797 },  // 1.0e-240
    { 0x855C3BE0A17FCD27,  -793 },  // 1.0e-239
    { 0xA6B34AD8C9DFC070,  -790 },  // 1.0e-238
    { 0xD0601D8EFC57B08C,  -787 },  // 1.0e-237
    { 0x823C12795DB6CE58,  -783 },  // 1.0e-236
    { 0xA2CB1717B52481EE,  -780 },  // 1.0e-235
    { 0xCB7DDCDDA26DA269,  -777 },  // 1.0e-234
    { 0xFE5D54150B090B03,  -774 },  // 1.0e-233
    { 0x9EFA548D26E5A6E2,  -770 },  // 1.0e-232
    { 0xC6B8E9B0709F109B,  -767 },  // 1.0e-231
    { 0xF867241C8CC6D4C1,  -764 },  // 1.0e-230
    { 0x9B407691D7FC44F9,  -760 },  // 1.0e-229
    { 0xC21094364DFB5637,  -757 },  // 1.0e-228
    { 0xF294B943E17A2BC5,  -754 },  // 1.0e-227
    { 0x979CF3CA6CEC5B5B,  -750 },  // 1.0e-226
    { 0xBD8430BD08277232,  -747 },  // 1.0e-225
    { 0xECE53CEC4A314EBE,  -744 },  // 1.0e-224
    { 0x940F4613AE5ED137,  -740 },  // 1.0e-223
    { 0xB913179899F68585,  -737 },  // 1.0e-222
    { 0xE757DD7EC07426E6,  -734 },  // 1.0e-221
    { 0x9096EA6F38489850,  -730 },  // 1.0e-220
    { 0xB4BCA50B065ABE64,  -727 },  // 1.0e-219
    { 0xE1EBCE4DC7F16DFC,  -724 },  // 1.0e-218
    { 0x8D3360F09CF6E4BE,  -720 },  // 1.0e-217
    { 0xB080392CC4349DED,  -717 },  // 1.0e-216
    { 0xDCA04777F541C568,  -714 },  // 1.0e-215
    { 0x89E42CAAF9491B61,  -710 },  // 1.0e-214
    { 0xAC5D37D5B79B623A,  -707 },  // 1.0e-213
    { 0xD77485CB25823AC8,  -704 },  // 1.0e-212
    { 0x86A8D39EF77164BD,  -700 },  // 1.0e-211
    { 0xA8530886B54DBDEC,  -697 },  // 1.0e-210
    { 0xD267CAA862A12D67,  -694 },  // 1.0e-209
    { 0x8380DEA93DA4BC61,  -690 },  // 1.0e-208
    { 0xA46116538D0DEB79,  -687 },  // 1.0e-207
    { 0xCD795BE870516657,  -684 },  // 1.0e-206
    { 0x806BD9714632DFF7,  -680 },  // 1.0e-205
    { 0xA086CFCD97BF97F4,  -677 },  // 1.0e-204
    { 0xC8A883C0FDAF7DF1,  -674 },  // 1.0e-203
    { 0xFAD2A4B13D1B5D6D,  -671 },  // 1.0e-202
    { 0x9CC3A6EEC6311A64,  -667 },  // 1.0e-201
    { 0xC3F490AA77BD60FD,  -664 },  // 1.0e-200
    { 0xF4F1B4D515ACB93C,  -661 },  // 1.0e-199
    { 0x991711052D8BF3C6,  -657 },  // 1.0e-198
    { 0xBF5CD54678EEF0B7,  -654 },  // 1.0e-197
    { 0xEF340A98172AACE5,  -651 },  // 1.0e-196
    { 0x9580869F0E7AAC0F,  -647 },  // 1.0e-195
    { 0xBAE0A846D2195713,  -644 },  // 1.0e-194
    { 0xE998D258869FACD8,  -641 },  // 1.0e-193
    { 0x91FF83775423CC07,  -637 },  // 1.0e-192
    { 0xB67F6455292CBF09,  -634 },  // 1.0e-191
    { 0xE41F3D6A7377EECB,  -631 },  // 1.0e-190
    { 0x8E938662882AF53F,  -627 },  // 1.0e-189
    { 0xB23867FB2A35B28E,  -624 },  // 1.0e-188
    { 0xDEC681F9F4C31F32,  -621 },  // 1.0e-187
    { 0x8B3C113C38F9F37F,  -617 },  // 1.0e-186
    { 0xAE0B158B4738705F,  -614 },  // 1.0e-185
    { 0xD98DDAEE19068C77,  -611 },  // 1.0e-184
    { 0x87F8A8D4CFA417CA,  -607 },  // 1.0e-183
    { 0xA9F6D30A038D1DBD,  -604 },  // 1.0e-182
    { 0xD47487CC8470652C,  -601 },  // 1.0e-181
    { 0x84C8D4DFD2C63F3C,  -597 },  // 1.0e-180
    { 0xA5FB0A17C777CF0A,  -594 },  // 1.0e-179
    { 0xCF79CC9DB955C2CD,  -591 },  // 1.0e-178
    { 0x81AC1FE293D599C0,  -587 },  // 1.0e-177
    { 0xA21727DB38CB0030,  -584 },  // 1.0e-176
    { 0xCA9CF1D206FDC03C,  -581 },  // 1.0e-175
    { 0xFD442E4688BD304B,  -578 },  // 1.0e-174
    { 0x9E4A9CEC15763E2F,  -574 },  // 1.0e-173
    { 0xC5DD44271AD3CDBB,  -571 },  // 1.0e-172
    { 0xF7549530E188C129,  -568 },  // 1.0e-171
    { 0x9A94DD3E8CF578BA,  -564 },  // 1.0e-170
    { 0xC13A148E3032D6E8,  -561 },  // 1.0e-169
    { 0xF18899B1BC3F8CA2,  -558 },  // 1.0e-168
    { 0x96F5600F15A7B7E6,  -554 },  // 1.0e-167
    { 0xBCB2B812DB11A5DF,  -551 },  // 1.0e-166
    { 0xEBDF661791D60F57,  -548 },  // 1.0e-165
    { 0x936B9FCEBB25C996,  -544 },  // 1.0e-164
    { 0xB84687C269EF3BFC,  -541 },  // 1.0e-163
    { 0xE65829B3046B0AFB,  -538 },  // 1.0e-162
    { 0x8FF71A0FE2C2E6DD,  -534 },  // 1.0e-161
    { 0xB3F4E093DB73A094,  -531 },  // 1.0e-160
    { 0xE0F218B8D25088B9,  -528 },  // 1.0e-159
    { 0x8C974F7383725574,  -524 },  // 1.0e-158
    { 0xAFBD2350644EEAD0,  -521 },  // 1.0e-157
    { 0xDBAC6C247D62A584,  -518 },  // 1.0e-156
    { 0x894BC396CE5DA773,  -514 },  // 1.0e-155
    { 0xAB9EB47C81F51150,  -511 },  // 1.0e-154
    { 0xD686619BA27255A3,  -508 },  // 1.0e-153
    { 0x8613FD0145877586,  -504 },  // 1.0e-152
    { 0xA798FC4196E952E8,  -501 },  // 1.0e-151
    { 0xD17F3B51FCA3A7A1,  -498 },  // 1.0e-150
    { 0x82EF85133DE648C5,  -494 },  // 1.0e-149
    { 0xA3AB66580D5FDAF6,  -491 },  // 1.0e-148
    { 0xCC963FEE10B7D1B4,  -488 },  // 1.0e-147
    { 0xFFBBCFE994E5C620,  -485 },  // 1.0e-146
    { 0x9FD561F1FD0F9BD4,  -481 },  // 1.0e-145
    { 0xC7CABA6E7C5382C9,  -478 },  // 1.0e-144
    { 0xF9BD690A1B68637C,  -475 },  // 1.0e-143
    { 0x9C1661A651213E2E,  -471 },  // 1.0e-142
    { 0xC31BFA0FE5698DB9,  -468 },  // 1.0e-141
    { 0xF3E2F893DEC3F127,  -465 },  // 1.0e-140
    { 0x986DDB5C6B3A76B8,  -461 },  // 1.0e-139
    { 0xBE89523386091466,  -458 },  // 1.0e-138
    { 0xEE2BA6C0678B5980,  -455 },  // 1.0e-137
    { 0x94DB483840B717F0,  -451 },  // 1.0e-136
    { 0xBA121A4650E4DDEC,  -448 },  // 1.0e-135
    { 0xE896A0D7E51E1567,  -445 },  // 1.0e-134
    { 0x915E2486EF32CD61,  -441 },  // 1.0e-133
    { 0xB5B5ADA8AAFF80B9,  -438 },  // 1.0e-132
    { 0xE3231912D5BF60E7,  -435 },  // 1.0e-131
    { 0x8DF5EFABC5979C90,  -431 },  // 1.0e-130
    { 0xB1736B96B6FD83B4,  -428 },  // 1.0e-129
    { 0xDDD0467C64BCE4A1,  -425 },  // 1.0e-128
    { 0x8AA22C0DBEF60EE5,  -421 },  // 1.0e-127
    { 0xAD4AB7112EB3929E,  -418 },  // 1.0e-126
    { 0xD89D64D57A607745,  -415 },  // 1.0e-125
    { 0x87625F056C7C4A8C,  -411 },  // 1.0e-124
    { 0xA93AF6C6C79B5D2E,  -408 },  // 1.0e-123
    { 0xD389B4787982347A,  -405 },  // 1.0e-122
    { 0x843610CB4BF160CC,  -401 },  // 1.0e-121
    { 0xA54394FE1EEDB8FF,  -398 },  // 1.0e-120
    { 0xCE947A3DA6A9273F,  -395 },  // 1.0e-119
    { 0x811CCC668829B888,  -391 },  // 1.0e-118
    { 0xA163FF802A3426A9,  -388 },  // 1.0e-117
    { 0xC9BCFF6034C13053,  -385 },  // 1.0e-116
    { 0xFC2C3F3841F17C68,  -382 },  // 1.0e-115
    { 0x9D9BA7832936EDC1,  -378 },  // 1.0e-114
    { 0xC5029163F384A932,  -375 },  // 1.0e-113
    { 0xF64335BCF065D37E,  -372 },  // 1.0e-112
    { 0x99EA0196163FA42F,  -368 },  // 1.0e-111
    { 0xC06481FB9BCF8D3A,  -365 },  // 1.0e-110
    { 0xF07DA27A82C37089,  -362 },  // 1.0e-109
    { 0x964E858C91BA2656,  -358 },  // 1.0e-108
    { 0xBBE226EFB628AFEB,  -355 },  // 1.0e-107
    { 0xEADAB0ABA3B2DBE6,  -352 },  // 1.0e-106
    { 0x92C8AE6B464FC970,  -348 },  // 1.0e-105
    { 0xB77ADA0617E3BBCC,  -345 },  // 1.0e-104
    { 0xE55990879DDCAABE,  -342 },  // 1.0e-103
    { 0x8F57FA54C2A9EAB7,  -338 },  // 1.0e-102
    { 0xB32DF8E9F3546565,  -335 },  // 1.0e-101
    { 0xDFF9772470297EBE,  -332 },  // 1.0e-100
    { 0x8BFBEA76C619EF37,  -328 },  // 1.0e-099
    { 0xAEFAE51477A06B04,  -325 },  // 1.0e-098
    { 0xDAB99E59958885C5,  -322 },  // 1.0e-097
    { 0x88B402F7FD75539C,  -318 },  // 1.0e-096
    { 0xAAE103B5FCD2A882,  -315 },  // 1.0e-095
    { 0xD59944A37C0752A3,  -312 },  // 1.0e-094
    { 0x857FCAE62D8493A6,  -308 },  // 1.0e-093
    { 0xA6DFBD9FB8E5B88F,  -305 },  // 1.0e-092
    { 0xD097AD07A71F26B3,  -302 },  // 1.0e-091
    { 0x825ECC24C8737830,  -298 },  // 1.0e-090
    { 0xA2F67F2DFA90563C,  -295 },  // 1.0e-089
    { 0xCBB41EF979346BCB,  -292 },  // 1.0e-088
    { 0xFEA126B7D78186BD,  -289 },  // 1.0e-087
    { 0x9F24B832E6B0F437,  -285 },  // 1.0e-086
    { 0xC6EDE63FA05D3144,  -282 },  // 1.0e-085
    { 0xF8A95FCF88747D95,  -279 },  // 1.0e-084
    { 0x9B69DBE1B548CE7D,  -275 },  // 1.0e-083
    { 0xC24452DA229B021C,  -272 },  // 1.0e-082
    { 0xF2D56790AB41C2A3,  -269 },  // 1.0e-081
    { 0x97C560BA6B0919A6,  -265 },  // 1.0e-080
    { 0xBDB6B8E905CB6010,  -262 },  // 1.0e-079
    { 0xED246723473E3814,  -259 },  // 1.0e-078
    { 0x9436C0760C86E30C,  -255 },  // 1.0e-077
    { 0xB94470938FA89BCF,  -252 },  // 1.0e-076
    { 0xE7958CB87392C2C3,  -249 },  // 1.0e-075
    { 0x90BD77F3483BB9BA,  -245 },  // 1.0e-074
    { 0xB4ECD5F01A4AA829,  -242 },  // 1.0e-073
    { 0xE2280B6C20DD5233,  -239 },  // 1.0e-072
    { 0x8D590723948A5360,  -235 },  // 1.0e-071
    { 0xB0AF48EC79ACE838,  -232 },  // 1.0e-070
    { 0xDCDB1B2798182245,  -229 },  // 1.0e-069
    { 0x8A08F0F8BF0F156C,  -225 },  // 1.0e-068
    { 0xAC8B2D36EED2DAC6,  -222 },  // 1.0e-067
    { 0xD7ADF884AA879178,  -219 },  // 1.0e-066
    { 0x86CCBB52EA94BAEB,  -215 },  // 1.0e-065
    { 0xA87FEA27A539E9A6,  -212 },  // 1.0e-064
    { 0xD29FE4B18E88640F,  -209 },  // 1.0e-063
    { 0x83A3EEEEF9153E8A,  -205 },  // 1.0e-062
    { 0xA48CEAAAB75A8E2C,  -202 },  // 1.0e-061
    { 0xCDB02555653131B7,  -199 },  // 1.0e-060
    { 0x808E17555F3EBF12,  -195 },  // 1.0e-059
    { 0xA0B19D2AB70E6ED7,  -192 },  // 1.0e-058
    { 0xC8DE047564D20A8C,  -189 },  // 1.0e-057
    { 0xFB158592BE068D2F,  -186 },  // 1.0e-056
    { 0x9CED737BB6C4183E,  -182 },  // 1.0e-055
    { 0xC428D05AA4751E4D,  -179 },  // 1.0e-054
    { 0xF53304714D9265E0,  -176 },  // 1.0e-053
    { 0x993FE2C6D07B7FAC,  -172 },  // 1.0e-052
    { 0xBF8FDB78849A5F97,  -169 },  // 1.0e-051
    { 0xEF73D256A5C0F77D,  -166 },  // 1.0e-050
    { 0x95A8637627989AAE,  -162 },  // 1.0e-049
    { 0xBB127C53B17EC15A,  -159 },  // 1.0e-048
    { 0xE9D71B689DDE71B0,  -156 },  // 1.0e-047
    { 0x9226712162AB070E,  -152 },  // 1.0e-046
    { 0xB6B00D69BB55C8D2,  -149 },  // 1.0e-045
    { 0xE45C10C42A2B3B06,  -146 },  // 1.0e-044
    { 0x8EB98A7A9A5B04E4,  -142 },  // 1.0e-043
    { 0xB267ED1940F1C61D,  -139 },  // 1.0e-042
    { 0xDF01E85F912E37A4,  -136 },  // 1.0e-041
    { 0x8B61313BBABCE2C7,  -132 },  // 1.0e-040
    { 0xAE397D8AA96C1B78,  -129 },  // 1.0e-039
    { 0xD9C7DCED53C72256,  -126 },  // 1.0e-038
    { 0x881CEA14545C7576,  -122 },  // 1.0e-037
    { 0xAA242499697392D3,  -119 },  // 1.0e-036
    { 0xD4AD2DBFC3D07788,  -116 },  // 1.0e-035
    { 0x84EC3C97DA624AB5,  -112 },  // 1.0e-034
    { 0xA6274BBDD0FADD62,  -109 },  // 1.0e-033
    { 0xCFB11EAD453994BB,  -106 },  // 1.0e-032
    { 0x81CEB32C4B43FCF5,  -102 },  // 1.0e-031
    { 0xA2425FF75E14FC32,   -99 },  // 1.0e-030
    { 0xCAD2F7F5359A3B3F,   -96 },  // 1.0e-029
    { 0xFD87B5F28300CA0E,   -93 },  // 1.0e-028
    { 0x9E74D1B791E07E49,   -89 },  // 1.0e-027
    { 0xC612062576589DDB,   -86 },  // 1.0e-026
    { 0xF79687AED3EEC552,   -83 },  // 1.0e-025
    { 0x9ABE14CD44753B53,   -79 },  // 1.0e-024
    { 0xC16D9A0095928A28,   -76 },  // 1.0e-023
    { 0xF1C90080BAF72CB2,   -73 },  // 1.0e-022
    { 0x971DA05074DA7BEF,   -69 },  // 1.0e-021
    { 0xBCE5086492111AEB,   -66 },  // 1.0e-020
    { 0xEC1E4A7DB69561A6,   -63 },  // 1.0e-019
    { 0x9392EE8E921D5D08,   -59 },  // 1.0e-018
    { 0xB877AA3236A4B44A,   -56 },  // 1.0e-017
    { 0xE69594BEC44DE15C,   -53 },  // 1.0e-016
    { 0x901D7CF73AB0ACDA,   -49 },  // 1.0e-015
    { 0xB424DC35095CD810,   -46 },  // 1.0e-014
    { 0xE12E13424BB40E14,   -43 },  // 1.0e-013
    { 0x8CBCCC096F5088CC,   -39 },  // 1.0e-012
    { 0xAFEBFF0BCB24AAFF,   -36 },  // 1.0e-011
    { 0xDBE6FECEBDEDD5BF,   -33 },  // 1.0e-010
    { 0x89705F4136B4A598,   -29 },  // 1.0e-009
    { 0xABCC77118461CEFD,   -26 },  // 1.0e-008
    { 0xD6BF94D5E57A42BD,   -23 },  // 1.0e-007
    { 0x8637BD05AF6C69B6,   -19 },  // 1.0e-006
    { 0xA7C5AC471B478424,   -16 },  // 1.0e-005
    { 0xD1B71758E219652C,   -13 },  // 1.0e-004
    { 0x83126E978D4FDF3C,    -9 },  // 1.0e-003
    { 0xA3D70A3D70A3D70B,    -6 },  // 1.0e-002
    { 0xCCCCCCCCCCCCCCCD,    -3 },  // 1.0e-001
    { 0x8000000000000000,    +1 },  // 1.0e+000
    { 0xA000000000000000,    +4 },  // 1.0e+001
    { 0xC800000000000000,    +7 },  // 1.0e+002
    { 0xFA00000000000000,   +10 },  // 1.0e+003
    { 0x9C40000000000000,   +14 },  // 1.0e+004
    { 0xC350000000000000,   +17 },  // 1.0e+005
    { 0xF424000000000000,   +20 },  // 1.0e+006
    { 0x9896800000000000,   +24 },  // 1.0e+007
    { 0xBEBC200000000000,   +27 },  // 1.0e+008
    { 0xEE6B280000000000,   +30 },  // 1.0e+009
    { 0x9502F90000000000,   +34 },  // 1.0e+010
    { 0xBA43B74000000000,   +37 },  // 1.0e+011
    { 0xE8D4A51000000000,   +40 },  // 1.0e+012
    { 0x9184E72A00000000,   +44 },  // 1.0e+013
    { 0xB5E620F480000000,   +47 },  // 1.0e+014
    { 0xE35FA931A0000000,   +50 },  // 1.0e+015
    { 0x8E1BC9BF04000000,   +54 },  // 1.0e+016
    { 0xB1A2BC2EC5000000,   +57 },  // 1.0e+017
    { 0xDE0B6B3A76400000,   +60 },  // 1.0e+018
    { 0x8AC7230489E80000,   +64 },  // 1.0e+019
    { 0xAD78EBC5AC620000,   +67 },  // 1.0e+020
    { 0xD8D726B7177A8000,   +70 },  // 1.0e+021
    { 0x878678326EAC9000,   +74 },  // 1.0e+022
    { 0xA968163F0A57B400,   +77 },  // 1.0e+023
    { 0xD3C21BCECCEDA100,   +80 },  // 1.0e+024
    { 0x84595161401484A0,   +84 },  // 1.0e+025
    { 0xA56FA5B99019A5C8,   +87 },  // 1.0e+026
    { 0xCECB8F27F4200F3A,   +90 },  // 1.0e+027
    { 0x813F3978F8940985,   +94 },  // 1.0e+028
    { 0xA18F07D736B90BE6,   +97 },  // 1.0e+029
    { 0xC9F2C9CD04674EDF,  +100 },  // 1.0e+030
    { 0xFC6F7C4045812297,  +103 },  // 1.0e+031
    { 0x9DC5ADA82B70B59E,  +107 },  // 1.0e+032
    { 0xC5371912364CE306,  +110 },  // 1.0e+033
    { 0xF684DF56C3E01BC7,  +113 },  // 1.0e+034
    { 0x9A130B963A6C115D,  +117 },  // 1.0e+035
    { 0xC097CE7BC90715B4,  +120 },  // 1.0e+036
    { 0xF0BDC21ABB48DB21,  +123 },  // 1.0e+037
    { 0x96769950B50D88F5,  +127 },  // 1.0e+038
    { 0xBC143FA4E250EB32,  +130 },  // 1.0e+039
    { 0xEB194F8E1AE525FE,  +133 },  // 1.0e+040
    { 0x92EFD1B8D0CF37BF,  +137 },  // 1.0e+041
    { 0xB7ABC627050305AE,  +140 },  // 1.0e+042
    { 0xE596B7B0C643C71A,  +143 },  // 1.0e+043
    { 0x8F7E32CE7BEA5C70,  +147 },  // 1.0e+044
    { 0xB35DBF821AE4F38C,  +150 },  // 1.0e+045
    { 0xE0352F62A19E306F,  +153 },  // 1.0e+046
    { 0x8C213D9DA502DE46,  +157 },  // 1.0e+047
    { 0xAF298D050E4395D7,  +160 },  // 1.0e+048
    { 0xDAF3F04651D47B4D,  +163 },  // 1.0e+049
    { 0x88D8762BF324CD10,  +167 },  // 1.0e+050
    { 0xAB0E93B6EFEE0054,  +170 },  // 1.0e+051
    { 0xD5D238A4ABE98069,  +173 },  // 1.0e+052
    { 0x85A36366EB71F042,  +177 },  // 1.0e+053
    { 0xA70C3C40A64E6C52,  +180 },  // 1.0e+054
    { 0xD0CF4B50CFE20766,  +183 },  // 1.0e+055
    { 0x82818F1281ED44A0,  +187 },  // 1.0e+056
    { 0xA321F2D7226895C8,  +190 },  // 1.0e+057
    { 0xCBEA6F8CEB02BB3A,  +193 },  // 1.0e+058
    { 0xFEE50B7025C36A09,  +196 },  // 1.0e+059
    { 0x9F4F2726179A2246,  +200 },  // 1.0e+060
    { 0xC722F0EF9D80AAD7,  +203 },  // 1.0e+061
    { 0xF8EBAD2B84E0D58C,  +206 },  // 1.0e+062
    { 0x9B934C3B330C8578,  +210 },  // 1.0e+063
    { 0xC2781F49FFCFA6D6,  +213 },  // 1.0e+064
    { 0xF316271C7FC3908B,  +216 },  // 1.0e+065
    { 0x97EDD871CFDA3A57,  +220 },  // 1.0e+066
    { 0xBDE94E8E43D0C8ED,  +223 },  // 1.0e+067
    { 0xED63A231D4C4FB28,  +226 },  // 1.0e+068
    { 0x945E455F24FB1CF9,  +230 },  // 1.0e+069
    { 0xB975D6B6EE39E437,  +233 },  // 1.0e+070
    { 0xE7D34C64A9C85D45,  +236 },  // 1.0e+071
    { 0x90E40FBEEA1D3A4B,  +240 },  // 1.0e+072
    { 0xB51D13AEA4A488DE,  +243 },  // 1.0e+073
    { 0xE264589A4DCDAB15,  +246 },  // 1.0e+074
    { 0x8D7EB76070A08AED,  +250 },  // 1.0e+075
    { 0xB0DE65388CC8ADA9,  +253 },  // 1.0e+076
    { 0xDD15FE86AFFAD913,  +256 },  // 1.0e+077
    { 0x8A2DBF142DFCC7AC,  +260 },  // 1.0e+078
    { 0xACB92ED9397BF997,  +263 },  // 1.0e+079
    { 0xD7E77A8F87DAF7FC,  +266 },  // 1.0e+080
    { 0x86F0AC99B4E8DAFE,  +270 },  // 1.0e+081
    { 0xA8ACD7C0222311BD,  +273 },  // 1.0e+082
    { 0xD2D80DB02AABD62C,  +276 },  // 1.0e+083
    { 0x83C7088E1AAB65DC,  +280 },  // 1.0e+084
    { 0xA4B8CAB1A1563F53,  +283 },  // 1.0e+085
    { 0xCDE6FD5E09ABCF27,  +286 },  // 1.0e+086
    { 0x80B05E5AC60B6179,  +290 },  // 1.0e+087
    { 0xA0DC75F1778E39D7,  +293 },  // 1.0e+088
    { 0xC913936DD571C84D,  +296 },  // 1.0e+089
    { 0xFB5878494ACE3A60,  +299 },  // 1.0e+090
    { 0x9D174B2DCEC0E47C,  +303 },  // 1.0e+091
    { 0xC45D1DF942711D9B,  +306 },  // 1.0e+092
    { 0xF5746577930D6501,  +309 },  // 1.0e+093
    { 0x9968BF6ABBE85F21,  +313 },  // 1.0e+094
    { 0xBFC2EF456AE276E9,  +316 },  // 1.0e+095
    { 0xEFB3AB16C59B14A3,  +319 },  // 1.0e+096
    { 0x95D04AEE3B80ECE6,  +323 },  // 1.0e+097
    { 0xBB445DA9CA612820,  +326 },  // 1.0e+098
    { 0xEA1575143CF97227,  +329 },  // 1.0e+099
    { 0x924D692CA61BE759,  +333 },  // 1.0e+100
    { 0xB6E0C377CFA2E12F,  +336 },  // 1.0e+101
    { 0xE498F455C38B997B,  +339 },  // 1.0e+102
    { 0x8EDF98B59A373FED,  +343 },  // 1.0e+103
    { 0xB2977EE300C50FE8,  +346 },  // 1.0e+104
    { 0xDF3D5E9BC0F653E2,  +349 },  // 1.0e+105
    { 0x8B865B215899F46D,  +353 },  // 1.0e+106
    { 0xAE67F1E9AEC07188,  +356 },  // 1.0e+107
    { 0xDA01EE641A708DEA,  +359 },  // 1.0e+108
    { 0x884134FE908658B3,  +363 },  // 1.0e+109
    { 0xAA51823E34A7EEDF,  +366 },  // 1.0e+110
    { 0xD4E5E2CDC1D1EA97,  +369 },  // 1.0e+111
    { 0x850FADC09923329F,  +373 },  // 1.0e+112
    { 0xA6539930BF6BFF46,  +376 },  // 1.0e+113
    { 0xCFE87F7CEF46FF17,  +379 },  // 1.0e+114
    { 0x81F14FAE158C5F6F,  +383 },  // 1.0e+115
    { 0xA26DA3999AEF774A,  +386 },  // 1.0e+116
    { 0xCB090C8001AB551D,  +389 },  // 1.0e+117
    { 0xFDCB4FA002162A64,  +392 },  // 1.0e+118
    { 0x9E9F11C4014DDA7F,  +396 },  // 1.0e+119
    { 0xC646D63501A1511E,  +399 },  // 1.0e+120
    { 0xF7D88BC24209A566,  +402 },  // 1.0e+121
    { 0x9AE7575969460760,  +406 },  // 1.0e+122
    { 0xC1A12D2FC3978938,  +409 },  // 1.0e+123
    { 0xF209787BB47D6B85,  +412 },  // 1.0e+124
    { 0x9745EB4D50CE6333,  +416 },  // 1.0e+125
    { 0xBD176620A501FC00,  +419 },  // 1.0e+126
    { 0xEC5D3FA8CE427B00,  +422 },  // 1.0e+127
    { 0x93BA47C980E98CE0,  +426 },  // 1.0e+128
    { 0xB8A8D9BBE123F018,  +429 },  // 1.0e+129
    { 0xE6D3102AD96CEC1E,  +432 },  // 1.0e+130
    { 0x9043EA1AC7E41393,  +436 },  // 1.0e+131
    { 0xB454E4A179DD1878,  +439 },  // 1.0e+132
    { 0xE16A1DC9D8545E95,  +442 },  // 1.0e+133
    { 0x8CE2529E2734BB1E,  +446 },  // 1.0e+134
    { 0xB01AE745B101E9E5,  +449 },  // 1.0e+135
    { 0xDC21A1171D42645E,  +452 },  // 1.0e+136
    { 0x899504AE72497EBB,  +456 },  // 1.0e+137
    { 0xABFA45DA0EDBDE6A,  +459 },  // 1.0e+138
    { 0xD6F8D7509292D604,  +462 },  // 1.0e+139
    { 0x865B86925B9BC5C3,  +466 },  // 1.0e+140
    { 0xA7F26836F282B733,  +469 },  // 1.0e+141
    { 0xD1EF0244AF236500,  +472 },  // 1.0e+142
    { 0x8335616AED761F20,  +476 },  // 1.0e+143
    { 0xA402B9C5A8D3A6E8,  +479 },  // 1.0e+144
    { 0xCD036837130890A2,  +482 },  // 1.0e+145
    { 0x802221226BE55A65,  +486 },  // 1.0e+146
    { 0xA02AA96B06DEB0FE,  +489 },  // 1.0e+147
    { 0xC83553C5C8965D3E,  +492 },  // 1.0e+148
    { 0xFA42A8B73ABBF48D,  +495 },  // 1.0e+149
    { 0x9C69A97284B578D8,  +499 },  // 1.0e+150
    { 0xC38413CF25E2D70E,  +502 },  // 1.0e+151
    { 0xF46518C2EF5B8CD2,  +505 },  // 1.0e+152
    { 0x98BF2F79D5993803,  +509 },  // 1.0e+153
    { 0xBEEEFB584AFF8604,  +512 },  // 1.0e+154
    { 0xEEAABA2E5DBF6785,  +515 },  // 1.0e+155
    { 0x952AB45CFA97A0B3,  +519 },  // 1.0e+156
    { 0xBA756174393D88E0,  +522 },  // 1.0e+157
    { 0xE912B9D1478CEB18,  +525 },  // 1.0e+158
    { 0x91ABB422CCB812EF,  +529 },  // 1.0e+159
    { 0xB616A12B7FE617AB,  +532 },  // 1.0e+160
    { 0xE39C49765FDF9D95,  +535 },  // 1.0e+161
    { 0x8E41ADE9FBEBC27E,  +539 },  // 1.0e+162
    { 0xB1D219647AE6B31D,  +542 },  // 1.0e+163
    { 0xDE469FBD99A05FE4,  +545 },  // 1.0e+164
    { 0x8AEC23D680043BEF,  +549 },  // 1.0e+165
    { 0xADA72CCC20054AEA,  +552 },  // 1.0e+166
    { 0xD910F7FF28069DA5,  +555 },  // 1.0e+167
    { 0x87AA9AFF79042287,  +559 },  // 1.0e+168
    { 0xA99541BF57452B29,  +562 },  // 1.0e+169
    { 0xD3FA922F2D1675F3,  +565 },  // 1.0e+170
    { 0x847C9B5D7C2E09B8,  +569 },  // 1.0e+171
    { 0xA59BC234DB398C26,  +572 },  // 1.0e+172
    { 0xCF02B2C21207EF2F,  +575 },  // 1.0e+173
    { 0x8161AFB94B44F57E,  +579 },  // 1.0e+174
    { 0xA1BA1BA79E1632DD,  +582 },  // 1.0e+175
    { 0xCA28A291859BBF94,  +585 },  // 1.0e+176
    { 0xFCB2CB35E702AF79,  +588 },  // 1.0e+177
    { 0x9DEFBF01B061ADAC,  +592 },  // 1.0e+178
    { 0xC56BAEC21C7A1917,  +595 },  // 1.0e+179
    { 0xF6C69A72A3989F5C,  +598 },  // 1.0e+180
    { 0x9A3C2087A63F639A,  +602 },  // 1.0e+181
    { 0xC0CB28A98FCF3C80,  +605 },  // 1.0e+182
    { 0xF0FDF2D3F3C30BA0,  +608 },  // 1.0e+183
    { 0x969EB7C47859E744,  +612 },  // 1.0e+184
    { 0xBC4665B596706115,  +615 },  // 1.0e+185
    { 0xEB57FF22FC0C795A,  +618 },  // 1.0e+186
    { 0x9316FF75DD87CBD9,  +622 },  // 1.0e+187
    { 0xB7DCBF5354E9BECF,  +625 },  // 1.0e+188
    { 0xE5D3EF282A242E82,  +628 },  // 1.0e+189
    { 0x8FA475791A569D11,  +632 },  // 1.0e+190
    { 0xB38D92D760EC4456,  +635 },  // 1.0e+191
    { 0xE070F78D3927556B,  +638 },  // 1.0e+192
    { 0x8C469AB843B89563,  +642 },  // 1.0e+193
    { 0xAF58416654A6BABC,  +645 },  // 1.0e+194
    { 0xDB2E51BFE9D0696B,  +648 },  // 1.0e+195
    { 0x88FCF317F22241E3,  +652 },  // 1.0e+196
    { 0xAB3C2FDDEEAAD25B,  +655 },  // 1.0e+197
    { 0xD60B3BD56A5586F2,  +658 },  // 1.0e+198
    { 0x85C7056562757457,  +662 },  // 1.0e+199
    { 0xA738C6BEBB12D16D,  +665 },  // 1.0e+200
    { 0xD106F86E69D785C8,  +668 },  // 1.0e+201
    { 0x82A45B450226B39D,  +672 },  // 1.0e+202
    { 0xA34D721642B06085,  +675 },  // 1.0e+203
    { 0xCC20CE9BD35C78A6,  +678 },  // 1.0e+204
    { 0xFF290242C83396CF,  +681 },  // 1.0e+205
    { 0x9F79A169BD203E42,  +685 },  // 1.0e+206
    { 0xC75809C42C684DD2,  +688 },  // 1.0e+207
    { 0xF92E0C3537826146,  +691 },  // 1.0e+208
    { 0x9BBCC7A142B17CCC,  +695 },  // 1.0e+209
    { 0xC2ABF989935DDBFF,  +698 },  // 1.0e+210
    { 0xF356F7EBF83552FF,  +701 },  // 1.0e+211
    { 0x98165AF37B2153DF,  +705 },  // 1.0e+212
    { 0xBE1BF1B059E9A8D7,  +708 },  // 1.0e+213
    { 0xEDA2EE1C7064130D,  +711 },  // 1.0e+214
    { 0x9485D4D1C63E8BE8,  +715 },  // 1.0e+215
    { 0xB9A74A0637CE2EE2,  +718 },  // 1.0e+216
    { 0xE8111C87C5C1BA9A,  +721 },  // 1.0e+217
    { 0x910AB1D4DB9914A1,  +725 },  // 1.0e+218
    { 0xB54D5E4A127F59C9,  +728 },  // 1.0e+219
    { 0xE2A0B5DC971F303B,  +731 },  // 1.0e+220
    { 0x8DA471A9DE737E25,  +735 },  // 1.0e+221
    { 0xB10D8E1456105DAE,  +738 },  // 1.0e+222
    { 0xDD50F1996B947519,  +741 },  // 1.0e+223
    { 0x8A5296FFE33CC930,  +745 },  // 1.0e+224
    { 0xACE73CBFDC0BFB7C,  +748 },  // 1.0e+225
    { 0xD8210BEFD30EFA5B,  +751 },  // 1.0e+226
    { 0x8714A775E3E95C79,  +755 },  // 1.0e+227
    { 0xA8D9D1535CE3B397,  +758 },  // 1.0e+228
    { 0xD31045A8341CA07D,  +761 },  // 1.0e+229
    { 0x83EA2B892091E44E,  +765 },  // 1.0e+230
    { 0xA4E4B66B68B65D61,  +768 },  // 1.0e+231
    { 0xCE1DE40642E3F4BA,  +771 },  // 1.0e+232
    { 0x80D2AE83E9CE78F4,  +775 },  // 1.0e+233
    { 0xA1075A24E4421731,  +778 },  // 1.0e+234
    { 0xC94930AE1D529CFD,  +781 },  // 1.0e+235
    { 0xFB9B7CD9A4A7443D,  +784 },  // 1.0e+236
    { 0x9D412E0806E88AA6,  +788 },  // 1.0e+237
    { 0xC491798A08A2AD4F,  +791 },  // 1.0e+238
    { 0xF5B5D7EC8ACB58A3,  +794 },  // 1.0e+239
    { 0x9991A6F3D6BF1766,  +798 },  // 1.0e+240
    { 0xBFF610B0CC6EDD40,  +801 },  // 1.0e+241
    { 0xEFF394DCFF8A948F,  +804 },  // 1.0e+242
    { 0x95F83D0A1FB69CDA,  +808 },  // 1.0e+243
    { 0xBB764C4CA7A44410,  +811 },  // 1.0e+244
    { 0xEA53DF5FD18D5514,  +814 },  // 1.0e+245
    { 0x92746B9BE2F8552D,  +818 },  // 1.0e+246
    { 0xB7118682DBB66A78,  +821 },  // 1.0e+247
    { 0xE4D5E82392A40516,  +824 },  // 1.0e+248
    { 0x8F05B1163BA6832E,  +828 },  // 1.0e+249
    { 0xB2C71D5BCA9023F9,  +831 },  // 1.0e+250
    { 0xDF78E4B2BD342CF7,  +834 },  // 1.0e+251
    { 0x8BAB8EEFB6409C1B,  +838 },  // 1.0e+252
    { 0xAE9672ABA3D0C321,  +841 },  // 1.0e+253
    { 0xDA3C0F568CC4F3E9,  +844 },  // 1.0e+254
    { 0x8865899617FB1872,  +848 },  // 1.0e+255
    { 0xAA7EEBFB9DF9DE8E,  +851 },  // 1.0e+256
    { 0xD51EA6FA85785632,  +854 },  // 1.0e+257
    { 0x8533285C936B35DF,  +858 },  // 1.0e+258
    { 0xA67FF273B8460357,  +861 },  // 1.0e+259
    { 0xD01FEF10A657842D,  +864 },  // 1.0e+260
    { 0x8213F56A67F6B29C,  +868 },  // 1.0e+261
    { 0xA298F2C501F45F43,  +871 },  // 1.0e+262
    { 0xCB3F2F7642717714,  +874 },  // 1.0e+263
    { 0xFE0EFB53D30DD4D8,  +877 },  // 1.0e+264
    { 0x9EC95D1463E8A507,  +881 },  // 1.0e+265
    { 0xC67BB4597CE2CE49,  +884 },  // 1.0e+266
    { 0xF81AA16FDC1B81DB,  +887 },  // 1.0e+267
    { 0x9B10A4E5E9913129,  +891 },  // 1.0e+268
    { 0xC1D4CE1F63F57D73,  +894 },  // 1.0e+269
    { 0xF24A01A73CF2DCD0,  +897 },  // 1.0e+270
    { 0x976E41088617CA02,  +901 },  // 1.0e+271
    { 0xBD49D14AA79DBC83,  +904 },  // 1.0e+272
    { 0xEC9C459D51852BA3,  +907 },  // 1.0e+273
    { 0x93E1AB8252F33B46,  +911 },  // 1.0e+274
    { 0xB8DA1662E7B00A18,  +914 },  // 1.0e+275
    { 0xE7109BFBA19C0C9E,  +917 },  // 1.0e+276
    { 0x906A617D450187E3,  +921 },  // 1.0e+277
    { 0xB484F9DC9641E9DB,  +924 },  // 1.0e+278
    { 0xE1A63853BBD26452,  +927 },  // 1.0e+279
    { 0x8D07E33455637EB3,  +931 },  // 1.0e+280
    { 0xB049DC016ABC5E60,  +934 },  // 1.0e+281
    { 0xDC5C5301C56B75F8,  +937 },  // 1.0e+282
    { 0x89B9B3E11B6329BB,  +941 },  // 1.0e+283
    { 0xAC2820D9623BF42A,  +944 },  // 1.0e+284
    { 0xD732290FBACAF134,  +947 },  // 1.0e+285
    { 0x867F59A9D4BED6C1,  +951 },  // 1.0e+286
    { 0xA81F301449EE8C71,  +954 },  // 1.0e+287
    { 0xD226FC195C6A2F8D,  +957 },  // 1.0e+288
    { 0x83585D8FD9C25DB8,  +961 },  // 1.0e+289
    { 0xA42E74F3D032F526,  +964 },  // 1.0e+290
    { 0xCD3A1230C43FB270,  +967 },  // 1.0e+291
    { 0x80444B5E7AA7CF86,  +971 },  // 1.0e+292
    { 0xA0555E361951C367,  +974 },  // 1.0e+293
    { 0xC86AB5C39FA63441,  +977 },  // 1.0e+294
    { 0xFA856334878FC151,  +980 },  // 1.0e+295
    { 0x9C935E00D4B9D8D3,  +984 },  // 1.0e+296
    { 0xC3B8358109E84F08,  +987 },  // 1.0e+297
    { 0xF4A642E14C6262C9,  +990 },  // 1.0e+298
    { 0x98E7E9CCCFBD7DBE,  +994 },  // 1.0e+299
    { 0xBF21E44003ACDD2D,  +997 },  // 1.0e+300
    { 0xEEEA5D5004981479, +1000 },  // 1.0e+301
    { 0x95527A5202DF0CCC, +1004 },  // 1.0e+302
    { 0xBAA718E68396CFFE, +1007 },  // 1.0e+303
    { 0xE950DF20247C83FE, +1010 },  // 1.0e+304
    { 0x91D28B7416CDD27F, +1014 },  // 1.0e+305
    { 0xB6472E511C81471E, +1017 },  // 1.0e+306
    { 0xE3D8F9E563A198E6, +1020 },  // 1.0e+307
    { 0x8E679C2F5E44FF90, +1024 },  // 1.0e+308
  };

// `exp10 = FLOOR(exp2 * LOG2)` where `LOG2 = 0.30102999`
constexpr int s_decimal_exp_min = s_decimal_multipliers[0].exp2 * 30102999LL / 100000000LL - 1;

template<typename valueT>
inline
bool
do_is_value_out_of_range(valueT& value, valueT min, valueT max)
  {
    if(value < min) {
      // `value` is less than `min` and `min` is not a NaN.
      value = min;
      return true;  // overflowed
    }
    else if(value > max) {
      // `value` is greater than `max` and `max` is not a NaN.
      value = max;
      return true;  // overflowed
    }
    else
      return false;
  }

const uint32_t ss_zero [2] = { 0b0'00000000'0UL << 22, 0b1'00000000'0UL << 22 };
const uint32_t ss_inf  [2] = { 0b0'11111111'0UL << 22, 0b1'11111111'0UL << 22 };
const uint32_t ss_qnan [2] = { 0b0'11111111'1UL << 22, 0b1'11111111'1UL << 22 };

const uint64_t sd_zero [2] = { 0b0'00000000000'0ULL << 51, 0b1'00000000000'0ULL << 51 };
const uint64_t sd_inf  [2] = { 0b0'11111111111'0ULL << 51, 0b1'11111111111'0ULL << 51 };
const uint64_t sd_qnan [2] = { 0b0'11111111111'1ULL << 51, 0b1'11111111111'1ULL << 51 };

}  // namespace

size_t
ascii_numget::
parse_TB(const char* str, size_t len) noexcept
  {
    const char* const eptr = str + len;
    const char* rptr = str;

    this->m_cls = value_class_finite;
    this->m_base = 2;
    this->m_exp = 0;
    this->m_more = false;

    if(do_subseq(rptr, eptr, "true")) {
      // `true` is treated as the integer negative one.
      this->m_sign = 1;
      this->m_mant = 1;
    }
    else if(do_subseq(rptr, eptr, "false")) {
      // `false` is treated as the integer zero.
      this->m_sign = 0;
      this->m_mant = 0;
    }
    else
      return 0;

    return (size_t) (rptr - str);
  }

size_t
ascii_numget::
parse_BU(const char* str, size_t len) noexcept
  {
    const char* const eptr = str + len;
    const char* rptr = str;

    this->m_sign = 0;
    this->m_cls = value_class_finite;

    do_skip_base_prefix(rptr, eptr, 'b');

    // Get at least one significant digit. The entire string will not
    // be otherwise accepted.
    xuint xu = do_collect_digits(rptr, eptr, 2, -1);
    if(!xu.valid)
      return 0;

    // Copy all digits from `xu`.
    this->m_base = 2;
    this->m_exp = xu.exp;
    this->m_mant = xu.mant;
    this->m_more = xu.more;

    return (size_t) (rptr - str);
  }

size_t
ascii_numget::
parse_XU(const char* str, size_t len) noexcept
  {
    const char* const eptr = str + len;
    const char* rptr = str;

    this->m_sign = 0;
    this->m_cls = value_class_finite;

    do_skip_base_prefix(rptr, eptr, 'x');

    // Get at least one significant digit. The entire string will not
    // be otherwise accepted.
    xuint xu = do_collect_digits(rptr, eptr, 16, -1);
    if(!xu.valid)
      return 0;

    // Copy all digits from `xu`.
    this->m_base = 2;
    this->m_exp = xu.exp;
    this->m_mant = xu.mant;
    this->m_more = xu.more;

    return (size_t) (rptr - str);
  }

size_t
ascii_numget::
parse_DU(const char* str, size_t len) noexcept
  {
    const char* const eptr = str + len;
    const char* rptr = str;

    this->m_sign = 0;
    this->m_cls = value_class_finite;

    // Get at least one significant digit. The entire string will not
    // be otherwise accepted.
    xuint xu = do_collect_digits(rptr, eptr, 10, -1);
    if(!xu.valid)
      return 0;

    // Copy all digits from `xu`.
    this->m_base = 10;
    this->m_exp = xu.exp;
    this->m_mant = xu.mant;
    this->m_more = xu.more;

    return (size_t) (rptr - str);
  }

size_t
ascii_numget::
parse_BI(const char* str, size_t len) noexcept
  {
    const char* const eptr = str + len;
    const char* rptr = str;

    this->m_sign = do_get_sign(rptr, eptr);
    this->m_cls = value_class_finite;

    do_skip_base_prefix(rptr, eptr, 'b');

    // Get at least one significant digit. The entire string will not
    // be otherwise accepted.
    xuint xu = do_collect_digits(rptr, eptr, 2, -1);
    if(!xu.valid)
      return 0;

    // Copy all digits from `xu`.
    this->m_base = 2;
    this->m_exp = xu.exp;
    this->m_mant = xu.mant;
    this->m_more = xu.more;

    return (size_t) (rptr - str);
  }

size_t
ascii_numget::
parse_XI(const char* str, size_t len) noexcept
  {
    const char* const eptr = str + len;
    const char* rptr = str;

    this->m_sign = do_get_sign(rptr, eptr);
    this->m_cls = value_class_finite;

    do_skip_base_prefix(rptr, eptr, 'x');

    // Get at least one significant digit. The entire string will not
    // be otherwise accepted.
    xuint xu = do_collect_digits(rptr, eptr, 16, -1);
    if(!xu.valid)
      return 0;

    // Copy all digits from `xu`.
    this->m_base = 2;
    this->m_exp = xu.exp;
    this->m_mant = xu.mant;
    this->m_more = xu.more;

    return (size_t) (rptr - str);
  }

size_t
ascii_numget::
parse_DI(const char* str, size_t len) noexcept
  {
    const char* const eptr = str + len;
    const char* rptr = str;

    this->m_sign = do_get_sign(rptr, eptr);
    this->m_cls = value_class_finite;

    // Get at least one significant digit. The entire string will not
    // be otherwise accepted.
    xuint xu = do_collect_digits(rptr, eptr, 10, -1);
    if(!xu.valid)
      return 0;

    // Copy all digits from `xu`.
    this->m_base = 10;
    this->m_exp = xu.exp;
    this->m_mant = xu.mant;
    this->m_more = xu.more;

    return (size_t) (rptr - str);
  }

size_t
ascii_numget::
parse_BD(const char* str, size_t len) noexcept
  {
    const char* const eptr = str + len;
    const char* rptr = str;

    this->m_sign = do_get_sign(rptr, eptr);
    this->m_cls = do_get_special_value(rptr, eptr);

    if(this->m_cls != value_class_finite)
      return (size_t) (rptr - str);

    do_skip_base_prefix(rptr, eptr, 'b');

    // Get at least one significant digit. The entire string will not
    // be otherwise accepted.
    xuint xu = do_collect_digits(rptr, eptr, 2, this->m_rdxp);
    if(!xu.valid)
      return 0;

    // Copy all digits from `xu`.
    this->m_base = 2;
    this->m_exp = xu.exp;
    this->m_mant = xu.mant;
    this->m_more = xu.more;

    return (size_t) (rptr - str);
  }

size_t
ascii_numget::
parse_XD(const char* str, size_t len) noexcept
  {
    const char* const eptr = str + len;
    const char* rptr = str;

    this->m_sign = do_get_sign(rptr, eptr);
    this->m_cls = do_get_special_value(rptr, eptr);

    if(this->m_cls != value_class_finite)
      return (size_t) (rptr - str);

    do_skip_base_prefix(rptr, eptr, 'x');

    // Get at least one significant digit. The entire string will not
    // be otherwise accepted.
    xuint xu = do_collect_digits(rptr, eptr, 16, this->m_rdxp);
    if(!xu.valid)
      return 0;

    // Copy all digits from `xu`.
    this->m_base = 2;
    this->m_exp = xu.exp;
    this->m_mant = xu.mant;
    this->m_more = xu.more;

    return (size_t) (rptr - str);
  }

size_t
ascii_numget::
parse_DD(const char* str, size_t len) noexcept
  {
    const char* const eptr = str + len;
    const char* rptr = str;

    this->m_sign = do_get_sign(rptr, eptr);
    this->m_cls = do_get_special_value(rptr, eptr);

    if(this->m_cls != value_class_finite)
      return (size_t) (rptr - str);

    // Get at least one significant digit. The entire string will not
    // be otherwise accepted.
    xuint xu = do_collect_digits(rptr, eptr, 10, this->m_rdxp);
    if(!xu.valid)
      return 0;

    // Copy all digits from `xu`.
    this->m_base = 10;
    this->m_exp = xu.exp;
    this->m_mant = xu.mant;
    this->m_more = xu.more;

    return (size_t) (rptr - str);
  }

size_t
ascii_numget::
parse_U(const char* str, size_t len) noexcept
  {
    if((len >= 2) && (str[0] == '0')) {
      // Check the literal prefix. `0b` denotes binary and `0x` denotes
      // hexadecimal. Prefixes are case-insensitive.
      if((str[1] | 0x20) == 'b')
        return this->parse_BU(str, len);

      if((str[1] | 0x20) == 'x')
        return this->parse_XU(str, len);
    }

    // Assume decimal.
    return this->parse_DU(str, len);
  }

size_t
ascii_numget::
parse_I(const char* str, size_t len) noexcept
  {
    if((len >= 2) && (str[0] == '0')) {
      // Check the literal prefix. `0b` denotes binary and `0x` denotes
      // hexadecimal. Prefixes are case-insensitive.
      if((str[1] | 0x20) == 'b')
        return this->parse_BI(str, len);

      if((str[1] | 0x20) == 'x')
        return this->parse_XI(str, len);
    }

    if((len >= 3) && ((str[0] == '+') || (str[0] == '-')) && (str[1] == '0')) {
      // Check the literal prefix. `0b` denotes binary and `0x` denotes
      // hexadecimal. Prefixes are case-insensitive.
      if((str[2] | 0x20) == 'b')
        return this->parse_BI(str, len);

      if((str[2] | 0x20) == 'x')
        return this->parse_XI(str, len);
    }

    // Assume decimal.
    return this->parse_DI(str, len);
  }

size_t
ascii_numget::
parse_D(const char* str, size_t len) noexcept
  {
    if((len >= 2) && (str[0] == '0')) {
      // Check the literal prefix. `0b` denotes binary and `0x` denotes
      // hexadecimal. Prefixes are case-insensitive.
      if((str[1] | 0x20) == 'b')
        return this->parse_BD(str, len);

      if((str[1] | 0x20) == 'x')
        return this->parse_XD(str, len);
    }

    if((len >= 3) && ((str[0] == '+') || (str[0] == '-')) && (str[1] == '0')) {
      // Check the literal prefix. `0b` denotes binary and `0x` denotes
      // hexadecimal. Prefixes are case-insensitive.
      if((str[2] | 0x20) == 'b')
        return this->parse_BD(str, len);

      if((str[2] | 0x20) == 'x')
        return this->parse_XD(str, len);
    }

    // Assume decimal.
    return this->parse_DD(str, len);
  }

void
ascii_numget::
cast_U(uint64_t& value, uint64_t min, uint64_t max) noexcept
  {
    switch(this->m_cls) {
      case value_class_empty:
        // Zero is implied for convenience, but this operation fails.
        this->m_ovfl = true;
        this->m_udfl = false;
        this->m_inxct = false;
        value = 0;
        break;

      case value_class_infinitesimal:
        // Negative values overflow. Positive values are truncated.
        this->m_ovfl = this->m_sign;
        this->m_udfl = !this->m_sign;
        this->m_inxct = true;
        value = 0;
        break;

      case value_class_infinity:
        // All values overflow.
        this->m_ovfl = true;
        this->m_udfl = false;
        this->m_inxct = false;
        value = this->m_sign ? 0ULL : UINT64_MAX;
        break;

      case value_class_nan:
        // All values overflow.
        this->m_ovfl = true;
        this->m_udfl = false;
        this->m_inxct = false;
        value = -(uint64_t) INT64_MIN;
        break;

      default:
        // The value is finite, so cast it.
        this->m_ovfl = false;
        this->m_udfl = false;
        this->m_inxct = this->m_more;

        if(this->m_mant == 0) {
          value = 0;
          break;
        }
        else if(this->m_sign) {
          this->m_ovfl = true;
          this->m_inxct = true;
          value = 0;
          break;
        }
        else if(this->m_exp < -10000) {
          this->m_udfl = true;
          this->m_inxct = true;
          value = 0;
          break;
        }
        else if(this->m_exp > 10000) {
          this->m_ovfl = true;
          value = UINT64_MAX;
          break;
        }

        int exp = (int) this->m_exp;
        value = this->m_mant;

        if(exp > 0) {
          // Raise the mantissa.
          do {
            exp --;
            this->m_ovfl |= ROCKET_MUL_OVERFLOW(value, this->m_base, &value);
          }
          while((exp != 0) && (this->m_ovfl == false));

          if(this->m_ovfl) {
            value = UINT64_MAX;
            break;
          }
        }
        else if(exp < 0) {
          // Lower the mantissa.
          do {
            exp ++;
            this->m_inxct |= (value % this->m_base) != 0;
            value /= this->m_base;
          }
          while((exp != 0) && (value != 0));

          if(value == 0) {
            this->m_udfl = true;
            break;
          }
        }

        ROCKET_ASSERT(value != 0);
        break;
    }

    this->m_ovfl |= do_is_value_out_of_range(value, min, max);
  }

void
ascii_numget::
cast_I(int64_t& value, int64_t min, int64_t max) noexcept
  {
    switch(this->m_cls) {
      case value_class_empty:
        // Zero is implied for convenience, but this operation fails.
        this->m_ovfl = true;
        this->m_udfl = false;
        this->m_inxct = false;
        value = 0;
        break;

      case value_class_infinitesimal:
        // All values are truncated.
        this->m_ovfl = false;
        this->m_udfl = true;
        this->m_inxct = true;
        value = 0;
        break;

      case value_class_infinity:
        // All values overflow.
        this->m_ovfl = true;
        this->m_udfl = false;
        this->m_inxct = false;
        value = this->m_sign ? INT64_MIN : INT64_MAX;
        break;

      case value_class_nan:
        // All values overflow.
        this->m_ovfl = true;
        this->m_udfl = false;
        this->m_inxct = false;
        value = INT64_MIN;
        break;

      default:
        // The value is finite, so cast it.
        this->m_ovfl = false;
        this->m_udfl = false;
        this->m_inxct = this->m_more;

        if(this->m_mant == 0) {
          value = 0;
          break;
        }
        else if(this->m_exp < -10000) {
          this->m_udfl = true;
          this->m_inxct = true;
          value = 0;
          break;
        }
        else if(this->m_exp > 10000) {
          this->m_ovfl = true;
          value = INT64_MAX;
          break;
        }

        int exp = (int) this->m_exp;
        uint64_t bits = this->m_mant;
        uint64_t umax = this->m_sign ? -(uint64_t) INT64_MIN : INT64_MAX;

        if(exp > 0) {
          // Raise the mantissa.
          do {
            exp --;
            this->m_ovfl |= ROCKET_MUL_OVERFLOW(bits, this->m_base, &bits);
          }
          while((exp != 0) && (this->m_ovfl == false));

          if(this->m_ovfl) {
            value = (int64_t) umax;
            break;
          }
        }
        else if(exp < 0) {
          // Lower the mantissa.
          do {
            exp ++;
            this->m_inxct |= (bits % this->m_base) != 0;
            bits /= this->m_base;
          }
          while((exp != 0) && (bits != 0));

          if(bits == 0) {
            this->m_udfl = true;
            value = 0;
            break;
          }
        }

        if(bits > umax) {
          this->m_ovfl = true;
          value = (int64_t) umax;
          break;
        }

        value = (int64_t) ((bits - umax + INT64_MAX) ^ INT64_MAX ^ umax);
        ROCKET_ASSERT(value != 0);
        break;
    }

    this->m_ovfl |= do_is_value_out_of_range(value, min, max);
  }

void
ascii_numget::
cast_F(float& value, float min, float max) noexcept
  {
    switch(this->m_cls) {
      case value_class_empty:
        // Zero is implied for convenience, but this operation fails.
        this->m_ovfl = true;
        this->m_udfl = false;
        this->m_inxct = false;
        value = 0;
        break;

      case value_class_infinitesimal:
        // All values are truncated.
        this->m_ovfl = false;
        this->m_udfl = true;
        this->m_inxct = true;
        ::memcpy(&value, ss_zero + this->m_sign, sizeof(float));
        break;

      case value_class_infinity:
        // All values overflow.
        this->m_ovfl = true;
        this->m_udfl = false;
        this->m_inxct = false;
        ::memcpy(&value, ss_inf + this->m_sign, sizeof(float));
        break;

      case value_class_nan:
        // All values overflow.
        this->m_ovfl = true;
        this->m_udfl = false;
        this->m_inxct = false;
        ::memcpy(&value, ss_qnan + this->m_sign, sizeof(float));
        break;

      default:
        // The value is finite, so cast it.
        this->m_ovfl = false;
        this->m_udfl = false;
        this->m_inxct = this->m_more;

        if(this->m_mant == 0) {
          ::memcpy(&value, ss_zero + this->m_sign, sizeof(float));
          break;
        }
        else if(this->m_exp < -10000) {
          this->m_udfl = true;
          this->m_inxct = true;
          ::memcpy(&value, ss_zero + this->m_sign, sizeof(float));
          break;
        }
        else if(this->m_exp > 10000) {
          this->m_ovfl = true;
          ::memcpy(&value, ss_inf + this->m_sign, sizeof(float));
          break;
        }

        // Align the mantissa to the left, so its MSB is non-zero.
        int exp = (int) this->m_exp;
        int sh = ROCKET_LZCNT64(this->m_mant);
        uint32_t bits = (uint32_t) (this->m_mant << sh);  // lower
        this->m_inxct |= bits != 0;
        bits = (uint32_t) ((this->m_mant << sh) >> 32) | (bits >> 31); // upper

        if(this->m_base == 10) {
          // Convert the base-10 exponent to a base-2 exponent.
          if(exp < s_decimal_exp_min) {
            this->m_udfl = true;
            this->m_inxct = true;
            ::memcpy(&value, ss_zero + this->m_sign, sizeof(float));
            break;
          }
          else if(exp >= s_decimal_exp_min + (int) noadl::size(s_decimal_multipliers)) {
            this->m_ovfl = true;
            ::memcpy(&value, ss_inf + this->m_sign, sizeof(float));
            break;
          }

          const auto& mult = s_decimal_multipliers[(uint32_t) (exp - s_decimal_exp_min)];
          exp = mult.exp2;
          uint64_t ceiled_mult_mant = (mult.mant + UINT32_MAX) >> 32;
          bits = (uint32_t) (bits * ceiled_mult_mant >> 32);

          // Re-align the mantissa, so its MSB is non-zero.
          int rsh = ROCKET_LZCNT32(bits);
          bits <<= rsh;
          sh += rsh;
        }

        // Get the biased base-2 exponent.
        exp = exp - sh + 190;

        if(ROCKET_EXPECT(exp >= 1)) {
          // This value is at least normal, so round the mantissa.
          constexpr uint32_t ulp = 1U << 8;
          uint32_t bits_out = bits & (ulp - 1);
          this->m_inxct |= (bits_out != 0) && (bits_out != (ulp - 1));
          bits_out <<= 1;
          bits_out |= this->m_more;

          // Round the mantissa to even. If rounding effects a carry,
          // the exponent shall be incremented.
          bits >>= 8;
          bits += (bits_out >= ulp) && (((ulp - bits_out) >> 31) | (bits & 1));
          sh = (int) (bits >> 24) & 1;
          exp += sh;

          if(exp > 254) {
            this->m_ovfl = true;
            ::memcpy(&value, ss_inf + this->m_sign, sizeof(float));
            break;
          }

          // If the exponent has been incremented, nudge the mantissa.
          bits >>= sh;
          bits &= (1U << 23) - 1;
        }
        else if(exp >= -22) {
          // This value is subnormal, so round the mantissa.
          sh = 9 - exp;
          uint32_t ulp = 1U << sh;
          uint32_t bits_out = bits & (ulp - 1);
          this->m_inxct |= (bits_out != 0) && (bits_out != (ulp - 1));
          bits_out <<= 1;
          bits_out |= this->m_more;

          // Round the mantissa to even. If rounding effects a carry,
          // the exponent shall be incremented.
          bits >>= sh;
          bits += (bits_out >= ulp) && (((ulp - bits_out) >> 31) | (bits & 1));
          sh = (int) (bits >> 23) & 1;
          exp += sh;

          // If the exponent has been incremented and the result is still
          // subnormal, nudge the mantissa. In other words, if the result
          // will have been normalized because the hidden bit will change
          // from zero to one, the other bits of the mantissa will not
          // change, and is not therefore nudged.
          bits >>= sh & (exp >> 15);
          bits &= (1U << 23) - 1;
        }
        else if(exp == -23) {
          // This is a special case. All bits are below the minimum
          // subnormal value. The only chance that this very tiny value
          // will become representable is when a carry happens. That is,
          // the mantissa is at least 0x8000000000000001. Being equal to
          // it will not work, as it's rounded to even.
          this->m_inxct = true;

          if(bits <= 0x80000000U) {
            this->m_udfl = true;
            ::memcpy(&value, ss_zero + this->m_sign, sizeof(float));
            break;
          }

          // Accept the carry.
          bits = 1;
          exp = -22;
        }
        else {
          // The value is below subnormal.
          this->m_udfl = true;
          this->m_inxct = true;
          ::memcpy(&value, ss_zero + this->m_sign, sizeof(float));
          break;
        }

        // Compose the value.
        bits |= (uint32_t) this->m_sign << 31;
        bits |= (uint32_t) (exp & -exp >> 15) << 23;
        ::memcpy(&value, &bits, sizeof(float));
        ROCKET_ASSERT(value != 0);
        break;
    }

    this->m_ovfl |= do_is_value_out_of_range(value, min, max);
  }

void
ascii_numget::
cast_D(double& value, double min, double max) noexcept
  {
    switch(this->m_cls) {
      case value_class_empty:
        // Zero is implied for convenience, but this operation fails.
        this->m_ovfl = true;
        this->m_udfl = false;
        this->m_inxct = false;
        value = 0;
        break;

      case value_class_infinitesimal:
        // All values are truncated.
        this->m_ovfl = false;
        this->m_udfl = true;
        this->m_inxct = true;
        ::memcpy(&value, sd_zero + this->m_sign, sizeof(double));
        break;

      case value_class_infinity:
        // All values overflow.
        this->m_ovfl = true;
        this->m_udfl = false;
        this->m_inxct = false;
        ::memcpy(&value, sd_inf + this->m_sign, sizeof(double));
        break;

      case value_class_nan:
        // All values overflow.
        this->m_ovfl = true;
        this->m_udfl = false;
        this->m_inxct = false;
        ::memcpy(&value, sd_qnan + this->m_sign, sizeof(double));
        break;

      default:
        // The value is finite, so cast it.
        this->m_ovfl = false;
        this->m_udfl = false;
        this->m_inxct = this->m_more;

        if(this->m_mant == 0) {
          ::memcpy(&value, sd_zero + this->m_sign, sizeof(double));
          break;
        }
        else if(this->m_exp < -10000) {
          this->m_udfl = true;
          this->m_inxct = true;
          ::memcpy(&value, sd_zero + this->m_sign, sizeof(double));
          break;
        }
        else if(this->m_exp > 10000) {
          this->m_ovfl = true;
          ::memcpy(&value, sd_inf + this->m_sign, sizeof(double));
          break;
        }

        // Align the mantissa to the left, so its MSB is non-zero.
        int exp = (int) this->m_exp;
        int sh = ROCKET_LZCNT64(this->m_mant);
        uint64_t bits = this->m_mant << sh;

        if(this->m_base == 10) {
          // Convert the base-10 exponent to a base-2 exponent.
          //   `exp10 = TRUNC((exp2 - 1) * LOG2)` where `LOG2 = 0.30103`
          if(exp < s_decimal_exp_min) {
            this->m_udfl = true;
            this->m_inxct = true;
            ::memcpy(&value, sd_zero + this->m_sign, sizeof(double));
            break;
          }
          else if(exp >= s_decimal_exp_min + (int) noadl::size(s_decimal_multipliers)) {
            this->m_ovfl = true;
            ::memcpy(&value, sd_inf + this->m_sign, sizeof(double));
            break;
          }

          const auto& mult = s_decimal_multipliers[(uint32_t) (exp - s_decimal_exp_min)];
          exp = mult.exp2;
          bits = mulh128(bits, mult.mant);

          // Re-align the mantissa, so its MSB is non-zero.
          int rsh = ROCKET_LZCNT64(bits);
          bits <<= rsh;
          sh += rsh;
        }

        // Get the biased base-2 exponent.
        exp = exp - sh + 1086;

        if(ROCKET_EXPECT(exp >= 1)) {
          // This value is at least normal, so round the mantissa.
          constexpr uint64_t ulp = 1UL << 11;
          uint64_t bits_out = bits & (ulp - 1);
          this->m_inxct |= (bits_out != 0) && (bits_out != (ulp - 1));
          bits_out <<= 1;
          bits_out |= this->m_more;

          // Round the mantissa to even. If rounding effects a carry,
          // the exponent shall be incremented.
          bits >>= 11;
          bits += (bits_out >= ulp) && (((ulp - bits_out) >> 63) | (bits & 1));
          sh = (int) (bits >> 53) & 1;
          exp += sh;

          if(exp > 2046) {
            this->m_ovfl = true;
            ::memcpy(&value, sd_inf + this->m_sign, sizeof(double));
            break;
          }

          // If the exponent has been incremented, nudge the mantissa.
          bits >>= sh;
          bits &= (1ULL << 52) - 1;
        }
        else if(exp >= -51) {
          // This value is subnormal, so round the mantissa.
          sh = 12 - exp;
          uint64_t ulp = 1ULL << sh;
          uint64_t bits_out = bits & (ulp - 1);
          this->m_inxct |= (bits_out != 0) && (bits_out != (ulp - 1));
          bits_out <<= 1;
          bits_out |= this->m_more;

          // Round the mantissa to even. If rounding effects a carry,
          // the exponent shall be incremented.
          bits >>= sh;
          bits += (bits_out >= ulp) && (((ulp - bits_out) >> 63) | (bits & 1));
          sh = (int) (bits >> 52) & 1;
          exp += sh;

          // If the exponent has been incremented and the result is still
          // subnormal, nudge the mantissa. In other words, if the result
          // will have been normalized because the hidden bit will change
          // from zero to one, the other bits of the mantissa will not
          // change, and is not therefore nudged.
          bits >>= sh & (exp >> 15);
          bits &= (1ULL << 52) - 1;
        }
        else if(exp == -52) {
          // This is a special case. All bits are below the minimum
          // subnormal value. The only chance that this very tiny value
          // will become representable is when a carry happens. That is,
          // the mantissa is at least 0x8000000000000001. Being equal to
          // it will not work, as it's rounded to even.
          this->m_inxct = true;

          if(bits <= 0x8000000000000000ULL) {
            this->m_udfl = true;
            ::memcpy(&value, sd_zero + this->m_sign, sizeof(double));
            break;
          }

          // Accept the carry.
          bits = 1;
          exp = -51;
        }
        else {
          // The value is below subnormal.
          this->m_udfl = true;
          this->m_inxct = true;
          ::memcpy(&value, sd_zero + this->m_sign, sizeof(double));
          break;
        }

        // Compose the value.
        bits |= (uint64_t) this->m_sign << 63;
        bits |= (uint64_t)(uint32_t) (exp & -exp >> 15) << 52;
        ::memcpy(&value, &bits, sizeof(double));
        ROCKET_ASSERT(value != 0);
        break;
    }

    this->m_ovfl |= do_is_value_out_of_range(value, min, max);
  }

}  // namespace rocket
