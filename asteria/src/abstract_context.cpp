// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "abstract_context.hpp"
#include "reference.hpp"

namespace Asteria {

Abstract_context::~Abstract_context()
  {
  }

void Abstract_context::do_clear_named_references() noexcept
  {
    this->m_named_refs.clear();
  }

bool Abstract_context::is_name_reserved(const String &name) const noexcept
  {
    return name.empty() || name.starts_with("__");
  }

const Reference * Abstract_context::get_named_reference_opt(const String &name) const noexcept
  {
    const auto it = this->m_named_refs.find(name);
    if(it == this->m_named_refs.end()) {
      return nullptr;
    }
    return &(it->second);
  }

void Abstract_context::set_named_reference(const String &name, Reference ref)
  {
    this->m_named_refs.insert_or_assign(name, std::move(ref));
  }

}
