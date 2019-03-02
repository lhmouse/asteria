// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_AIR_NODE_HPP_
#define ASTERIA_RUNTIME_AIR_NODE_HPP_

#include "../fwd.hpp"
#include "value.hpp"
#include "reference.hpp"
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

    enum Index : std::uint8_t
      {
        index_string     = 0,
        index_value      = 1,
        index_reference  = 2,
        index_nodes      = 3,
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , PreHashed_String      // 0,
        , Value                 // 1,
        , Reference             // 2,
        , Cow_Vector<Air_Node>  // 3,
      )>;
    static_assert(rocket::is_nothrow_copy_constructible<Variant>::value, "???");

    struct Opaque
      {
        std::intptr_t ione;
        std::intptr_t itwo;
        Cow_Vector<Variant> vars;
      };
    using Executor = Status (Reference_Stack &stack_io, Executive_Context &ctx_io, const Opaque &opaque, const Cow_String &func, const Global_Context &global);

  private:
    Executor *m_fptr;
    Opaque m_opaque;

  public:
    Air_Node(Executor *fptr, Opaque opaque)
      : m_fptr(fptr), m_opaque(std::move(opaque))
      {
      }

  public:
    Status execute(Reference_Stack &stack_io, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global) const;
    void enumerate_variables(const Abstract_Variable_Callback &callback) const;
  };

}

#endif
