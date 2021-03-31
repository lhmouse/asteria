// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_MODIFIER_HPP_
#define ASTERIA_RUNTIME_REFERENCE_MODIFIER_HPP_

#include "../fwd.hpp"

namespace asteria {

class Reference_Modifier
  {
  public:
    struct S_array_index
      {
        int64_t index;
      };

    struct S_object_key
      {
        phsh_string key;
      };

    struct S_array_head
      {
      };

    struct S_array_tail
      {
      };

    enum Index : uint8_t
      {
        index_array_index  = 0,
        index_object_key   = 1,
        index_array_head   = 2,
        index_array_tail   = 3,
      };

  private:
    ::rocket::variant<
      ROCKET_CDR(
        ,S_array_index  // 0,
        ,S_object_key   // 1,
        ,S_array_head   // 2,
        ,S_array_tail   // 3,
      )>
      m_stor;

  public:
    // Constructors and assignment operators
    template<typename XModT,
    ROCKET_ENABLE_IF(::std::is_constructible<decltype(m_stor), XModT&&>::value)>
    constexpr
    Reference_Modifier(XModT&& xmod)
      noexcept(::std::is_nothrow_constructible<decltype(m_stor), XModT&&>::value)
      : m_stor(::std::forward<XModT>(xmod))
      { }

    template<typename XModT,
    ROCKET_ENABLE_IF(::std::is_assignable<decltype(m_stor)&, XModT&&>::value)>
    Reference_Modifier&
    operator=(XModT&& xmod)
      noexcept(::std::is_nothrow_assignable<decltype(m_stor)&, XModT&&>::value)
      {
        this->m_stor = ::std::forward<XModT>(xmod);
        return *this;
      }

  public:
    Index
    index()
      const noexcept
      { return static_cast<Index>(this->m_stor.index());  }

    bool
    is_array_index()
      const noexcept
      { return this->index() == index_array_index;  }

    int64_t
    as_array_index()
      const
      { return this->m_stor.as<index_array_index>().index; }

    bool
    is_object_key()
      const noexcept
      { return this->index() == index_object_key;  }

    const cow_string&
    as_object_key()
      const
      { return this->m_stor.as<index_object_key>().key; }

    bool
    is_array_head()
      const noexcept
      { return this->index() == index_array_head;  }

    bool
    is_array_tail()
      const noexcept
      { return this->index() == index_array_tail;  }

    Reference_Modifier&
    swap(Reference_Modifier& other)
      noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    // Apply this modifier on a value.
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
swap(Reference_Modifier& lhs, Reference_Modifier& rhs)
  noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
