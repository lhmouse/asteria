// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_AIR_NODE_HPP_
#define ASTERIA_RUNTIME_AIR_NODE_HPP_

#include "../fwd.hpp"
#include "../rocket/preprocessor_utilities.h"
#include "../rocket/variant.hpp"

namespace Asteria {

class Air_Node
  {
  public:
    enum Status : std::uint8_t
      {
        status_next             = 0,
        status_return           = 1,
        status_break_unspec     = 2,
        status_break_switch     = 3,
        status_break_while      = 4,
        status_break_for        = 5,
        status_continue_unspec  = 6,
        status_continue_while   = 7,
        status_continue_for     = 8,
      };
    struct Opaque
      {
        std::intptr_t i;
        PreHashed_String s;
        RefCnt_Ptr<RefCnt_Base> p;
      };
    using Callback = Status (Reference_Stack &stack_io, Executive_Context &ctx_io, const Opaque &opaque, const Cow_String &func, const Global_Context &global);

  private:
    Callback *m_fptr;
    Opaque m_opaque;
    Cow_Vector<Reference> m_refs;

  public:
    Air_Node(Callback *fptr, Opaque opaque, Cow_Vector<Reference> refs)
      : m_fptr(fptr), m_opaque(std::move(opaque)), m_refs(std::move(refs))
      {
      }

  public:
    Status execute(Reference_Stack &stack_io, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global) const;
    void enumerate_variables(const Abstract_Variable_Callback &callback) const;
  };

}

#endif
