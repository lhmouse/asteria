// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "tinyfmt.hpp"
#include <cmath>

namespace rocket {
namespace {

template<typename floatT>
struct float_traits;

template<>
struct float_traits<float>
  {
    static_assert(sizeof(float) == 4);

    static constexpr int exp_bits = 8;
    static constexpr int mant_bits = 24;
    using bit_cast_type = uint32_t;
  };

template<>
struct float_traits<double>
  {
    static_assert(sizeof(double) == 8);

    static constexpr int exp_bits = 11;
    static constexpr int mant_bits = 53;
    using bit_cast_type = uint64_t;
  };

template<typename floatT>
inline void
do_split(floatT& hi, floatT& lo, floatT value) noexcept
  {
    using traits = float_traits<floatT>;
    using bit_cast_type = typename traits::bit_cast_type;
    constexpr int half_bits = ((traits::mant_bits + 1) / 2);

    bit_cast_type bits;
    ::std::memcpy(&bits, &value, sizeof(bits));
    bits >>= half_bits;
    bits <<= half_bits;
    ::std::memcpy(&hi, &bits, sizeof(bits));
    lo = value - hi;
  }

template<typename floatT>
inline floatT
do_basic_fma(floatT x, floatT y, floatT z)
  {
    // Forward NaN arguments verbatim.
    floatT temp = x * y + z;
    if(!::std::isfinite(temp))
      return temp;

    // -------------------------
    //              aaaaa bbbbb
    //              fffff ggggg
    // -------------------------
    //              bg_hi bg_lo
    //        ag_hi ag_lo
    //        fb_hi fb_lo
    //  af_hi af_lo
    // -------------------------
    floatT aa, bb, ff, gg;
    floatT bg_hi, bg_lo, ag_hi, ag_lo;
    floatT fb_hi, fb_lo, af_hi, af_lo;

    do_split(aa, bb, x);
    do_split(ff, gg, y);
    do_split(bg_hi, bg_lo, bb * gg);
    do_split(ag_hi, ag_lo, aa * gg);
    do_split(fb_hi, fb_lo, ff * bb);
    do_split(af_hi, af_lo, aa * ff);

    // Accumulate all terms from MSB to LSB.
    temp = z;
    temp += af_hi;
    temp += af_lo + fb_hi + ag_hi;
    temp += fb_lo + ag_lo + bg_hi;
    temp += bg_lo;
    return temp;
  }

}  // namespace

extern "C" float
fmaf(float x, float y, float z) noexcept
  {
    return do_basic_fma<float>(x, y, z);
  }

extern "C" double
fma(double x, double y, double z) noexcept
  {
    return do_basic_fma<double>(x, y, z);
  }

}  // namespace rocket
