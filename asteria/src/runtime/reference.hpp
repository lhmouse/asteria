// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_HPP_
#define ASTERIA_RUNTIME_REFERENCE_HPP_

#include "../fwd.hpp"
#include "../value.hpp"

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

    enum Index : uint8_t
      {
        index_uninit     = 0,
        index_void       = 1,
        index_constant   = 2,
        index_temporary  = 3,
        index_variable   = 4,
        index_ptc_args   = 5,
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

  private:
    Reference&
    do_finish_call_slow(Global_Context& global);

    const Value&
    do_dereference_readonly_slow()
      const;

    Value&
    do_mutate_into_temporary_slow();

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(Reference);

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
      {
        return ROCKET_EXPECT(this->is_variable())
          ? unerase_pointer_cast<Variable>(this->m_root.as<index_variable>().var)
          : nullptr;
      }

    bool
    is_ptc_args()
      const noexcept
      { return this->index() == index_ptc_args;  }

    ASTERIA_INCOMPLET(PTC_Arguments)
    rcptr<PTC_Arguments>
    get_ptc_args_opt()
      const noexcept
      {
        return ROCKET_UNEXPECT(this->is_ptc_args())
          ? unerase_pointer_cast<PTC_Arguments>(this->m_root.as<index_ptc_args>().ptca)
          : nullptr;
      }

    ROCKET_FORCED_INLINE_FUNCTION
    Reference&
    finish_call(Global_Context& global)
      {
        return ROCKET_UNEXPECT(this->is_ptc_args())
          ? this->do_finish_call_slow(global)
          : *this;
      }

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
      { return (1 << this->index()) & (1 << index_variable);  }

    bool
    is_rvalue()
      const noexcept
      { return (1 << this->index()) & (1 << index_constant | 1 << index_temporary);  }

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
    // modifiers. Modifiers can be removed to yield references to ancestor objects.
    // Removing the last modifier shall yield the constant `null`.
    template<typename XModT>
    Reference&
    zoom_in(XModT&& xmod)
      {
        // Push a new modifier.
        this->m_mods.emplace_back(::std::forward<XModT>(xmod));
        return *this;
      }

    Reference&
    zoom_out()
      {
        if(ROCKET_UNEXPECT(this->m_mods.size())) {
          // Drop the last modifier.
          this->m_mods.pop_back();
        }
        else {
          // Overwrite the root with a constant null.
          auto qxroot = this->m_root.get<index_constant>();
          if(qxroot)
            qxroot->val = V_null();
          else
            this->m_root = S_constant();
        }
        return *this;
      }

    // These are general read/write functions.
    // Note that not all references denote values. Some of them are placeholders.
    ROCKET_FORCED_INLINE_FUNCTION
    const Value&
    dereference_readonly()
      const
      {
        // If the root is an rvalue, and there is no modifier, then dereferencing
        // will always succeed and yield the root.
        if(ROCKET_EXPECT(this->m_mods.empty())) {
          // Perform fast indirection.
          if(this->m_root.index() == index_constant)
            return this->m_root.as<index_constant>().val;

          if(this->m_root.index() == index_temporary)
            return this->m_root.as<index_temporary>().val;
        }

        // Otherwise, try dereferencing.
        return this->do_dereference_readonly_slow();
      }

    ROCKET_FORCED_INLINE_FUNCTION
    Value&
    mutate_into_temporary()
      {
        // If the root is a temporary, and there is no modifier, then this
        // function can return the value directly.
        if(ROCKET_EXPECT(this->m_mods.empty())) {
          // Perform fast indirection.
          if(this->m_root.index() == index_temporary)
            return this->m_root.as<index_temporary>().val;
        }

        // Otherwise, try dereferencing.
        return this->do_mutate_into_temporary_slow();
      }

    Value&
    dereference_mutable()
      const;

    Value
    dereference_unset()
      const;
  };

inline
void
swap(Reference& lhs, Reference& rhs)
  noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
