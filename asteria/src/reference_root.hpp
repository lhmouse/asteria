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
    enum Index : Uint8
      {
        index_constant   = 0,
        index_temporary  = 1,
        index_variable   = 2,
      };
    struct S_constant
      {
        Value src;
      };
    struct S_temporary
      {
        Value value;
      };
    struct S_variable
      {
        Sptr<Variable> var;
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_constant   // 0,
        , S_temporary  // 1,
        , S_variable   // 2,
      )>;

  private:
    Variant m_stor;

  public:
    Reference_root() noexcept
      : m_stor()  // Initialize to a constant `null`.
      {
      }
    template<typename AltT, typename std::enable_if<std::is_constructible<Variant, AltT &&>::value>::type * = nullptr>
      Reference_root(AltT &&alt)
        : m_stor(std::forward<AltT>(alt))
        {
        }
    ~Reference_root();

    Reference_root(const Reference_root &) noexcept;
    Reference_root & operator=(const Reference_root &) noexcept;
    Reference_root(Reference_root &&) noexcept;
    Reference_root & operator=(Reference_root &&) noexcept;

  public:
    bool is_constant() const noexcept;
    bool is_lvalue() const noexcept;
    bool is_unique() const noexcept;

    const Value & dereference_readonly() const;
    Value & dereference_mutable() const;
  };

}

#endif
