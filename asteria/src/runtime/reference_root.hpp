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
    struct S_null
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
        index_null       = 0,
        index_constant   = 1,
        index_temporary  = 2,
        index_variable   = 3,
        index_tail_call  = 4,
      };
    using Xvariant = variant<
      ROCKET_CDR(
        , S_null       // 0,
        , S_constant   // 1,
        , S_temporary  // 2,
        , S_variable   // 3,
        , S_tail_call  // 4,
      )>;
    static_assert(std::is_nothrow_copy_assignable<Xvariant>::value, "");

  private:
    Xvariant m_stor;

  public:
    Reference_Root() noexcept
      :
        m_stor()  // Initialize to a null reference.
      {
      }
    template<typename XrootT, ASTERIA_SFINAE_CONSTRUCT(Xvariant, XrootT&&)> Reference_Root(XrootT&& xroot)
      :
        m_stor(rocket::forward<XrootT>(xroot))
      {
      }
    template<typename XrootT, ASTERIA_SFINAE_ASSIGN(Xvariant, XrootT&&)> Reference_Root& operator=(XrootT&& xroot)
      {
        this->m_stor = rocket::forward<XrootT>(xroot);
        return *this;
      }

  public:
    Index index() const noexcept
      {
        return static_cast<Index>(this->m_stor.index());
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
