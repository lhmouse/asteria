// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_ROOT_HPP_
#define ASTERIA_RUNTIME_REFERENCE_ROOT_HPP_

#include "../fwd.hpp"
#include "../value.hpp"
#include "../source_location.hpp"

namespace asteria {

class Reference_root
  {
  public:
    struct S_uninit
      {
      };

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
        rcfwdp<Variable> var;
      };

    struct S_tail_call
      {
        rcfwdp<PTC_Arguments> tca;
      };

    struct S_jump_src
      {
        Source_Location sloc;
      };

    enum Index : uint8_t
      {
        index_uninit      = 0,
        index_void        = 1,
        index_constant    = 2,
        index_temporary   = 3,
        index_variable    = 4,
        index_tail_call   = 5,
        index_jump_src    = 6,
      };

    using Storage = variant<
      ROCKET_CDR(
      , S_uninit      // 0,
      , S_void        // 1,
      , S_constant    // 2,
      , S_temporary   // 3,
      , S_variable    // 4,
      , S_tail_call   // 5,
      , S_jump_src    // 6,
      )>;

    static_assert(::std::is_nothrow_copy_assignable<Storage>::value);

  private:
    Storage m_stor;

  public:
    ASTERIA_VARIANT_CONSTRUCTOR(Reference_root, Storage, XRootT, xroot)
      : m_stor(::std::forward<XRootT>(xroot))
      { }

    ASTERIA_VARIANT_ASSIGNMENT(Reference_root, Storage, XRootT, xroot)
      {
        this->m_stor = ::std::forward<XRootT>(xroot);
        return *this;
      }

  public:
    Index
    index()
    const noexcept
      { return static_cast<Index>(this->m_stor.index());  }

    bool
    is_uninitialized()
    const noexcept
      { return this->index() == index_uninit;  }

    bool
    is_void()
    const noexcept
      { return this->index() == index_void;  }

    bool
    is_constant()
    const noexcept
      { return this->index() == index_constant;  }

    const Value&
    as_constant()
    const
      { return this->m_stor.as<index_constant>().val;  }

    bool
    is_temporary()
    const noexcept
      { return this->index() == index_temporary;  }

    const Value&
    as_temporary()
    const
      { return this->m_stor.as<index_temporary>().val;  }

    bool
    is_variable()
    const noexcept
      { return this->index() == index_variable;  }

    const rcfwdp<Variable>&
    as_variable()
    const
      { return this->m_stor.as<index_variable>().var;  }

    bool
    is_tail_call()
    const noexcept
      { return this->index() == index_tail_call;  }

    const rcfwdp<PTC_Arguments>&
    as_tail_call()
    const
      { return this->m_stor.as<index_tail_call>().tca;  }

    bool
    is_jump_src()
    const noexcept
      { return this->index() == index_jump_src;  }

    const Source_Location&
    as_jump_src()
    const
      { return this->m_stor.as<index_jump_src>().sloc;  }

    Reference_root&
    swap(Reference_root& other)
    noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    const Value&
    dereference_const()
    const;

    Value&
    dereference_mutable()
    const;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const;
  };

inline
void
swap(Reference_root& lhs, Reference_root& rhs)
noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
