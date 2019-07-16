// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_ROOT_HPP_
#define ASTERIA_RUNTIME_REFERENCE_ROOT_HPP_

#include "../fwd.hpp"
#include "value.hpp"
#include "variable.hpp"

namespace Asteria {

class Reference_Root
  {
  public:
    struct S_null
      {
      };
    struct S_constant
      {
        Value source;
      };
    struct S_temporary
      {
        Value value;
      };
    struct S_variable
      {
        Rcptr<Variable> var_opt;
      };

    enum Index : std::uint8_t
      {
        index_null       = 0,
        index_constant   = 1,
        index_temporary  = 2,
        index_variable   = 3,
      };
    using Xvariant = Variant<
      ROCKET_CDR(
        , S_null       // 0,
        , S_constant   // 1,
        , S_temporary  // 2,
        , S_variable   // 3,
      )>;
    static_assert(std::is_nothrow_copy_assignable<Xvariant>::value, "???");

  private:
    Xvariant m_stor;

  public:
    Reference_Root() noexcept
      : m_stor()  // Initialize to a null reference.
      {
      }
    template<typename XrootT, ASTERIA_SFINAE_CONSTRUCT(Xvariant, XrootT&&)> Reference_Root(XrootT&& root) noexcept
      : m_stor(rocket::forward<XrootT>(root))
      {
      }
    template<typename XrootT, ASTERIA_SFINAE_ASSIGN(Xvariant, XrootT&&)> Reference_Root& operator=(XrootT&& root) noexcept
      {
        this->m_stor = rocket::forward<XrootT>(root);
        return *this;
      }

  public:
    bool is_constant() const noexcept
      {
        return (this->m_stor.index() == index_null) || (this->m_stor.index() == index_constant);
      }
    bool is_temporary() const noexcept
      {
        return (this->m_stor.index() == index_null) || (this->m_stor.index() == index_temporary);
      }
    bool is_variable() const noexcept
      {
        return (this->m_stor.index() == index_variable);
      }

    void swap(Reference_Root& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
      }

    const Value& dereference_const() const;
    Value& dereference_mutable() const;

    void enumerate_variables(const Abstract_Variable_Callback& callback) const;
  };

inline void swap(Reference_Root& lhs, Reference_Root& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
