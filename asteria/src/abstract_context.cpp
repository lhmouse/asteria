// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "abstract_context.hpp"

namespace Asteria {

Abstract_context::~Abstract_context()
  {
  }

const Reference * Abstract_context::get_named_reference_opt(const String &name) const noexcept
  {
    const auto it = m_named_refs.find(name);
    if(it == m_named_refs.end()) {
      return nullptr;
    }
    return &(it->second);
  }
Reference * Abstract_context::get_named_reference_opt(const String &name) noexcept
  {
    const auto it = m_named_refs.find_mut(name);
    if(it == m_named_refs.end()) {
      return nullptr;
    }
    return &(it->second);
  }
Reference & Abstract_context::set_named_reference(const String &name, Reference ref)
  {
    const auto pair = m_named_refs.insert_or_assign(name, std::move(ref));
    return pair.first->second;
  }

bool is_name_reserved(const String &name)
  {
    return name.empty() || name.starts_with("__");
  }

}
