// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_DESIGNATOR_HPP_
#define ASTERIA_REFERENCE_DESIGNATOR_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria
{

class Reference_designator
  {
  public:
    enum Type : std::uint8_t
      {
        type_array_member   = 0,
        type_object_member  = 1,
      };
    struct S_array_member
      {
        Signed_integer index;
      };
    struct S_object_member
      {
        String name;
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_array_member   // 0,
        , S_object_member  // 1,
      )>;

  private:
    Variant m_variant;

  public:
    template<typename CandidateT, typename std::enable_if<std::is_constructible<Variant, CandidateT &&>::value>::type * = nullptr>
      Reference_designator(CandidateT &&cand)
        : m_variant(std::forward<CandidateT>(cand))
        {
        }
    Reference_designator(Reference_designator &&) noexcept;
    Reference_designator & operator=(Reference_designator &&) noexcept;
    ~Reference_designator();

  public:
    Type which() const noexcept
      {
        return static_cast<Type>(m_variant.index());
      }
    template<typename ExpectT>
      const ExpectT * get_opt() const noexcept
        {
          return m_variant.get<ExpectT>();
        }
    template<typename ExpectT>
      ExpectT * get_opt() noexcept
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
      void set(CandidateT &&cand)
        {
          m_variant.set(std::forward<CandidateT>(cand));
        }
  };

}

#endif
