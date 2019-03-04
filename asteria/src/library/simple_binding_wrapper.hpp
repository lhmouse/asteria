// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_SIMPLE_BINDING_WRAPPER_HPP_
#define ASTERIA_LIBRARY_SIMPLE_BINDING_WRAPPER_HPP_

#include "../fwd.hpp"
#include "../runtime/abstract_function.hpp"

namespace Asteria {

class Simple_Binding_Wrapper : public Abstract_Function
  {
  public:
    struct Opaque
      {
        std::int64_t ll;
        std::intptr_t i;
        Cow_String s;
        Rcptr<Rcbase> p;
      };
    using Prototype = Reference (const Opaque &opaque, Cow_Vector<Reference> &&args);

  private:
    Cow_String m_desc;
    Prototype *m_fptr;
    Opaque m_opaque;

  public:
    Simple_Binding_Wrapper(Cow_String desc, Prototype *fptr, Opaque opaque)
      : m_desc(rocket::move(desc)),
        m_fptr(fptr), m_opaque(rocket::move(opaque))
      {
      }
    ~Simple_Binding_Wrapper() override;

  public:
    void describe(std::ostream &os) const override;
    void invoke(Reference &self_io, const Global_Context &global, Cow_Vector<Reference> &&args) const override;
    void enumerate_variables(const Abstract_Variable_Callback &callback) const override;
  };

inline Rcobj<Simple_Binding_Wrapper> make_simple_binding(Cow_String desc, Simple_Binding_Wrapper::Prototype *fptr, Simple_Binding_Wrapper::Opaque opaque)
  {
    return Rcobj<Simple_Binding_Wrapper>(rocket::move(desc), fptr, rocket::move(opaque));
  }
inline Rcobj<Simple_Binding_Wrapper> make_simple_binding(Cow_String desc, Simple_Binding_Wrapper::Prototype *fptr)
  {
    return Rcobj<Simple_Binding_Wrapper>(rocket::move(desc), fptr, Simple_Binding_Wrapper::Opaque());
  }

}

#endif
