// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_HPP_
#define ASTERIA_REFERENCE_HPP_

#include "fwd.hpp"
#include "reference_root.hpp"
#include "reference_member_designator.hpp"

namespace Asteria
{

class Reference
  {
  private:
    Reference_root m_root;
    Vector<Reference_member_designator> m_mdsgs;

  public:
    Reference(Reference_root &&root)
      : m_root(std::move(root)), m_mdsgs()
      {
      }
    Reference(Reference_root &&root, Vector<Reference_member_designator> &&mdsgs)
      : m_root(std::move(root)), m_mdsgs(std::move(mdsgs))
      {
      }
    Reference(Reference &&) noexcept;
    Reference & operator=(Reference &&) noexcept;
    ~Reference();

  public:
    const Reference_root & get_root() const noexcept
      {
        return m_root;
      }
    Reference_root & get_root() noexcept
      {
        return m_root;
      }
    Reference_root & set_root(Reference_root &&root)
      {
        return m_root = std::move(root);
      }

    const Vector<Reference_member_designator> & get_member_designators() const noexcept
      {
        return m_mdsgs;
      }
    Vector<Reference_member_designator> & get_member_designators() noexcept
      {
        return m_mdsgs;
      }
    void clear_member_designators() noexcept
      {
        m_mdsgs.clear();
      }
    Reference_member_designator & push_member_designator(Reference_member_designator &&mdsg)
      {
        return m_mdsgs.emplace_back(std::move(mdsg));
      }
    void pop_member_designator()
      {
        m_mdsgs.pop_back();
      }
  };

extern Value read_reference(const Reference &ref);
extern void write_reference(const Reference &ref, Value &&value);

extern void materialize_reference(Reference &ref);

}

#endif
