// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_AIR_NODE_HPP_
#define ASTERIA_RUNTIME_AIR_NODE_HPP_

#include "../fwd.hpp"
#include "value.hpp"
#include "reference.hpp"
#include "../syntax/source_location.hpp"
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
        index_int64             = 0,
        index_string            = 1,
        index_vector_string     = 2,
        index_source_location   = 3,
        index_vector_statement  = 4,
        index_value             = 5,
        index_reference         = 6,
        index_vector_air_node   = 7,
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , std::int64_t                  // 0,
        , PreHashed_String              // 1,
        , Cow_Vector<PreHashed_String>  // 2,
        , Source_Location               // 3,
        , Cow_Vector<Statement>         // 4,
        , Value                         // 5,
        , Reference                     // 6,
        , Cow_Vector<Air_Node>          // 7,
      )>;
    using Executor = Status (Reference_Stack &stack_io, Executive_Context &ctx_io, const Cow_Vector<Variant> &params, const Cow_String &func, const Global_Context &global);

  private:
    Executor *m_fptr;
    Cow_Vector<Variant> m_params;

  public:
    Air_Node(Executor *fptr, Cow_Vector<Variant> &&params)
      : m_fptr(fptr), m_params(std::move(params))
      {
      }

  public:
    Status execute(Reference_Stack &stack_io, Executive_Context &ctx_io, const Cow_String &func, const Global_Context &global) const
      {
        return (*(this->m_fptr))(stack_io, ctx_io, this->m_params, func, global);
      }
    void enumerate_variables(const Abstract_Variable_Callback &callback) const;
  };

}

#endif
