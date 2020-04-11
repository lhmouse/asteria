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
  public:
    using Root      = Reference_root;
    using Modifier  = Reference_modifier;

  private:
    Root m_root;
    cow_vector<Modifier> m_mods;

  public:
    ASTERIA_VARIANT_CONSTRUCTOR(Reference, Root, XRootT, xroot)
      : m_root(::std::forward<XRootT>(xroot)),
        m_mods()
      { }

    ASTERIA_VARIANT_ASSIGNMENT(Reference, Root, XRootT, xroot)
      {
        this->m_root = ::std::forward<XRootT>(xroot);
        this->m_mods.clear();
        return *this;
      }

    ~Reference();

  private:
    [[noreturn]] void do_throw_unset_no_modifier() const;

    const Value& do_read(const Modifier* mods, size_t nmod, const Modifier& last) const;
    Value& do_open(const Modifier* mods, size_t nmod, const Modifier& last) const;
    Value do_unset(const Modifier* mods, size_t nmod, const Modifier& last) const;

    Reference& do_finish_call(Global_Context& global);

  public:
    bool is_void() const noexcept
      { return this->m_root.is_void();  }

    bool is_constant() const noexcept
      { return this->m_root.is_constant();  }

    bool is_temporary() const noexcept
      { return this->m_root.is_temporary();  }

    bool is_variable() const noexcept
      { return this->m_root.is_variable();  }

    bool is_tail_call() const noexcept
      { return this->m_root.is_tail_call();  }

    bool is_lvalue() const noexcept
      { return this->is_variable();  }

    bool is_rvalue() const noexcept
      { return this->is_constant() || this->is_temporary();  }

    bool is_glvalue() const noexcept
      { return this->is_lvalue() || !this->m_mods.empty();  }

    bool is_prvalue() const noexcept
      { return this->is_rvalue() && this->m_mods.empty();  }

    template<typename XModT> Reference& zoom_in(XModT&& xmod)
      {
        // Append a modifier.
        this->m_mods.emplace_back(::std::forward<XModT>(xmod));
        return *this;
      }

    Reference& zoom_out()
      {
        // Drop the last modifier. If there is no modifier, set `this` to `null`.
        if(ROCKET_EXPECT(this->m_mods.empty()))
          this->m_root = Root::S_constant();
        else
          this->m_mods.pop_back();
        return *this;
      }

    Reference& swap(Reference& other) noexcept
      {
        this->m_root.swap(other.m_root);
        this->m_mods.swap(other.m_mods);
        return *this;
      }

    const Value& read() const
      {
        if(ROCKET_EXPECT(this->m_mods.empty()))
          return this->m_root.dereference_const();
        else
          return this->do_read(this->m_mods.data(), this->m_mods.size() - 1, this->m_mods.back());
      }

    Value& open() const
      {
        if(ROCKET_EXPECT(this->m_mods.empty()))
          return this->m_root.dereference_mutable();
        else
          return this->do_open(this->m_mods.data(), this->m_mods.size() - 1, this->m_mods.back());
      }

    Value unset() const
      {
        if(ROCKET_EXPECT(this->m_mods.empty()))
          this->do_throw_unset_no_modifier();
        else
          return this->do_unset(this->m_mods.data(), this->m_mods.size() - 1, this->m_mods.back());
      }

    const Value& read(const Modifier& last) const
      { return this->do_read(this->m_mods.data(), this->m_mods.size(), last);  }

    Value& open(const Modifier& last) const
      { return this->do_open(this->m_mods.data(), this->m_mods.size(), last);  }

    Value unset(const Modifier& last) const
      { return this->do_unset(this->m_mods.data(), this->m_mods.size(), last);  }

    ASTERIA_INCOMPLET(Variable) rcptr<Variable> get_variable_opt() const noexcept
      {
        if(ROCKET_UNEXPECT(!this->is_variable()))
          return nullptr;
        else
          return unerase_cast<Variable>(this->m_root.as_variable());
      }

    ASTERIA_INCOMPLET(PTC_Arguments) rcptr<PTC_Arguments> get_tail_call_opt() const noexcept
      {
        if(ROCKET_UNEXPECT(!this->is_tail_call()))
          return nullptr;
        else
          return unerase_cast<PTC_Arguments>(this->m_root.as_tail_call());
      }

    Reference& finish_call(Global_Context& global)
      {
        if(ROCKET_EXPECT(!this->is_tail_call()))
          return *this;
        else
          return this->do_finish_call(global);
      }

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const;
  };

inline void swap(Reference& lhs, Reference& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace Asteria

#endif
