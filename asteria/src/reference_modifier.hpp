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
    template<typename CandidateT, typename std::enable_if<std::is_constructible<Variant, CandidateT &&>::value>::type * = nullptr>
      Reference_modifier(CandidateT &&cand)
        : m_variant(std::forward<CandidateT>(cand))
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
    template<typename ExpectT>
      const ExpectT * opt() const noexcept
        {
          return m_variant.get<ExpectT>();
        }
    template<typename ExpectT>
      ExpectT * opt() noexcept
        {
          return m_variant.get<ExpectT>();
        }
    template<typename ExpectT>
      const ExpectT & as() const
        {
          return m_variant.as<ExpectT>();
        }
    template<typename ExpectT>
      ExpectT & as()
        {
          return m_variant.as<ExpectT>();
        }
    template<typename CandidateT>
      CandidateT & set(CandidateT &&cand)
        {
          return m_variant.set(std::forward<CandidateT>(cand));
        }
  };

}

#endif
