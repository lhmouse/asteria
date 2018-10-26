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
    // This assignment operator does not accept lvalues.
    template<typename AltT, typename std::enable_if<(Variant::index_of<AltT>::value || true)>::type * = nullptr>
      Reference_modifier & operator=(AltT &&alt)
      {
        this->m_stor = std::forward<AltT>(alt);
        return *this;
      }
    ROCKET_DECLARE_COPYABLE_DESTRUCTOR(Reference_modifier);

  public:
    const Value * apply_readonly_opt(const Value &parent) const;
    // 1. If `create_new` is `true`, a new element is created if no one exists, and this function returns a pointer to
    //    it which is never null.
    // 2. If `erased_out_opt` is non-null, any existent element is removed, which is move-assigned to `*erased_out_opt`,
    //    and this function returns `erased_out_opt`.
    // 3. If no such element is found or created, this function returns a null pointer.
    Value * apply_mutable_opt(Value &parent, bool create_new, Value *erased_out_opt) const;
  };

}

#endif
