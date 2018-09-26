// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_MODIFIER_HPP_
#define ASTERIA_REFERENCE_MODIFIER_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Reference_modifier
  {
  public:
    struct S_array_index
      {
        Sint64 index;
      };
    struct S_object_key
      {
        String key;
      };

    enum Index : Uint8
      {
        index_array_index  = 0,
        index_object_key   = 1,
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_array_index  // 0,
        , S_object_key   // 1,
      )>;

  private:
    Variant m_stor;

  public:
    // This constructor does not accept lvalues.
    template<typename AltT, typename std::enable_if<(Variant::index_of<AltT>::value || true)>::type * = nullptr>
      Reference_modifier(AltT &&alt)
        : m_stor(std::forward<AltT>(alt))
        {
        }
    ~Reference_modifier();

  public:
    void swap(Reference_modifier &other) noexcept
      {
        this->m_stor.swap(other.m_stor);
      }

    const Value * apply_readonly_opt(const Value &parent) const;
    Value * apply_mutable_opt(Value &parent, bool create_new, Value *erased_out_opt) const;
  };

inline void swap(Reference_modifier &lhs, Reference_modifier &rhs) noexcept
  {
    lhs.swap(rhs);
  }

}

#endif
