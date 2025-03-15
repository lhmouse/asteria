// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_VARIABLE_HASHMAP_
#  error Please include <asteria/llds/variable_hashmap.hpp> instead.
#endif
namespace asteria {
namespace details_variable_hashmap {

struct Bucket
  {
    Bucket* prev;
    Bucket* next;
    const void* key;
    union { refcnt_ptr<Variable> var_opt;  };

    Bucket() noexcept { }
    ~Bucket() noexcept { }
    Bucket(const Bucket&) = delete;
    Bucket& operator=(const Bucket&) = delete;

    void
    attach(Bucket& after) noexcept
      {
        this->prev = &after;
        this->next = after.next;
        this->prev->next = this;
        this->next->prev = this;
      }

    void
    detach() noexcept
      {
        this->prev->next = this->next;
        this->next->prev = this->prev;
        this->prev = nullptr;
        this->next = nullptr;
      }

    explicit operator bool() const noexcept
      { return this->next != nullptr;  }
  };

}  // namespace details_variable_hashmap
}  // namespace asteria
