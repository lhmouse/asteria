// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_ROOT_HPP_
#define ASTERIA_RUNTIME_REFERENCE_ROOT_HPP_

#include "../fwd.hpp"
#include "value.hpp"
#include "variable.hpp"
#include "../rocket/variant.hpp"

namespace Asteria {

class Reference_root
  {
  public:
    struct S_null
      {
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
        rocket::refcounted_ptr<Variable> var;
      };

    enum Index : std::uint8_t
      {
        index_null       = 0,
        index_constant   = 1,
        index_temporary  = 2,
        index_variable   = 3,
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_null       // 0,
        , S_constant   // 1,
        , S_temporary  // 2,
        , S_variable   // 3,
      )>;

  private:
    Variant m_stor;

  public:
    Reference_root() noexcept
      : m_stor()  // Initialize to a constant `null`.
      {
      }
    // This constructor does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Variant::index_of<AltT>::value)>
      Reference_root(AltT &&alt)
      : m_stor(std::forward<AltT>(alt))
      {
      }
    // This assignment operator does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Variant::index_of<AltT>::value)>
      Reference_root & operator=(AltT &&alt)
      {
        this->m_stor = std::forward<AltT>(alt);
        return *this;
      }
    ROCKET_COPYABLE_DESTRUCTOR(Reference_root);

  public:
    Index index() const noexcept
      {
        return Index(this->m_stor.index());
      }
    template<typename AltT>
      const AltT * opt() const noexcept
      {
        return this->m_stor.get<AltT>();
      }
    template<typename AltT>
      const AltT & check() const
      {
        return this->m_stor.as<AltT>();
      }

    const Value & dereference_const() const;
    Value & dereference_mutable() const;

    void enumerate_variables(const Abstract_variable_callback &callback) const;
    void dispose_variable(Global_context &global) const noexcept;
  };

}

#endif
