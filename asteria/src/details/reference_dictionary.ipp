// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_REFERENCE_DICTIONARY_HPP_
#  error Please include <asteria/llds/reference_dictionary.hpp> instead.
#endif

namespace asteria {
namespace details_reference_dictionary {

struct Bucket
  {
    Bucket* next;  // the next bucket in the [non-circular] list
    Bucket* prev;  // the previous bucket in the [circular] list
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
    // Generally, we expect teh strings to compare equal.
    if(lhs.length() != rhs.length())
      return false;

    if(ROCKET_EXPECT(lhs.data() == rhs.data()))
      return true;

    if(lhs.rdhash() != rhs.rdhash())
      return false;

    // This handwritten bytewise comparison prevents unnecessary pushs
    // and pops in the prolog and epilog of this function.
    for(size_t k = 0;  k != lhs.size(); ++k)
      if(lhs[k] != rhs[k])
        return false;

    return true;
  }

}  // namespace details_reference_dictionary
}  // namespace asteria
