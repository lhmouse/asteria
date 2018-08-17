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
    enum Type : std::uint8_t
      {
        type_constant   = 0,
        type_rvalue     = 1,
        type_lvalue     = 2,
      };
    struct S_constant
      {
        Value source;
      };
    struct S_rvalue
      {
        Value value;
      };
    struct S_lvalue
      {
        Sptr<Variable> var;
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_constant   // 0,
        , S_rvalue     // 1,
        , S_lvalue     // 2,
      )>;

  private:
    Variant m_variant;

  public:
    template<typename CandidateT, typename std::enable_if<std::is_constructible<Variant, CandidateT &&>::value>::type * = nullptr>
      Reference_root(CandidateT &&cand)
        : m_variant(std::forward<CandidateT>(cand))
        {
        }
    Reference_root(Reference_root &&) noexcept;
    Reference_root & operator=(Reference_root &&) noexcept;
    ~Reference_root();

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

extern const char * get_reference_category_name(Reference_root::Type type) noexcept;
extern const char * get_reference_category_name_of(const Reference_root &root) noexcept;

}

#endif
