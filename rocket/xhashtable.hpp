// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XHASHTABLE_
#define ROCKET_XHASHTABLE_

#include "fwd.hpp"
#include "assert.hpp"
namespace rocket {

template<typename bucketT>
bucketT*
get_probing_origin(bucketT* begin, bucketT* end, size_t hval) noexcept
  {
    ROCKET_ASSERT(begin < end);

    // Make a fixed-point value in the interval [0,1).
    uint64_t dist = (uint32_t) (hval * 0x9E3779B9U);

    // Multiply it by the number of buckets.
    dist *= (size_t) ((char*) end - (char*) begin);
    dist >>= 32;
    dist /= sizeof(*begin);

    // Return a pointer to the origin.
    return begin + (ptrdiff_t) dist;
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
    for(auto bkt = from;  bkt != end;  ++bkt)
      if(!*bkt || (bool) pred(*bkt))
        return bkt;

    // Phase 2: Probe from `begin` to `to`.
    for(auto bkt = begin;  bkt != to;  ++bkt)
      if(!*bkt || (bool) pred(*bkt))
        return bkt;

    // The table is full and no desired bucket has been found so far.
    return nullptr;
  }

}  // namespace rocket
#endif
