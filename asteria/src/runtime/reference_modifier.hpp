// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_MODIFIER_HPP_
#define ASTERIA_RUNTIME_REFERENCE_MODIFIER_HPP_

#include "../fwd.hpp"

namespace asteria {

class Reference_modifier
  {
  public:
    struct S_array_index
      {
        int64_t index;
      };

    struct S_object_key
      {
        phsh_string key;
      };

    struct S_array_head
      {
      };

    struct S_array_tail
      {
      };

    enum Index : uint8_t
      {
        index_array_index  = 0,
        index_object_key   = 1,
        index_array_head   = 2,
        index_array_tail   = 3,
      };

    using Storage = variant<
      ROCKET_CDR(
      , S_array_index  // 0,
      , S_object_key   // 1,
      , S_array_head   // 2,
      , S_array_tail   // 3,
      )>;

    static_assert(::std::is_nothrow_copy_assignable<Storage>::value);

  private:
    Storage m_stor;

  public:
    ASTERIA_VARIANT_CONSTRUCTOR(Reference_modifier, Storage, XModT, xmod)
      : m_stor(::std::forward<XModT>(xmod))
      { }

    ASTERIA_VARIANT_ASSIGNMENT(Reference_modifier, Storage, XModT, xmod)
      {
        this->m_stor = ::std::forward<XModT>(xmod);
        return *this;
      }

  public:
    Index
    index()
    const noexcept
      { return static_cast<Index>(this->m_stor.index());  }

    Reference_modifier&
    swap(Reference_modifier& other)
    noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    const Value*
    apply_const_opt(const Value& parent)
    const;

    Value*
    apply_mutable_opt(Value& parent)
    const;

    Value&
    apply_and_create(Value& parent)
    const;

    Value
    apply_and_erase(Value& parent)
    const;
  };

inline
void
swap(Reference_modifier& lhs, Reference_modifier& rhs)
noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
