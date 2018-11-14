// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_HPP_
#define ASTERIA_REFERENCE_HPP_

#include "fwd.hpp"
#include "reference_root.hpp"
#include "reference_modifier.hpp"
#include <functional>

namespace Asteria {

class Reference
  {
  private:
    Reference_root m_root;
    rocket::cow_vector<Reference_modifier> m_mods;

  public:
    Reference() noexcept
      : m_root(), m_mods()
      {
      }
    // This constructor does not accept lvalues.
    template<typename XrootT,
      typename std::enable_if<(Reference_root::Variant::index_of<XrootT>::value || true)>::type * = nullptr>
        Reference(XrootT &&xroot)
      : m_root(std::forward<XrootT>(xroot)), m_mods()
      {
      }
    // This assignment operator does not accept lvalues.
    template<typename XrootT,
      typename std::enable_if<(Reference_root::Variant::index_of<XrootT>::value || true)>::type * = nullptr>
        Reference & operator=(XrootT &&xroot)
      {
        this->m_root = std::forward<XrootT>(xroot);
        this->m_mods.clear();
        return *this;
      }
    ROCKET_COPYABLE_DESTRUCTOR(Reference);

  private:
    [[noreturn]] void do_throw_no_modifier() const;

  public:
    bool is_constant() const noexcept
      {
        return this->m_root.index() == Reference_root::index_constant;
      }

    Value read() const
      {
        // Dereference the root.
        auto cur = std::ref(this->m_root.dereference_const());
        // Apply modifiers.
        const auto end = this->m_mods.end();
        for(auto it = this->m_mods.begin(); it != end; ++it) {
          const auto qnext = it->apply_const_opt(cur);
          if(!qnext) {
            return { };
          }
          cur = std::ref(*qnext);
        }
        return cur;
      }
    template<typename ValueT>
      Value & write(ValueT &&value) const
      {
        // Dereference the root.
        auto cur = std::ref(this->m_root.dereference_mutable());
        // Apply modifiers.
        const auto end = this->m_mods.end();
        for(auto it = this->m_mods.begin(); it != end; ++it) {
          const auto qnext = it->apply_mutable_opt(cur, true);
          if(!qnext) {
            ROCKET_ASSERT(false);
          }
          cur = std::ref(*qnext);
        }
        return cur.get() = std::forward<ValueT>(value);
      }
    Value unset() const
      {
        if(this->m_mods.empty()) {
          this->do_throw_no_modifier();
        }
        // Dereference the root.
        auto cur = std::ref(this->m_root.dereference_mutable());
        // Apply modifiers.
        const auto end = this->m_mods.end() - 1;
        for(auto it = this->m_mods.begin(); it != end; ++it) {
          const auto qnext = it->apply_mutable_opt(cur, false);
          if(!qnext) {
            return { };
          }
          cur = std::ref(*qnext);
        }
        return end->apply_and_erase(cur);
      }

    template<typename XmodT>
      Reference & zoom_in(XmodT &&mod)
      {
        // Append a modifier.
        this->m_mods.emplace_back(std::forward<XmodT>(mod));
        return *this;
      }
    Reference & zoom_out()
      {
        if(this->m_mods.empty()) {
          // If there is no modifier, set `*this` to a null reference.
          Reference_root::S_constant ref_c = { D_null() };
          *this = std::move(ref_c);
          return *this;
        }
        // Drop the last modifier.
        this->m_mods.pop_back();
        return *this;
      }

    void enumerate_variables(const Abstract_variable_callback &callback) const
      {
        this->m_root.enumerate_variables(callback);
      }
    void dispose_variable(Global_context &global) const noexcept
      {
        this->m_root.dispose_variable(global);
      }

    Reference & convert_to_temporary();
    Reference & convert_to_variable(Global_context &global, bool immutable);
  };

}

#endif
