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
    using Prototype = Reference (std::intptr_t iparam, const RefCnt_Ptr<RefCnt_Base> &pparam, Cow_Vector<Reference> &&args);

  private:
    Cow_String m_desc;

    Prototype *m_sfunc;
    std::intptr_t m_iparam;
    RefCnt_Ptr<RefCnt_Base> m_pparam;

  public:
    Simple_Binding_Wrapper(Cow_String desc, Prototype *sfunc, std::intptr_t iparam, RefCnt_Ptr<RefCnt_Base> pparam) noexcept
      : m_desc(std::move(desc)),
        m_sfunc(sfunc), m_iparam(iparam), m_pparam(std::move(pparam))
      {
      }
    ~Simple_Binding_Wrapper() override;

  public:
    void describe(std::ostream &os) const override;
    void invoke(Reference &self_io, const Global_Context &global, Cow_Vector<Reference> &&args) const override;
    void enumerate_variables(const Abstract_Variable_Callback &callback) const override;
  };

inline RefCnt_Object<Simple_Binding_Wrapper> make_simple_binding(Cow_String desc, Simple_Binding_Wrapper::Prototype *sfunc, std::intptr_t iparam, RefCnt_Ptr<RefCnt_Base> pparam)
  {
    return RefCnt_Object<Simple_Binding_Wrapper>(std::move(desc), sfunc, iparam, std::move(pparam));
  }

}

#endif
