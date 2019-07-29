// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_SIMPLE_BINDING_WRAPPER_HPP_
#define ASTERIA_LIBRARY_SIMPLE_BINDING_WRAPPER_HPP_

#include "../fwd.hpp"
#include "../runtime/abstract_function.hpp"
#include "../runtime/value.hpp"

namespace Asteria {

class Simple_Binding_Wrapper : public Abstract_Function
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
    std::ostream& describe(std::ostream& os) const override;
    Reference& invoke(Reference& self, const Global_Context& global, cow_vector<Reference>&& args) const override;
    Abstract_Variable_Callback& enumerate_variables(Abstract_Variable_Callback& callback) const override;
  };

inline rcobj<Simple_Binding_Wrapper> make_simple_binding(cow_string desc, Value opaque, Simple_Binding_Wrapper::Prototype* fptr)
  {
    return rcobj<Simple_Binding_Wrapper>(rocket::move(desc), rocket::move(opaque), fptr);
  }

}  // namespace Asteria

#endif
