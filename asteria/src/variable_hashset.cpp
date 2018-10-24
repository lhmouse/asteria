// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable_hashset.hpp"

namespace Asteria {

Variable_hashset::~Variable_hashset()
  {
    const auto data = this->m_data;
    const auto nbkt = this->m_nbkt;
    for(Size i = 0; i != nbkt; ++i) {
      rocket::destroy_at(data + i);
    }
    ::operator delete(data);
  }

    namespace {

    Size do_get_origin(Size nbkt, const rocket::refcounted_ptr<Variable> &var) noexcept
      {
        // Conversion between an unsigned integer type and a floating point type results in performance penalty.
        // For a value known to be non-negative, an intermediate cast to some signed integer type will mitigate this.
        const auto fcast = [](Size x) { return static_cast<double>(static_cast<Diff>(x)); };
        const auto ucast = [](double y) { return static_cast<Size>(static_cast<Diff>(y)); };
        // Multiplication is faster than division.
        const auto seed = static_cast<Uint32>(reinterpret_cast<Uintptr>(var.get()) * 0x82F63B78);
        const auto ratio = fcast(seed >> 1) / double(0x80000000);
        ROCKET_ASSERT((0.0 <= ratio) && (ratio < 1.0));
        const auto pos = ucast(fcast(nbkt) * ratio);
        ROCKET_ASSERT(pos < nbkt);
        return pos;
      }

    template<typename PredT>
      rocket::refcounted_ptr<Variable> * do_linear_probe(rocket::refcounted_ptr<Variable> * data, Size nbkt, Size first, Size last, PredT &&pred)
      {
        // Phase one: Probe from `first` to the end of the table.
        for(Size i = first; i != nbkt; ++i) {
          const auto bkt = data + i;
          if(!*bkt || std::forward<PredT>(pred)(*bkt)) {
            return bkt;
          }
        }
        // Phase two: Probe from the beginning of the table to `last`.
        for(Size i = 0; i != last; ++i) {
          const auto bkt = data + i;
          if(!*bkt || std::forward<PredT>(pred)(*bkt)) {
            return bkt;
          }
        }
        // The table is full.
        // This is not possible as there shall always be empty slots in the table.
        ROCKET_ASSERT(false);
      }

    }

void Variable_hashset::do_rehash(Size res_arg)
  {
    if(res_arg > this->max_size()) {
      rocket::throw_length_error("Variable_hashset::do_reserve(): A table of `%lld` variables is too large and cannot be allocated.",
                                 static_cast<long long>(res_arg));
    }
    // Round up the capacity for efficiency.
    const auto nbkt = res_arg * 2 | this->m_size * 3 | 32;
    // Allocate the new table. This may throw `std::bad_alloc`.
    const auto data = static_cast<rocket::refcounted_ptr<Variable> *>(::operator new(nbkt * sizeof(rocket::refcounted_ptr<Variable>)));
    // Initialize the table. This will not throw exceptions.
    for(Size i = 0; i != nbkt; ++i) {
      rocket::construct_at(data + i);
    }
    // Rehash elements and move them into the new table. This will not throw exceptions, either.
    const auto data_old = this->m_data;
    const auto nbkt_old = this->m_nbkt;
    for(Size i = 0; i != nbkt_old; ++i) {
      if(data_old[i]) {
        // Find a bucket for it.
        const auto origin = do_get_origin(nbkt, data_old[i]);
        const auto qbkt = do_linear_probe(data, nbkt, origin, origin, [&](const rocket::refcounted_ptr<Variable> &) { return false; });
        ROCKET_ASSERT(!*qbkt);
        *qbkt = std::move(data_old[i]);
      }
      rocket::destroy_at(data_old + i);
    }
    ::operator delete(data_old);
    // Set up the new table.
    this->m_data = data;
    this->m_nbkt = nbkt;
  }

Diff Variable_hashset::do_find(const rocket::refcounted_ptr<Variable> &var) const noexcept
  {
    const auto data = this->m_data;
    if(!data) {
      return -1;
    }
    const auto nbkt = this->m_nbkt;
    // Looking for the variable using linear probing.
    const auto origin = do_get_origin(nbkt, var);
    const auto qbkt = do_linear_probe(data, nbkt, origin, origin, [&](const rocket::refcounted_ptr<Variable> &cand) { return cand == var; }
      );
    if(!*qbkt) {
      // Not found.
      return -1;
    }
    const auto toff = qbkt - data;
    ROCKET_ASSERT(toff >= 0);
    return toff;
  }

bool Variable_hashset::do_insert_unchecked(const rocket::refcounted_ptr<Variable> &var) noexcept
  {
    const auto data = this->m_data;
    ROCKET_ASSERT(data);
    const auto nbkt = this->m_nbkt;
    ROCKET_ASSERT(this->m_size < nbkt / 2);
    // Find a bucket for the new element.
    const auto origin = do_get_origin(nbkt, var);
    const auto qbkt = do_linear_probe(data, nbkt, origin, origin, [&](const rocket::refcounted_ptr<Variable> &cand) { return cand == var; });
    if(*qbkt) {
      // Already exists.
      return false;
    }
    *qbkt = var;
    this->m_size += 1;
    return true;
  }

void Variable_hashset::do_erase_unchecked(Size tpos) noexcept
  {
    const auto data = this->m_data;
    ROCKET_ASSERT(data);
    const auto nbkt = this->m_nbkt;
    ROCKET_ASSERT(tpos < nbkt);
    // Nullify the bucket.
    ROCKET_ASSERT(data[tpos]);
    data[tpos] = nullptr;
    this->m_size -= 1;
    // Relocate elements after the erasure point.
    do_linear_probe(data, nbkt, tpos + 1, tpos,
      [&](rocket::refcounted_ptr<Variable> &cand)
        {
          // Remove the element from the old bucket.
          auto var = std::move(cand);
          ROCKET_ASSERT(!cand);
          // Find a new bucket for it.
          const auto origin = do_get_origin(nbkt, var);
          const auto qbkt = do_linear_probe(data, nbkt, origin, origin, [&](const rocket::refcounted_ptr<Variable> &) { return false; });
          ROCKET_ASSERT(!*qbkt);
          // Insert it into the new bucket.
          *qbkt = std::move(var);
          return false;
        }
      );
  }

}
