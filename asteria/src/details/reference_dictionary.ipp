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

    Bucket()
      { }

    ~Bucket()
      { }

    explicit operator
    bool() const noexcept
      { return this->prev != nullptr;  }
  };

}  // namespace details_reference_dictionary
}  // namespace asteria
