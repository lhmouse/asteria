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
        std::int64_t index;
      };
    struct S_object_key
      {
        PreHashed_String key;
      };

    enum Index : std::uint8_t
      {
        index_array_index  = 0,
        index_object_key   = 1,
      };
    using Xvariant = Variant<
      ROCKET_CDR(
        , S_array_index  // 0,
        , S_object_key   // 1,
      )>;
    static_assert(std::is_nothrow_copy_assignable<Xvariant>::value, "???");

  private:
    Xvariant m_stor;

  public:
    // This constructor does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Xvariant::index_of<AltT>::value)> Reference_Modifier(AltT&& altr) noexcept
      : m_stor(rocket::forward<AltT>(altr))
      {
      }
    // This assignment operator does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Xvariant::index_of<AltT>::value)> Reference_Modifier& operator=(AltT&& altr) noexcept
      {
        this->m_stor = rocket::forward<AltT>(altr);
        return *this;
      }

  public:
    void swap(Reference_Modifier& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
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
