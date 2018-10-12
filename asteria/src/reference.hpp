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
    Vector<Reference_modifier> m_mods;

  public:
    Reference() noexcept
      : m_root(), m_mods()
      {
      }
    // This constructor does not accept lvalues.
    template<typename XrootT, typename std::enable_if<(Reference_root::Variant::index_of<XrootT>::value || true)>::type * = nullptr>
      Reference(XrootT &&xroot)
        : m_root(std::forward<XrootT>(xroot)), m_mods()
        {
        }
    // This assignment operator does not accept lvalues.
    template<typename XrootT, typename std::enable_if<(Reference_root::Variant::index_of<XrootT>::value || true)>::type * = nullptr>
      Reference & operator=(XrootT &&xroot)
        {
          this->m_root = std::forward<XrootT>(xroot);
          this->m_mods.clear();
          return *this;
        }
    ~Reference();

    Reference(const Reference &) noexcept;
    Reference & operator=(const Reference &) noexcept;
    Reference(Reference &&) noexcept;
    Reference & operator=(Reference &&) noexcept;

  public:
    bool is_constant() const noexcept
      {
        return this->m_root.index() == Reference_root::index_constant;
      }

    Value read() const;
    Value & write(Value value) const;
    Value unset() const;

    Reference & zoom_in(Reference_modifier mod);
    Reference & zoom_out();

    Reference & convert_to_temporary();
    Reference & convert_to_variable(Global_context &global);

    void collect_variables(bool (*callback)(void *, const rocket::refcounted_ptr<Variable> &), void *param) const;
  };

}

#endif
