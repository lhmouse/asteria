// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XHASHTABLE_HPP_
#define ROCKET_XHASHTABLE_HPP_

#include "fwd.hpp"
#include "assert.hpp"

namespace rocket {

template<typename bucketT>
bucketT*
get_probing_origin(bucketT* begin, bucketT* end, size_t hval)
  noexcept
  {
    ROCKET_ASSERT(begin < end);

    // This makes a floating-point value in the interval [1,2).
    uint64_t word = static_cast<uint32_t>(hval * 0x9E3779B9);
    word = 0x3FF00000'00000000 | word << 20;

    // We assume floating-point numbers have the same endianness as integers.
    double ratio;
    ::std::memcpy(&ratio, &word, sizeof(double));
    ROCKET_ASSERT((1 <= ratio) && (ratio < 2));

    // The compiler is free to transform this expression into a fused multiply-add.
    double dist = static_cast<double>(end - begin);
    dist = ratio * dist - dist;

    // Truncate the distance towards zero.
    auto bkt = begin + static_cast<ptrdiff_t>(dist);
    ROCKET_ASSERT(bkt < end);
    return bkt;
  }

template<typename bucketT, typename predT>
bucketT*
linear_probe(bucketT* begin, bucketT* to, bucketT* from, bucketT* end, predT&& pred)
  {
    ROCKET_ASSERT(begin <= to);
    ROCKET_ASSERT(to <= from);
    ROCKET_ASSERT(from <= end);
    ROCKET_ASSERT(begin < end);

    // Phase 1: Probe from `from` to `end`.
    for(auto bkt = from; bkt != end; ++bkt)
      if(!*bkt || pred(*bkt))
        return bkt;

    // Phase 2: Probe from `begin` to `to`.
    for(auto bkt = begin; bkt != to; ++bkt)
      if(!*bkt || pred(*bkt))
        return bkt;

    // The table is full and no desired bucket has been found so far.
    return nullptr;
  }

}  // namespace rocket

#endif
