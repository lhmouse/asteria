// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "abstract_context.hpp"
#include "reference.hpp"

namespace Asteria {

bool Abstract_context::is_name_reserved(const String &name)
  {
    return name.empty() || name.starts_with("__");
  }

Abstract_context::~Abstract_context()
  {
  }

const Reference * Abstract_context::get_named_reference_opt(const String &name) const noexcept
  {
    const auto it = this->m_named_refs.find(name);
    if(it == this->m_named_refs.end()) {
      return nullptr;
    }
    return &(it->second);
  }
Reference * Abstract_context::get_named_reference_opt(const String &name) noexcept
  {
    const auto it = this->m_named_refs.find_mut(name);
    if(it == this->m_named_refs.end()) {
      return nullptr;
    }
    return &(it->second);
  }
Reference & Abstract_context::open_named_reference(const String &name)
  {
    return this->m_named_refs.try_emplace(name).first->second;
  }
void Abstract_context::set_named_reference(const String &name, Reference ref)
  {
    this->m_named_refs.insert_or_assign(name, std::move(ref));
  }

}
