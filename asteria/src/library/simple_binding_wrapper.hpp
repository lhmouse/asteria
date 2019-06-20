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
    using Prototype = Reference (const Value& opaque, const Global_Context& global, Reference&& self, Cow_Vector<Reference>&& args);

  private:
    Cow_String m_desc;
    Value m_opaque;
    Prototype* m_fptr;

  public:
    Simple_Binding_Wrapper(Cow_String&& desc, Value&& opaque, Prototype* fptr) noexcept
      : m_desc(rocket::move(desc)), m_opaque(rocket::move(opaque)), m_fptr(fptr)
      {
      }
    ~Simple_Binding_Wrapper() override;

  public:
    void describe(std::ostream& os) const override;
    Reference& invoke(Reference& self, const Global_Context& global, Cow_Vector<Reference>&& args) const override;
    void enumerate_variables(const Abstract_Variable_Callback& callback) const override;
  };

inline Rcobj<Simple_Binding_Wrapper> make_simple_binding(Cow_String desc, Value opaque, Simple_Binding_Wrapper::Prototype* fptr)
  {
    return Rcobj<Simple_Binding_Wrapper>(rocket::move(desc), rocket::move(opaque), fptr);
  }

}  // namespace Asteria

#endif
