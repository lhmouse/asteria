// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_HPP_
#define ASTERIA_RUNTIME_REFERENCE_HPP_

#include "../fwd.hpp"
#include "reference_root.hpp"
#include "reference_modifier.hpp"

namespace Asteria {

class Reference
  {
  private:
    Reference_Root m_root;
    cow_vector<Reference_Modifier> m_mods;

  public:
    Reference() noexcept
      :
        m_root(), m_mods()
      {
      }
    ASTERIA_VARIANT_CONSTRUCTOR(Reference, Reference_Root, XRootT, xroot)
      :
        m_root(::rocket::forward<XRootT>(xroot)), m_mods()
      {
      }
    ASTERIA_VARIANT_ASSIGNMENT(Reference, Reference_Root, XRootT, xroot)
      {
        this->m_root = ::rocket::forward<XRootT>(xroot);
        this->m_mods.clear();
        return *this;
      }

  private:
    [[noreturn]] Value do_throw_unset_no_modifier() const;

    const Value& do_read(const Reference_Modifier* mods, size_t nmod, const Reference_Modifier& last) const;
    Value& do_open(const Reference_Modifier* mods, size_t nmod, const Reference_Modifier& last) const;
    Value do_unset(const Reference_Modifier* mods, size_t nmod, const Reference_Modifier& last) const;

    Reference& do_convert_to_temporary();
    Reference& do_finish_call(Global_Context& global);

  public:
    bool is_void() const noexcept
      {
        return this->m_root.is_void();
      }
    bool is_constant() const noexcept
      {
        return this->m_root.is_constant();
      }
    bool is_temporary() const noexcept
      {
        return this->m_root.is_temporary();
      }
    bool is_variable() const noexcept
      {
        return this->m_root.is_variable();
      }
    bool is_tail_call() const noexcept
      {
        return this->m_root.is_tail_call();
      }

    bool is_lvalue() const noexcept
      {
        return this->is_variable();
      }
    bool is_rvalue() const noexcept
      {
        return this->is_constant() || this->is_temporary();
      }

    template<typename XModT> Reference& zoom_in(XModT&& xmod)
      {
        // Append a modifier.
        this->m_mods.emplace_back(::rocket::forward<XModT>(xmod));
        return *this;
      }
    Reference& zoom_out()
      {
        // Drop the last modifier. If there is no modifier, set `this` to `null`.
        if(ROCKET_EXPECT(this->m_mods.empty()))
          this->m_root = Reference_Root::S_void();
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
        if(this->m_mods.empty()) {
          return this->m_root.dereference_const();
        }
        return this->do_read(this->m_mods.data(), this->m_mods.size() - 1, this->m_mods.back());
      }
    const Value& read(const Reference_Modifier& last) const
      {
        return this->do_read(this->m_mods.data(), this->m_mods.size(), last);
      }
    Value& open() const
      {
        if(this->m_mods.empty()) {
          return this->m_root.dereference_mutable();
        }
        return this->do_open(this->m_mods.data(), this->m_mods.size() - 1, this->m_mods.back());
      }
    Value& open(const Reference_Modifier& last) const
      {
        return this->do_open(this->m_mods.data(), this->m_mods.size(), last);
      }
    Value unset() const
      {
        if(this->m_mods.empty()) {
          return this->do_throw_unset_no_modifier();
        }
        return this->do_unset(this->m_mods.data(), this->m_mods.size() - 1, this->m_mods.back());
      }
    Value unset(const Reference_Modifier& last) const
      {
        return this->do_unset(this->m_mods.data(), this->m_mods.size(), last);
      }

    rcptr<Variable> get_variable_opt() const
      {
        if(!this->is_variable()) {
          return nullptr;
        }
        return this->m_root.as_variable();
      }
    Reference& convert_to_rvalue()
      {
        if(ROCKET_EXPECT(this->is_rvalue() && this->m_mods.empty())) {
          return *this;
        }
        return this->do_convert_to_temporary();
      }
    Reference& finish_call(Global_Context& global)
      {
        if(ROCKET_EXPECT(!this->m_root.is_tail_call())) {
          return *this;
        }
        return this->do_finish_call(global);
      }

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const;
  };

inline void swap(Reference& lhs, Reference& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
