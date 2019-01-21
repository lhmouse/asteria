// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_CONTEXT_HPP_

#include "../fwd.hpp"
#include "reference_dictionary.hpp"

namespace Asteria {

class Abstract_Context
  {
  private:
    RefCnt_Ptr<RefCnt_Base> m_tied_collector_opt;
    Reference_Dictionary m_named_references;

  public:
    Abstract_Context() noexcept
      : m_tied_collector_opt(), m_named_references()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Abstract_Context, virtual);

  protected:
    void do_tie_collector(const RefCnt_Ptr<Generational_Collector> &tied_collector_opt) noexcept;
    void do_set_named_reference_templates(const Reference_Dictionary::Template *tdata_opt, std::size_t tsize) noexcept;

  public:
    const Reference * get_named_reference_opt(const PreHashed_String &name) const
      {
        return this->m_named_references.get_opt(name);
      }
    Reference & open_named_reference(const PreHashed_String &name)
      {
        return this->m_named_references.open(name);
      }
    void clear_named_references() noexcept
      {
        this->m_named_references.clear();
      }

    virtual bool is_analytic() const noexcept = 0;
    virtual const Abstract_Context * get_parent_opt() const noexcept = 0;
  };

}

#endif
