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
  public:
    static Reference make_constant(Value value);
    static Reference make_temporary(Value value);

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
        return this->m_root;
      }
    std::size_t get_modifier_count() const noexcept
      {
        return this->m_modifiers.size();
      }
    const Reference_modifier & get_modifier(std::size_t pos) const
      {
        return this->m_modifiers.at(pos);
      }

    Reference_root & set_root(Reference_root root, Vector<Reference_modifier> modifiers = { });
    void clear_modifiers() noexcept;
    Reference_modifier & push_modifier(Reference_modifier modifier);
    void pop_modifier();

    Value read() const;
    Value & write(Value value) const;
    Value unset() const;

    Reference & materialize();
    Reference & dematerialize();
  };

}

#endif
