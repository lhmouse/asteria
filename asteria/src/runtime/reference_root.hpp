// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_ROOT_HPP_
#define ASTERIA_RUNTIME_REFERENCE_ROOT_HPP_

#include "../fwd.hpp"
#include "../value.hpp"
#include "variable.hpp"
#include "tail_call_arguments.hpp"
#include "../source_location.hpp"

namespace Asteria {

class Reference_Root
  {
  public:
    struct S_void
      {
      };
    struct S_constant
      {
        Value val;
      };
    struct S_temporary
      {
        Value val;
      };
    struct S_variable
      {
        ckptr<Variable> var;
      };
    struct S_tail_call
      {
        ckptr<Tail_Call_Arguments> tca;
      };

    enum Index : uint8_t
      {
        index_void       = 0,
        index_constant   = 1,
        index_temporary  = 2,
        index_variable   = 3,
        index_tail_call  = 4,
      };
    using Xvariant = variant<
      ROCKET_CDR(
      , S_void       // 0,
      , S_constant   // 1,
      , S_temporary  // 2,
      , S_variable   // 3,
      , S_tail_call  // 4,
      )>;
    static_assert(::std::is_nothrow_copy_assignable<Xvariant>::value, "");

  private:
    Xvariant m_stor;

  public:
    ASTERIA_VARIANT_CONSTRUCTOR(Reference_Root, Xvariant, XrootT, xroot)
      :
        m_stor(::rocket::forward<XrootT>(xroot))
      {
      }
    ASTERIA_VARIANT_ASSIGNMENT(Reference_Root, Xvariant, XrootT, xroot)
      {
        this->m_stor = ::rocket::forward<XrootT>(xroot);
        return *this;
      }

  public:
    Index index() const noexcept
      {
        return static_cast<Index>(this->m_stor.index());
      }

    bool is_void() const noexcept
      {
        return this->index() == index_void;
      }

    bool is_constant() const noexcept
      {
        return this->index() == index_constant;
      }
    const Value& as_constant() const
      {
        return this->m_stor.as<index_constant>().val;
      }

    bool is_temporary() const noexcept
      {
        return this->index() == index_temporary;
      }
    const Value& as_temporary() const
      {
        return this->m_stor.as<index_temporary>().val;
      }

    bool is_variable() const noexcept
      {
        return this->index() == index_variable;
      }
    const ckptr<Variable>& as_variable() const
      {
        return this->m_stor.as<index_variable>().var;
      }

    bool is_tail_call() const noexcept
      {
        return this->index() == index_tail_call;
      }
    const ckptr<Tail_Call_Arguments>& as_tail_call() const
      {
        return this->m_stor.as<index_tail_call>().tca;
      }

    Reference_Root& swap(Reference_Root& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    const Value& dereference_const() const;
    Value& dereference_mutable() const;
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const;
  };

inline void swap(Reference_Root& lhs, Reference_Root& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
