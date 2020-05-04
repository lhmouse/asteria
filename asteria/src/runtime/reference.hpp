// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_HPP_
#define ASTERIA_RUNTIME_REFERENCE_HPP_

#include "../fwd.hpp"
#include "reference_root.hpp"
#include "reference_modifier.hpp"

namespace Asteria {

class Reference
  {
  private:
    Reference_root m_root;
    cow_vector<Reference_modifier> m_mods;

  public:
    ASTERIA_VARIANT_CONSTRUCTOR(Reference, Reference_root, XRootT, xroot)
      : m_root(::std::forward<XRootT>(xroot)),
        m_mods()
      { }

    ASTERIA_VARIANT_ASSIGNMENT(Reference, Reference_root, XRootT, xroot)
      {
        this->m_root = ::std::forward<XRootT>(xroot);
        this->m_mods.clear();
        return *this;
      }

    ASTERIA_COPYABLE_DESTRUCTOR(Reference);

  private:
    [[noreturn]]
    void
    do_throw_unset_no_modifier()
    const;

    const Value&
    do_read(size_t nmod, const Reference_modifier& last)
    const;

    Value&
    do_open(size_t nmod, const Reference_modifier& last)
    const;

    Value
    do_unset(size_t nmod, const Reference_modifier& last)
    const;

    Reference&
    do_finish_call(Global_Context& global);

  public:
    bool
    is_void()
    const noexcept
      { return this->m_root.is_void();  }

    bool
    is_constant()
    const noexcept
      { return this->m_root.is_constant();  }

    bool
    is_constant_null()
    const noexcept
      { return this->m_root.is_constant() && this->m_root.as_constant().is_null();  }

    bool
    is_temporary()
    const noexcept
      { return this->m_root.is_temporary();  }

    bool
    is_variable()
    const noexcept
      { return this->m_root.is_variable();  }

    bool
    is_tail_call()
    const noexcept
      { return this->m_root.is_tail_call();  }

    bool
    is_jump_src()
    const noexcept
      { return this->m_root.is_jump_src();  }

    bool
    is_lvalue()
    const noexcept
      { return this->is_variable();  }

    bool
    is_rvalue()
    const noexcept
      { return this->is_constant() || this->is_temporary();  }

    bool
    is_glvalue()
    const noexcept
      { return this->is_lvalue() || (this->is_rvalue() && !this->m_mods.empty());  }

    bool
    is_prvalue()
    const noexcept
      { return this->is_rvalue() && this->m_mods.empty();  }

    // Append a modifier.
    template<typename XModT>
    Reference&
    zoom_in(XModT&& xmod)
      {
        this->m_mods.emplace_back(::std::forward<XModT>(xmod));
        return *this;
      }

    // Drop the last modifier.
    // If there is no modifier, set `this` to `null`.
    Reference&
    zoom_out()
      {
        if(ROCKET_EXPECT(this->m_mods.empty()))
          this->m_root = Reference_root::S_constant();
        else
          this->m_mods.pop_back();
        return *this;
      }

    Reference&
    swap(Reference& other)
    noexcept
      {
        this->m_root.swap(other.m_root);
        this->m_mods.swap(other.m_mods);
        return *this;
      }

    const Value&
    read()
    const
      {
        if(ROCKET_EXPECT(this->m_mods.empty()))
          return this->m_root.dereference_const();
        else
          return this->do_read(this->m_mods.size() - 1, this->m_mods.back());
      }

    Value&
    open()
    const
      {
        if(ROCKET_EXPECT(this->m_mods.empty()))
          return this->m_root.dereference_mutable();
        else
          return this->do_open(this->m_mods.size() - 1, this->m_mods.back());
      }

    Value
    unset()
    const
      {
        if(ROCKET_EXPECT(this->m_mods.empty()))
          this->do_throw_unset_no_modifier();
        else
          return this->do_unset(this->m_mods.size() - 1, this->m_mods.back());
      }

    const Value&
    read(const Reference_modifier& last)
    const
      { return this->do_read(this->m_mods.size(), last);  }

    Value&
    open(const Reference_modifier& last)
    const
      { return this->do_open(this->m_mods.size(), last);  }

    Value
    unset(const Reference_modifier& last)
    const
      { return this->do_unset(this->m_mods.size(), last);  }

    ASTERIA_INCOMPLET(Variable)
    rcptr<Variable>
    get_variable_opt()
    const noexcept
      {
        if(ROCKET_UNEXPECT(!this->is_variable()))
          return nullptr;
        else
          return unerase_cast<Variable>(this->m_root.as_variable());
      }

    ASTERIA_INCOMPLET(PTC_Arguments)
    rcptr<PTC_Arguments>
    get_tail_call_opt()
    const noexcept
      {
        if(ROCKET_UNEXPECT(!this->is_tail_call()))
          return nullptr;
        else
          return unerase_cast<PTC_Arguments>(this->m_root.as_tail_call());
      }

    const Source_Location&
    as_jump_src()
    const
      { return this->m_root.as_jump_src();  }

    Reference&
    finish_call(Global_Context& global)
      {
        if(ROCKET_EXPECT(!this->is_tail_call()))
          return *this;
        else
          return this->do_finish_call(global);
      }

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const;
  };

inline
void
swap(Reference& lhs, Reference& rhs)
noexcept
  { lhs.swap(rhs);  }

}  // namespace Asteria

#endif
