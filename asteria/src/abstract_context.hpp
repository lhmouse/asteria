// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ABSTRACT_CONTEXT_HPP_
#define ASTERIA_ABSTRACT_CONTEXT_HPP_

#include "fwd.hpp"
#include "reference.hpp"

namespace Asteria {

class Abstract_context
  {
  private:
    Dictionary<Reference> m_named_refs;

  public:
    Abstract_context() noexcept
      : m_named_refs()
      {
      }
    virtual ~Abstract_context();

    Abstract_context(const Abstract_context &)
      = delete;
    Abstract_context & operator=(const Abstract_context &)
      = delete;

  public:
    virtual bool is_analytic() const noexcept = 0;
    virtual const Abstract_context * get_parent_opt() const noexcept = 0;

    const Reference * get_named_reference_opt(const String &name) const noexcept
      {
        const auto it = m_named_refs.find(name);
        if(it == m_named_refs.end()) {
          return nullptr;
        }
        return &(it->second);
      }
    Reference * get_named_reference_opt(const String &name) noexcept
      {
        const auto it = m_named_refs.find_mut(name);
        if(it == m_named_refs.end()) {
          return nullptr;
        }
        return &(it->second);
      }
    Reference & set_named_reference(const String &name, Reference ref)
      {
        const auto pair = m_named_refs.insert_or_assign(name, std::move(ref));
        return pair.first->second;
      }
  };

extern bool is_name_reserved(const String &name);

}

#endif
