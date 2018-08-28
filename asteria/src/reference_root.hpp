// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_ROOT_HPP_
#define ASTERIA_REFERENCE_ROOT_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"
#include "value.hpp"
#include "variable.hpp"

namespace Asteria {

class Reference_root
  {
  public:
    enum Index : std::uint8_t
      {
        index_constant    = 0,
        index_temp_value  = 1,
        index_variable    = 2,
      };
    struct S_constant
      {
        Value src;
      };
    struct S_temp_value
      {
        Value value;
      };
    struct S_variable
      {
        Sptr<Variable> var;
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_constant    // 0,
        , S_temp_value  // 1,
        , S_variable    // 2,
      )>;

  private:
    Variant m_variant;

  public:
    Reference_root() noexcept
      : m_variant()  // Initialize to a constant `null`.
      {
      }
    template<typename AltT, typename std::enable_if<std::is_constructible<Variant, AltT &&>::value>::type * = nullptr>
      Reference_root(AltT &&alt)
        : m_variant(std::forward<AltT>(alt))
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

extern const Value & dereference_root_readonly_partial(const Reference_root &root) noexcept;
extern Value & dereference_root_mutable_partial(const Reference_root &root);

}

#endif
