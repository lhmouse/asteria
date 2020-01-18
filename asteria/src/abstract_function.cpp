// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "abstract_function.hpp"
#include "runtime/reference.hpp"

namespace Asteria {

Abstract_Function::~Abstract_Function()
  {
  }

Reference& Abstract_Function::invoke(Reference& self, Global_Context& global, cow_vector<Reference>&& args)
  {
    this->invoke_ptc_aware(self, global, ::rocket::move(args));
    return self.finish_call(global);
  }

Reference Abstract_Function::invoke(Global_Context& global, cow_vector<Reference>&& args)
  {
    Reference self = Reference_Root::S_constant();
    this->invoke(self, global, ::rocket::move(args));
    return self;
  }

}  // namespace Asteria
