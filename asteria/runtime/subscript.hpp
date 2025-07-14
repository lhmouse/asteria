// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_SUBSCRIPT_
#define ASTERIA_RUNTIME_SUBSCRIPT_

#include "../fwd.hpp"
namespace asteria {

class Subscript
  {
  public:
    struct S_array_index
      {
        int64_t index;
      };

    struct S_object_key
      {
        phcow_string key;
      };

    struct S_array_head
      {
      };

    struct S_array_tail
      {
      };

    struct S_array_random
      {
        uint32_t seed;
      };

    enum Index : uint8_t
      {
        index_array_index   = 0,
        index_object_key    = 1,
        index_array_head    = 2,
        index_array_tail    = 3,
        index_array_random  = 4,
      };

  private:
    ASTERIA_VARIANT(
      m_stor
        , S_array_index   // 0,
        , S_object_key    // 1,
        , S_array_head    // 2,
        , S_array_tail    // 3,
        , S_array_random  // 4,
      );

  public:
    // Constructors and assignment operators
    template<typename xSubscript,
    ROCKET_ENABLE_IF(::std::is_constructible<decltype(m_stor), xSubscript&&>::value)>
    constexpr Subscript(xSubscript&& xsub)
      noexcept(::std::is_nothrow_constructible<decltype(m_stor), xSubscript&&>::value)
      :
        m_stor(forward<xSubscript>(xsub))
      { }

    template<typename xSubscript,
    ROCKET_ENABLE_IF(::std::is_assignable<decltype(m_stor)&, xSubscript&&>::value)>
    Subscript&
    operator=(xSubscript&& xsub)
      & noexcept(::std::is_nothrow_assignable<decltype(m_stor)&, xSubscript&&>::value)
      {
        this->m_stor = forward<xSubscript>(xsub);
        return *this;
      }

    Subscript&
    swap(Subscript& other)
      noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    bool
    is_array_index()
      const noexcept
      { return this->m_stor.index() == index_array_index;  }

    int64_t
    as_array_index()
      const
      { return this->m_stor.as<S_array_index>().index; }

    bool
    is_object_key()
      const noexcept
      { return this->m_stor.index() == index_object_key;  }

    const phcow_string&
    as_object_key()
      const
      { return this->m_stor.as<S_object_key>().key; }

    bool
    is_array_head()
      const noexcept
      { return this->m_stor.index() == index_array_head;  }

    bool
    is_array_tail()
      const noexcept
      { return this->m_stor.index() == index_array_tail;  }

    bool
    is_array_random()
      const noexcept
      { return this->m_stor.index() == index_array_random;  }

    // Apply this subscript on a value.
    const Value*
    apply_read_opt(const Value& parent)
      const;

    Value*
    apply_write_opt(Value& parent)
      const;

    Value&
    apply_open(Value& parent)
      const;

    Value
    apply_unset(Value& parent)
      const;
  };

inline
void
swap(Subscript& lhs, Subscript& rhs)
  noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria
#endif
