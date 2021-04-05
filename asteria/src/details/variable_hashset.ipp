// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_VARIABLE_HASHSET_HPP_
#  error Please include <asteria/llds/variable_hashset.hpp> instead.
#endif

namespace asteria {
namespace details_variable_hashset {

struct Bucket
  {
    Bucket* next;  // the next bucket in the [non-circular] list
    Bucket* prev;  // the previous bucket in the [circular] list
    union { rcptr<Variable> kstor[1];  };  // initialized iff `prev` is non-null

    Bucket()
      { }

    ~Bucket()
      { }

    explicit operator
    bool() const noexcept
      { return this->prev != nullptr;  }
  };

}  // namespace details_variable_hashset
}  // namespace asteria
