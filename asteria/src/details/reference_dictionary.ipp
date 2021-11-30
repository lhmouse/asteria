// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_REFERENCE_DICTIONARY_HPP_
#  error Please include <asteria/llds/reference_dictionary.hpp> instead.
#endif

namespace asteria {
namespace details_reference_dictionary {

struct Bucket
  {
    Bucket* next;  // the next bucket in the [non-circular] list;
                   // used for iteration
    Bucket* prev;  // the previous bucket in the [circular] list;
                   // used to mark whether this bucket is empty or not

    union { phsh_string kstor[1];  };  // initialized iff `prev` is non-null
    union { Reference vstor[1];  };  // initialized iff `prev` is non-null

    Bucket() noexcept { }
    ~Bucket() noexcept { }

   void
   debug_clear() noexcept
     {
       ::std::memset(static_cast<void*>(this), 0xD3, sizeof(*this));
       this->prev = nullptr;
     }

    explicit operator
    bool() const noexcept
      { return this->prev != nullptr;  }
  };

inline bool
do_compare_eq(const phsh_string& lhs, const phsh_string& rhs) noexcept
  {
    // Generally, we expect the strings to compare equal.
    size_t n = lhs.size();
    if(n != rhs.size())
      return false;

    const char* lp = lhs.data();
    const char* rp = rhs.data();
    if(lp == rp)
      return true;

    if(lhs.rdhash() != rhs.rdhash())
      return false;

    // Perform word-wise comparison.
    constexpr size_t M = sizeof(uintptr_t);
    while(n >= M) {
      uintptr_t words[2];
      ::std::memcpy(words + 0, lp, M);
      ::std::memcpy(words + 1, rp, M);
      if(words[0] != words[1])
        return false;

      lp += M;
      rp += M;
      n -= M;
    }

    // Perform character-wise comparison.
    while(n > 0) {
      if(static_cast<const volatile char&>(*lp) != *rp)
        return false;

      lp += 1;
      rp += 1;
      n -= 1;
    }

    return true;
  }

}  // namespace details_reference_dictionary
}  // namespace asteria
