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
    Cow_Vector<Reference_Modifier> m_mods;

  public:
    Reference() noexcept
      : m_root(),
        m_mods()
      {
      }
    template<typename XrootT, ASTERIA_SFINAE_CONSTRUCT(Reference_Root, XrootT&&)> Reference(XrootT&& xroot) noexcept
      : m_root(rocket::forward<XrootT>(xroot)),
        m_mods()
      {
      }
    template<typename XrootT, ASTERIA_SFINAE_ASSIGN(Reference_Root, XrootT&&)> Reference& operator=(XrootT&& xroot) noexcept
      {
        this->m_root = rocket::forward<XrootT>(xroot);
        this->m_mods.clear();
        return *this;
      }

  private:
    [[noreturn]] Value do_throw_unset_no_modifier() const;

    const Value& do_read(const Reference_Modifier* mods, std::size_t nmod, const Reference_Modifier& last) const;
    Value& do_open(const Reference_Modifier* mods, std::size_t nmod, const Reference_Modifier& last) const;
    Value do_unset(const Reference_Modifier* mods, std::size_t nmod, const Reference_Modifier& last) const;

    Reference& do_finish_call(const Global_Context& global);

  public:
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
    bool is_rvalue() const noexcept
      {
        return this->m_root.is_rvalue();
      }

    template<typename XmodT> Reference& zoom_in(XmodT&& xmod)
      {
        // Append a modifier.
        this->m_mods.emplace_back(rocket::forward<XmodT>(xmod));
        return *this;
      }
    Reference& zoom_out()
      {
        if(ROCKET_EXPECT(this->m_mods.empty())) {
          // If there is no modifier, set `this` to `null`.
          this->m_root = Reference_Root::S_null();
          return *this;
        }
        // Drop the last modifier.
        this->m_mods.pop_back();
        return *this;
      }

    void swap(Reference& other) noexcept
      {
        this->m_root.swap(other.m_root);
        this->m_mods.swap(other.m_mods);
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

    Reference& finish_call(const Global_Context& global)
      {
        if(!this->m_root.is_tail_call()) {
          return *this;
        }
        return this->do_finish_call(global);
      }

    void enumerate_variables(const Abstract_Variable_Callback& callback) const;
  };

inline void swap(Reference& lhs, Reference& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
