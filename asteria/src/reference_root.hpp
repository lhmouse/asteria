// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_ROOT_HPP_
#define ASTERIA_REFERENCE_ROOT_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"
#include "value.hpp"

namespace Asteria
{

class Reference_root
  {
  public:
    enum Index : std::uint8_t
      {
        type_constant         = 0,
        type_temporary_value  = 1,
        type_variable         = 2,
      };
    struct S_constant
      {
        Value src;
      };
    struct S_temporary_value
      {
        Value value;
      };
    struct S_variable
      {
        Sptr<Variable> var;
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_constant         // 0,
        , S_temporary_value  // 1,
        , S_variable         // 2,
      )>;

  private:
    Variant m_variant;

  public:
    Reference_root() noexcept
      : m_variant()  // Initialize to a constant `null`.
      {
      }
    template<typename CandidateT, typename std::enable_if<std::is_constructible<Variant, CandidateT &&>::value>::type * = nullptr>
      Reference_root(CandidateT &&cand)
        : m_variant(std::forward<CandidateT>(cand))
        {
        }
    Reference_root(const Reference_root &) noexcept;
    Reference_root & operator=(const Reference_root &) noexcept;
    Reference_root(Reference_root &&) noexcept;
    Reference_root & operator=(Reference_root &&) noexcept;
    ~Reference_root();

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
