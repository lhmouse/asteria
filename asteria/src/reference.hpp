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
      Reference(RootT &&root)
        : m_root(std::forward<RootT>(root)), m_modifiers()
        {
        }
    ~Reference();

    Reference(const Reference &) noexcept;
    Reference & operator=(const Reference &) noexcept;
    Reference(Reference &&) noexcept;
    Reference & operator=(Reference &&) noexcept;

  public:
    Value read() const;
    Value & write(Value value) const;
    Value unset() const;

    Reference & zoom_in(Reference_modifier modifier);
    Reference & zoom_out();

    Reference & materialize();
    Reference & dematerialize();
  };

}

#endif
