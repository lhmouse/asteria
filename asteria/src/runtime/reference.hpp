// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_HPP_
#define ASTERIA_RUNTIME_REFERENCE_HPP_

#include "../fwd.hpp"
#include "../value.hpp"
#include "../source_location.hpp"

namespace asteria {

class Reference
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

    struct S_ptc_args
      {
        rcfwdp<PTC_Arguments> ptca;
      };

    struct S_jump_src
      {
        Source_Location sloc;
      };

    enum Index : uint8_t
      {
        index_uninit     = 0,
        index_void       = 1,
        index_constant   = 2,
        index_temporary  = 3,
        index_variable   = 4,
        index_ptc_args   = 5,
        index_jump_src   = 6,
      };

    struct M_array_index
      {
        int64_t index;
      };

    struct M_object_key
      {
        phsh_string key;
      };

    struct M_array_head
      {
      };

    struct M_array_tail
      {
      };

  private:
    using Root = ::rocket::variant<
      ROCKET_CDR(
        ,S_uninit     // 0,
        ,S_void       // 1,
        ,S_constant   // 2,
        ,S_temporary  // 3,
        ,S_variable   // 4,
        ,S_ptc_args   // 5,
        ,S_jump_src   // 6,
      )>;

    enum MIndex : uint8_t
      {
        mindex_array_index  = 0,
        mindex_object_key   = 1,
        mindex_array_head   = 2,
        mindex_array_tail   = 3,
      };

    using Modifier = ::rocket::variant<
      ROCKET_CDR(
        ,M_array_index  // 0,
        ,M_object_key   // 1,
        ,M_array_head   // 2,
        ,M_array_tail   // 3,
      )>;

    Root m_root;
    cow_vector<Modifier> m_mods;

  public:
    ASTERIA_VARIANT_CONSTRUCTOR(Reference, Root, XRootT, xroot)
      : m_root(::std::forward<XRootT>(xroot)),
        m_mods()
      { }

    ASTERIA_VARIANT_ASSIGNMENT(Reference, Root, XRootT, xroot)
      { this->m_root = ::std::forward<XRootT>(xroot);
        this->m_mods.clear();
        return *this;  }

    ASTERIA_COPYABLE_DESTRUCTOR(Reference);

  public:
    Index
    index()
      const noexcept
      { return static_cast<Index>(this->m_root.index());  }

    bool
    is_uninit()
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

    bool
    is_temporary()
      const noexcept
      { return this->index() == index_temporary;  }

    bool
    is_variable()
      const noexcept
      { return this->index() == index_variable;  }

    ASTERIA_INCOMPLET(Variable)
    rcptr<Variable>
    get_variable_opt()
      const noexcept
      { return ROCKET_EXPECT(this->is_variable())
          ? unerase_pointer_cast<Variable>(this->m_root.as<index_variable>().var)
          : nullptr;  }

    bool
    is_ptc_args()
      const noexcept
      { return this->index() == index_ptc_args;  }

    ASTERIA_INCOMPLET(PTC_Arguments)
    rcptr<PTC_Arguments>
    get_ptc_args_opt()
      const noexcept
      { return ROCKET_EXPECT(this->is_ptc_args())
          ? unerase_pointer_cast<PTC_Arguments>(this->m_root.as<index_ptc_args>().ptca)
          : nullptr;  }

    Reference&
    finish_call(Global_Context& global);

    bool
    is_jump_src()
      const noexcept
      { return this->index() == index_jump_src;  }

    const Source_Location&
    as_jump_src()
      const
      { return this->m_root.as<index_jump_src>().sloc;  }

    Reference&
    swap(Reference& other)
      noexcept
      {
        this->m_root.swap(other.m_root);
        this->m_mods.swap(other.m_mods);
        return *this;
      }

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
      const;

    // An lvalue is something writable.
    // An rvalue is something read-only.
    // Everything else is neither an lvalue or an rvalue.
    bool
    is_lvalue()
      const noexcept
      { return this->is_variable();  }

    bool
    is_rvalue()
      const noexcept
      { return this->is_constant() || this->is_temporary();  }

    // A glvalue is an lvalue or a subobject of an rvalue.
    // A prvalue is an rvalue that is not a subobject.
    // These are used to determine whether a value can be further normalized.
    bool
    is_glvalue()
      const noexcept
      { return this->is_lvalue() || (this->is_rvalue() && this->m_mods.size());  }

    bool
    is_prvalue()
      const noexcept
      { return this->is_rvalue() && this->m_mods.empty();  }

    // A modifier is created by a dot or bracket operator.
    // For instance, the expression `obj.x[42]` results in a reference having two
    // modifiers.
    // Modifiers can be removed to yield references to ancestor objects. Removing
    // the last modifier shall yield the constant `null`.
    template<typename XModT>
    Reference&
    zoom_in(XModT&& xmod)
      {
        this->m_mods.emplace_back(::std::forward<XModT>(xmod));
        return *this;
      }

    Reference&
    zoom_out()
      {
        if(ROCKET_EXPECT(this->m_mods.empty()))
          this->m_root = S_constant();
        else
          this->m_mods.pop_back();
        return *this;
      }

    // This function is called during argument passing to catch early errors.
    ROCKET_FORCED_INLINE_FUNCTION
    const Reference&
    dereference_validate()
      const
      {
        // If the root is a value, and there is no modifier, then dereferencing
        // will always succeed and yield the root.
        int mask = (1 << this->index())
                & (1 << index_constant | 1 << index_temporary | 1 << index_variable)
                & (static_cast<int32_t>(this->m_mods.ssize() - 1) >> 31);
        if(ROCKET_EXPECT(mask))
          return *this;

        // Otherwise, try dereferencing.
        // If the operation fails, an exception is thrown.
        static_cast<void>(this->dereference_readonly());
        return *this;
      }

    // These are general read/write functions.
    // Note that not all references denote values. Some of them are placeholders.
    const Value&
    dereference_readonly()
      const;

    Value&
    dereference_mutable()
      const;

    Value
    dereference_unset()
      const;

    Value&
    mutate_into_temporary();
  };

inline
void
swap(Reference& lhs, Reference& rhs)
  noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
