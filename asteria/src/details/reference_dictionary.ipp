// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

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

    uintptr_t padding[2];

    union { phsh_string kstor[1];  };  // initialized iff `prev` is non-null
    union { Reference vstor[1];  };  // initialized iff `prev` is non-null

    Bucket() noexcept { }
    ~Bucket() noexcept { }
    Bucket(const Bucket&) = delete;
    Bucket& operator=(const Bucket&) = delete;

    explicit operator
    bool() const noexcept
      { return this->prev != nullptr;  }
  };

inline bool
do_compare_eq(const phsh_string& lhs, const phsh_string& rhs) noexcept
  {
    // Generally, we expect the strings to compare equal.
    return ROCKET_EXPECT((((uintptr_t) lhs.c_str() ^ (uintptr_t) rhs.c_str())
                          | (lhs.size() ^ rhs.size())) == 0)
           || ((lhs.size() == rhs.size())
               && (::std::memcmp(lhs.c_str(), rhs.c_str(), lhs.size()) == 0));
  }

}  // namespace details_reference_dictionary
}  // namespace asteria
