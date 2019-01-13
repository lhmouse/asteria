// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "global_context.hpp"
#include "generational_collector.hpp"
#include "variable.hpp"
#include "executive_context.hpp"
#include "reference_stack.hpp"
#include "../utilities.hpp"

namespace Asteria {

Global_context::~Global_context()
  {
    // Perform the final garbage collection.
    try {
      this->clear_named_references();
      this->m_gen_coll.perform_garbage_collection(UINT_MAX);
    } catch(std::exception &e) {
      ASTERIA_DEBUG_LOG("An exception was thrown during final garbage collection and some resources might have leaked: ", e.what());
    }
  }

void Global_context::do_add_std_bindings()
  {
    D_object root;
    ASTERIA_DEBUG_LOG("TODO add std library");
    Reference_root::S_constant ref_c = { std::move(root) };
    this->open_named_reference(std::ref("std")) = std::move(ref_c);
  }

}
