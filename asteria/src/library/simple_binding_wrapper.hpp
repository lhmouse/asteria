// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_SIMPLE_BINDING_WRAPPER_HPP_
#define ASTERIA_LIBRARY_SIMPLE_BINDING_WRAPPER_HPP_

#include "../fwd.hpp"
#include "../value.hpp"
#include "../abstract_function.hpp"

namespace Asteria {

class Simple_Binding_Wrapper final : public Abstract_Function
  {
  public:
    // This is the complete prototype of binding callbacks.
    // Parameters that are less likely to be useful come more behind and can be conditionally omitted.
    using Prototype_4  = Value (cow_vector<Reference>&& args,  // required if takes arguments
                                Reference&& self,              // required if is a member function
                                Global& global,                // required if wishes to call other functions
                                const Value& pval);            // required if contains static data
    // These are shorthand prototypes.
    using Prototype_0  = Value (void);
    using Prototype_1  = Value (cow_vector<Reference>&&);
    using Prototype_2  = Value (cow_vector<Reference>&&, Reference&&);
    using Prototype_3  = Value (cow_vector<Reference>&&, Reference&&, Global&);

  private:
    cow_string m_desc;
    Prototype_4* m_proc;
    Value m_pval;

  public:
    Simple_Binding_Wrapper(const cow_string& desc, Prototype_0* xproc)
      :
        m_desc(desc), m_proc(reinterpret_cast<Prototype_4*>(reinterpret_cast<intptr_t>(xproc))), m_pval()
      {
      }
    Simple_Binding_Wrapper(const cow_string& desc, Prototype_1* xproc)
      :
        m_desc(desc), m_proc(reinterpret_cast<Prototype_4*>(reinterpret_cast<intptr_t>(xproc))), m_pval()
      {
      }
    Simple_Binding_Wrapper(const cow_string& desc, Prototype_2* xproc)
      :
        m_desc(desc), m_proc(reinterpret_cast<Prototype_4*>(reinterpret_cast<intptr_t>(xproc))), m_pval()
      {
      }
    Simple_Binding_Wrapper(const cow_string& desc, Prototype_3* xproc)
      :
        m_desc(desc), m_proc(reinterpret_cast<Prototype_4*>(reinterpret_cast<intptr_t>(xproc))), m_pval()
      {
      }
    template<typename XPvalT> Simple_Binding_Wrapper(const cow_string& desc, Prototype_4* proc, XPvalT&& xpval)
      :
        m_desc(desc), m_proc(proc), m_pval(::rocket::forward<XPvalT>(xpval))
      {
      }
    ~Simple_Binding_Wrapper() override;

  public:
    tinyfmt& describe(tinyfmt& fmt) const override;
    Reference& invoke(Reference& self, Global_Context& global, cow_vector<Reference>&& args) const override;
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const override;
  };

}  // namespace Asteria

#endif
