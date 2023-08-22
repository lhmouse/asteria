// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_VARIABLE_HASHMAP_
#  error Please include <asteria/llds/variable_hashmap.hpp> instead.
#endif
namespace asteria {
namespace details_variable_hashmap {

struct Bucket
  {
    const void* key_opt;
    union { refcnt_ptr<Variable> vstor[1];  };

    Bucket() noexcept { }
    ~Bucket() noexcept { }
    Bucket(const Bucket&) = delete;
    Bucket& operator=(const Bucket&) = delete;

    explicit operator
    bool() const noexcept
      { return this->key_opt != nullptr;  }
  };

}  // namespace details_variable_hashmap
}  // namespace asteria
