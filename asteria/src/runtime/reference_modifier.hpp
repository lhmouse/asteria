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
    template<typename XmodT, ASTERIA_SFINAE_CONSTRUCT(Xvariant, XmodT&&)> Reference_Modifier(XmodT&& xmod) noexcept
      : m_stor(rocket::forward<XmodT>(xmod))
      {
      }
    template<typename XmodT, ASTERIA_SFINAE_ASSIGN(Xvariant, XmodT&&)> Reference_Modifier& operator=(XmodT&& xmod) noexcept
      {
        this->m_stor = rocket::forward<XmodT>(xmod);
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
    return lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
