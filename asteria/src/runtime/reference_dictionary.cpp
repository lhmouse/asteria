// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_dictionary.hpp"

namespace Asteria {

Reference_dictionary::~Reference_dictionary()
  {
    const auto data = this->m_data;
    const auto nbkt = this->m_nbkt;
    for(std::size_t i = 0; i != nbkt; ++i) {
      rocket::destroy_at(data + i);
    }
    ::operator delete(data);
  }

    namespace {

    std::size_t do_get_origin(std::size_t nbkt, const rocket::prehashed_string &name) noexcept
      {
        // Conversion between an unsigned integer type and a floating point type results in performance penalty.
        // For a value known to be non-negative, an intermediate cast to some signed integer type will mitigate this.
        const auto fcast = [](std::size_t x) { return static_cast<double>(static_cast<std::ptrdiff_t>(x)); };
        const auto ucast = [](double y) { return static_cast<std::size_t>(static_cast<std::ptrdiff_t>(y)); };
        // Multiplication is faster than division.
        const auto seed = static_cast<std::uint32_t>(name.rdhash());
        const auto ratio = fcast(seed >> 1) / double(0x80000000);
        ROCKET_ASSERT((0.0 <= ratio) && (ratio < 1.0));
        const auto pos = ucast(fcast(nbkt) * ratio);
        ROCKET_ASSERT(pos < nbkt);
        return pos;
      }

    inline bool do_check_equal(const rocket::prehashed_string &lhs, const rocket::prehashed_string &rhs) noexcept
      {
        if(lhs.rdhash() != rhs.rdhash()) {
          return false;
        }
        if(lhs.size() != rhs.size()) {
          return false;
        }
        auto rp = lhs.rdstr().data();
        const auto ep = rp + lhs.rdstr().size();
        auto rq = rhs.rdstr().data();
        while(rp != ep) {
          // We only check for equality.
          // If total ordering was requested, we would have to cast both operands to `unsigned char` for comparison.
          if(static_cast<const volatile char &>(*rp) != *rq) {
            return false;
          }
          ++rp;
          ++rq;
        }
        return true;
      }

    template<typename BucketT, typename PredT>
      BucketT * do_linear_probe(BucketT *data, std::size_t nbkt, std::size_t first, std::size_t last, PredT &&pred)
      {
        // Phase one: Probe from `first` to the end of the table.
        for(std::size_t i = first; i != nbkt; ++i) {
          const auto qbkt = data + i;
          if(qbkt->refv.empty() || std::forward<PredT>(pred)(*qbkt)) {
            return qbkt;
          }
        }
        // Phase two: Probe from the beginning of the table to `last`.
        for(std::size_t i = 0; i != last; ++i) {
          const auto qbkt = data + i;
          if(qbkt->refv.empty() || std::forward<PredT>(pred)(*qbkt)) {
            return qbkt;
          }
        }
        // The table is full.
        // This is not possible as there shall always be empty slots in the table.
        ROCKET_ASSERT(false);
      }

    }

void Reference_dictionary::do_rehash(std::size_t res_arg)
  {
    if(res_arg > this->max_size()) {
      rocket::throw_length_error("Reference_dictionary::do_reserve(): A table of `%lld` references is too large and cannot be allocated.",
                                 static_cast<long long>(res_arg));
    }
    // Round up the capacity for efficiency.
    const auto nbkt = res_arg * 2 | this->m_size * 3 | 5;
    // Allocate the new table. This may throw `std::bad_alloc`.
    const auto data = static_cast<Bucket *>(::operator new(nbkt * sizeof(Bucket)));
    // Initialize the table. This will not throw exceptions.
    for(std::size_t i = 0; i != nbkt; ++i) {
      static_assert(std::is_nothrow_default_constructible<Bucket>::value, "??");
      rocket::default_construct_at(data + i);
    }
    // Rehash elements and move them into the new table. This will not throw exceptions, either.
    const auto data_old = this->m_data;
    if(ROCKET_UNEXPECT(data_old)) {
      const auto nbkt_old = this->m_nbkt;
      for(std::size_t i = 0; i != nbkt_old; ++i) {
        if(ROCKET_UNEXPECT(!data_old[i].refv.empty())) {
          // Find a bucket for it.
          const auto origin = do_get_origin(nbkt, data_old[i].name);
          const auto qbkt = do_linear_probe(data, nbkt, origin, origin, [&](const Bucket &) { return false; });
          ROCKET_ASSERT(qbkt->refv.empty());
          *qbkt = std::move(data_old[i]);
        }
        rocket::destroy_at(data_old + i);
      }
      ::operator delete(data_old);
    }
    // Set up the new table.
    this->m_data = data;
    this->m_nbkt = nbkt;
  }

std::ptrdiff_t Reference_dictionary::do_find(const rocket::prehashed_string &name) const noexcept
  {
    const auto data = this->m_data;
    if(!data) {
      return -1;
    }
    const auto nbkt = this->m_nbkt;
    // Looking for the variable using linear probing.
    const auto origin = do_get_origin(nbkt, name);
    const auto qbkt = do_linear_probe(data, nbkt, origin, origin, [&](const Bucket &cand) { return do_check_equal(cand.name, name); });
    if(qbkt->refv.empty()) {
      // Not found.
      return -1;
    }
    const auto toff = qbkt - data;
    ROCKET_ASSERT(toff >= 0);
    return toff;
  }

Reference & Reference_dictionary::do_mutate_unchecked(const rocket::prehashed_string &name)
  {
    ROCKET_ASSERT(!name.empty());
    const auto data = this->m_data;
    ROCKET_ASSERT(data);
    const auto nbkt = this->m_nbkt;
    ROCKET_ASSERT(this->m_size < nbkt / 2);
    // Find a bucket for the new element.
    const auto origin = do_get_origin(nbkt, name);
    const auto qbkt = do_linear_probe(data, nbkt, origin, origin, [&](const Bucket &cand) { return do_check_equal(cand.name, name); });
    if(!qbkt->refv.empty()) {
      // Already exists.
      return qbkt->refv.front();
    }
    qbkt->name = name;
    qbkt->refv.emplace_back();
    this->m_size += 1;
    return qbkt->refv.front();
  }

void Reference_dictionary::do_erase_unchecked(std::size_t tpos) noexcept
  {
    const auto data = this->m_data;
    ROCKET_ASSERT(data);
    const auto nbkt = this->m_nbkt;
    ROCKET_ASSERT(tpos < nbkt);
    // Nullify the bucket.
    ROCKET_ASSERT(!data[tpos].refv.empty());
    data[tpos].refv.clear();
    this->m_size -= 1;
    // Relocate elements after the erasure point.
    do_linear_probe(data, nbkt, tpos + 1, tpos,
      [&](Bucket &cand)
        {
          // Remove the element from the old bucket.
          auto old = std::move(cand);
          cand.refv.clear();
          // Find a new bucket for it.
          const auto origin = do_get_origin(nbkt, old.name);
          const auto qbkt = do_linear_probe(data, nbkt, origin, origin, [&](const Bucket &) { return false; });
          ROCKET_ASSERT(qbkt->refv.empty());
          // Insert it into the new bucket.
          *qbkt = std::move(old);
          return false;
        }
      );
  }

}
