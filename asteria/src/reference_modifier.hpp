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
    enum Index : std::uint8_t
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
    Variant m_variant;

  public:
    template<typename AltT, typename std::enable_if<std::is_constructible<Variant, AltT &&>::value>::type * = nullptr>
      Reference_modifier(AltT &&alt)
        : m_variant(std::forward<AltT>(alt))
        {
        }
    Reference_modifier(const Reference_modifier &) noexcept;
    Reference_modifier & operator=(const Reference_modifier &) noexcept;
    Reference_modifier(Reference_modifier &&) noexcept;
    Reference_modifier & operator=(Reference_modifier &&) noexcept;
    ~Reference_modifier();

  public:
    Index index() const noexcept
      {
        return static_cast<Index>(m_variant.index());
      }
    template<typename AltT>
      const AltT * opt() const noexcept
        {
          return m_variant.get<AltT>();
        }
    template<typename AltT>
      const AltT & as() const
        {
          return m_variant.as<AltT>();
        }
  };

extern const Value * apply_reference_modifier_readonly_partial_opt(const Reference_modifier &modifier, const Value &parent);
extern Value * apply_reference_modifier_mutable_partial_opt(const Reference_modifier &modifier, Value &parent, bool creates, Value *erased_out_opt);

}

#endif
