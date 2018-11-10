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
        std::int64_t index;
      };
    struct S_object_key
      {
        rocket::prehashed_string key;
      };

    enum Index : std::uint8_t
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
    template<typename AltT,
      typename std::enable_if<(Variant::index_of<AltT>::value || true)>::type * = nullptr>
        Reference_modifier(AltT &&alt)
      : m_stor(std::forward<AltT>(alt))
      {
      }
    // This assignment operator does not accept lvalues.
    template<typename AltT,
      typename std::enable_if<(Variant::index_of<AltT>::value || true)>::type * = nullptr>
        Reference_modifier & operator=(AltT &&alt)
      {
        this->m_stor = std::forward<AltT>(alt);
        return *this;
      }
    ROCKET_COPYABLE_DESTRUCTOR(Reference_modifier);

  public:
    const Value * apply_const_opt(const Value &parent) const;
    Value * apply_mutable_opt(Value &parent, bool create_new) const;
    Value apply_and_erase(Value &parent) const;
  };

}

#endif
