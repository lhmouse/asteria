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
    enum Index : Uint8
      {
        index_array_index  = 0,
        index_object_key   = 1,
      };
    struct S_array_index
      {
        Signed index;
      };
    struct S_object_key
      {
        String key;
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_array_index  // 0,
        , S_object_key   // 1,
      )>;

  private:
    Variant m_stor;

  public:
    template<typename AltT, typename std::enable_if<std::is_constructible<Variant, AltT &&>::value>::type * = nullptr>
      Reference_modifier(AltT &&alt)
        : m_stor(std::forward<AltT>(alt))
        {
        }
    ~Reference_modifier();

    Reference_modifier(const Reference_modifier &) noexcept;
    Reference_modifier & operator=(const Reference_modifier &) noexcept;
    Reference_modifier(Reference_modifier &&) noexcept;
    Reference_modifier & operator=(Reference_modifier &&) noexcept;

  public:
    const Value * apply_readonly_opt(const Value &parent) const;
    Value * apply_mutable_opt(Value &parent, bool creates, Value *erased_out_opt) const;
  };

}

#endif
