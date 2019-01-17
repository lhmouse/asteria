// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_MODIFIER_HPP_
#define ASTERIA_RUNTIME_REFERENCE_MODIFIER_HPP_

#include "../fwd.hpp"
#include "../rocket/preprocessor_utilities.h"
#include "../rocket/variant.hpp"

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
             ROCKET_ENABLE_IF_HAS_VALUE(Variant::index_of<AltT>::value)>
      Reference_Modifier(AltT &&alt)
      : m_stor(std::forward<AltT>(alt))
      {
      }
    // This assignment operator does not accept lvalues.
    template<typename AltT,
             ROCKET_ENABLE_IF_HAS_VALUE(Variant::index_of<AltT>::value)>
      Reference_Modifier & operator=(AltT &&alt)
      {
        this->m_stor = std::forward<AltT>(alt);
        return *this;
      }
    ROCKET_COPYABLE_DESTRUCTOR(Reference_Modifier);

  public:
    const Value * apply_const_opt(const Value &parent) const;
    Value * apply_mutable_opt(Value &parent, bool create_new) const;
    Value apply_and_erase(Value &parent) const;
  };

}

#endif
