// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XHASHTABLE_
#define ROCKET_XHASHTABLE_

#include "fwd.hpp"
#include "xassert.hpp"
namespace rocket {

constexpr
size_t
probe_origin(size_t size, size_t hval)
  noexcept
  {
    // Make a fixed-point value in the interval [0,1), and then multiply
    // `size` by it to get an index in the middle.
    return (size_t) (hval * 0x9E3779B9U % 0x100000000ULL * size / 0x100000000ULL);
  }

template<typename bucketT, typename predictorT>
constexpr
bucketT*
linear_probe(bucketT* data, size_t to, size_t from, size_t size, predictorT&& pred)
  {
    // Probe from `from` to `size`, then from `0` to `to`. Probing shall
    // stop if an empty bucket or a match is found.
    for(size_t k = from;  k != size;  ++k)
      if(!(bool) data[k] || (bool) pred(data[k]))
        return data + k;

    for(size_t k = 0;  k != to;  ++k)
      if(!(bool) data[k] || (bool) pred(data[k]))
        return data + k;

    return nullptr;
  }

}  // namespace rocket
#endif
