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

    enum Index : Uint8
      {
        index_constant   = 0,
        index_temporary  = 1,
        index_variable   = 2,
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
    // This constructor does not accept lvalues.
    template<typename AltT, typename std::enable_if<(Variant::index_of<AltT>::value || true)>::type * = nullptr>
      Reference_root(AltT &&alt)
        : m_stor(std::forward<AltT>(alt))
        {
        }
    // This assignment operator does not accept lvalues.
    template<typename AltT, typename std::enable_if<(Variant::index_of<AltT>::value || true)>::type * = nullptr>
      Reference_root & operator=(AltT &&alt)
        {
          this->m_stor = std::forward<AltT>(alt);
          return *this;
        }
    ~Reference_root();

  public:
    Index index() const noexcept
      {
        return Index(this->m_stor.index());
      }

    const Value & dereference_readonly() const;
    Value & dereference_mutable() const;
  };

}

#endif
