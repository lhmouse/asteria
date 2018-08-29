// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_HPP_
#define ASTERIA_REFERENCE_HPP_

#include "fwd.hpp"
#include "reference_root.hpp"
#include "reference_modifier.hpp"

namespace Asteria {

class Reference
  {
  private:
    Reference_root m_root;
    Vector<Reference_modifier> m_modifiers;

  public:
    Reference() noexcept
      : m_root(), m_modifiers()
      {
      }
    template<typename RootT, typename std::enable_if<std::is_constructible<Reference_root, RootT &&>::value>::type * = nullptr>
    Reference(RootT &&root, Vector<Reference_modifier> modifiers = { })
      : m_root(std::forward<RootT>(root)), m_modifiers(std::move(modifiers))
      {
      }
    ~Reference();

    Reference(const Reference &) noexcept;
    Reference & operator=(const Reference &) noexcept;
    Reference(Reference &&) noexcept;
    Reference & operator=(Reference &&) noexcept;

  public:
    const Reference_root & get_root() const noexcept
      {
        return m_root;
      }
    std::size_t get_modifier_count() const noexcept
      {
        return m_modifiers.size();
      }
    const Reference_modifier & get_modifier(std::size_t pos) const
      {
        return m_modifiers.at(pos);
      }
    Reference_root & set_root(Reference_root root, Vector<Reference_modifier> modifiers = { })
      {
        m_root = std::move(root);
        m_modifiers = std::move(modifiers);
        return m_root;
      }
    void clear_modifiers() noexcept
      {
        m_modifiers.clear();
      }
    Reference_modifier & push_modifier(Reference_modifier modifier)
      {
        return m_modifiers.emplace_back(std::move(modifier));
      }
    void pop_modifier()
      {
        m_modifiers.pop_back();
      }
  };

extern Value read_reference(const Reference &ref);
extern Value & write_reference(const Reference &ref, Value value);
extern Value unset_reference(const Reference &ref);

extern Reference reference_constant(Value value);
extern Reference reference_temp_value(Value value);

extern Reference & materialize_reference(Reference &ref);
extern Reference & dematerialize_reference(Reference &ref);

}

#endif
