// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_SIMPLE_BINDING_WRAPPER_HPP_
#define ASTERIA_LIBRARY_SIMPLE_BINDING_WRAPPER_HPP_

#include "../fwd.hpp"
#include "../runtime/abstract_function.hpp"
#include "../runtime/value.hpp"

namespace Asteria {

class Simple_Binding_Wrapper final : public Abstract_Function
  {
  public:
    using Prototype = Reference (const Value& opaque, const Global_Context& global, Reference&& self, cow_vector<Reference>&& args);

  private:
    cow_string m_desc;
    Value m_opaque;
    Prototype* m_fptr;

  public:
    Simple_Binding_Wrapper(cow_string&& desc, Value&& opaque, Prototype* fptr) noexcept
      : m_desc(rocket::move(desc)), m_opaque(rocket::move(opaque)), m_fptr(fptr)
      {
      }
    ~Simple_Binding_Wrapper() override;

  public:
    tinyfmt& describe(tinyfmt& fmt) const override;
    Reference& invoke(Reference& self, const Global_Context& global, cow_vector<Reference>&& args) const override;
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const override;
  };

}  // namespace Asteria

#endif
