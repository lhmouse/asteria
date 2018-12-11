// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_HPP_
#define ASTERIA_RUNTIME_REFERENCE_HPP_

#include "../fwd.hpp"
#include "reference_root.hpp"
#include "reference_modifier.hpp"
#include "../rocket/cow_vector.hpp"

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
    template<typename XrootT, ROCKET_ENABLE_IF_HAS_VALUE(Reference_root::Variant::index_of<XrootT>::value)>
      Reference(XrootT &&xroot)
      : m_root(std::forward<XrootT>(xroot)), m_mods()
      {
      }
    // This assignment operator does not accept lvalues.
    template<typename XrootT, ROCKET_ENABLE_IF_HAS_VALUE(Reference_root::Variant::index_of<XrootT>::value)>
      Reference & operator=(XrootT &&xroot)
      {
        this->m_root = std::forward<XrootT>(xroot);
        this->m_mods.clear();
        return *this;
      }
    ROCKET_COPYABLE_DESTRUCTOR(Reference);

  private:
    [[noreturn]] void do_throw_unset_no_modifier() const;

    const Value & do_read_with_modifiers() const;
    Value & do_open_with_modifiers() const;
    Value do_unset_with_modifiers() const;

  public:
    bool is_constant() const noexcept
      {
        return (this->m_root.index() == Reference_root::index_null) || (this->m_root.index() == Reference_root::index_constant);
      }
    bool is_temporary() const noexcept
      {
        return this->m_root.index() == Reference_root::index_temporary;
      }

    const Value & read() const
      {
        if(ROCKET_EXPECT(this->m_mods.empty())) {
          return this->m_root.dereference_const();
        }
        return this->do_read_with_modifiers();
      }
    Value & open() const
      {
        if(ROCKET_EXPECT(this->m_mods.empty())) {
          return this->m_root.dereference_mutable();
        }
        return this->do_open_with_modifiers();
      }
    Value unset() const
      {
        if(ROCKET_UNEXPECT(this->m_mods.empty())) {
          this->do_throw_unset_no_modifier();
        }
        return this->do_unset_with_modifiers();
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
          this->m_root = Reference_root::S_null();
          return *this;
        }
        // Drop the last modifier.
        this->m_mods.pop_back();
        return *this;
      }

    void enumerate_variables(const Abstract_variable_callback &callback) const;
  };

}

#endif
