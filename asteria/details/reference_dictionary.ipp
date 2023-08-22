// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_REFERENCE_DICTIONARY_
#  error Please include <asteria/llds/reference_dictionary.hpp> instead.
#endif
namespace asteria {
namespace details_reference_dictionary {

struct Bucket
  {
    uint32_t flags;
    uint32_t khash;
    union { cow_string kstor[1];  };
    union { Reference vstor[1];  };

    Bucket() noexcept { }
    ~Bucket() noexcept { }
    Bucket(const Bucket&) = delete;
    Bucket& operator=(const Bucket&) = delete;

    explicit operator
    bool() const noexcept
      { return this->flags != 0;  }

    bool
    key_equals(phsh_stringR ykey) const noexcept
      {
        return ROCKET_EXPECT((this->khash == (uint32_t) ykey.rdhash())
                &&  (this->kstor[0].size() == ykey.size())
                && ((this->kstor[0].data() == ykey.data())
                    || ::rocket::xmemeq(this->kstor[0].data(), ykey.data(), ykey.size())));
      }
  };

}  // namespace details_reference_dictionary
}  // namespace asteria
