// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_REFERENCE_DICTIONARY_
#  error Please include <asteria/llds/reference_dictionary.hpp> instead.
#endif
namespace asteria {
namespace details_reference_dictionary {

struct Bucket
  {
    Bucket* prev;
    Bucket* next;
    void* padding_1;
    void* padding_2;
    union { phsh_string key;  };
    union { Reference ref;  };

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

}  // namespace details_reference_dictionary
}  // namespace asteria
