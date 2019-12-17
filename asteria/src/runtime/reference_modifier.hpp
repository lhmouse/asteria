// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_MODIFIER_HPP_
#define ASTERIA_RUNTIME_REFERENCE_MODIFIER_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Reference_Modifier
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
    struct S_array_tail
      {
      };

    enum Index : uint8_t
      {
        index_array_index  = 0,
        index_object_key   = 1,
        index_array_tail   = 2,
      };
    using Xvariant = variant<
      ROCKET_CDR(
      , S_array_index  // 0,
      , S_object_key   // 1,
      , S_array_tail   // 2,
      )>;
    static_assert(::std::is_nothrow_copy_assignable<Xvariant>::value, "");

  private:
    Xvariant m_stor;

  public:
    ASTERIA_VARIANT_CONSTRUCTOR(Reference_Modifier, Xvariant, XmodT, xmod)
      :
        m_stor(::rocket::forward<XmodT>(xmod))
      {
      }
    ASTERIA_VARIANT_ASSIGNMENT(Reference_Modifier, Xvariant, XmodT, xmod)
      {
        this->m_stor = ::rocket::forward<XmodT>(xmod);
        return *this;
      }

  public:
    Index index() const noexcept
      {
        return static_cast<Index>(this->m_stor.index());
      }

    Reference_Modifier& swap(Reference_Modifier& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    const Value* apply_const_opt(const Value& parent) const;
    Value* apply_mutable_opt(Value& parent, bool create_new) const;
    Value apply_and_erase(Value& parent) const;
  };

inline void swap(Reference_Modifier& lhs, Reference_Modifier& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
