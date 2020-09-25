// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_EXPRESSION_UNIT_HPP_
#define ASTERIA_COMPILER_EXPRESSION_UNIT_HPP_

#include "../fwd.hpp"
#include "../value.hpp"
#include "../source_location.hpp"

namespace asteria {

class Expression_Unit
  {
  public:
    struct S_literal
      {
        Value value;
      };

    struct S_named_reference
      {
        Source_Location sloc;
        phsh_string name;
      };

    struct S_closure_function
      {
        Source_Location sloc;
        cow_string unique_name;
        cow_vector<phsh_string> params;
        cow_vector<Statement> body;
      };

    struct S_branch
      {
        Source_Location sloc;
        cow_vector<Expression_Unit> branch_true;
        cow_vector<Expression_Unit> branch_false;
        bool assign;
      };

    struct S_function_call
      {
        Source_Location sloc;
        uint32_t nargs;
      };

    struct S_member_access
      {
        Source_Location sloc;
        phsh_string name;
      };

    struct S_operator_rpn
      {
        Source_Location sloc;
        Xop xop;
        bool assign;  // ignored for `++`, `--`, `[]` and `=`
      };

    struct S_unnamed_array
      {
        Source_Location sloc;
        uint32_t nelems;
      };

    struct S_unnamed_object
      {
        Source_Location sloc;
        cow_vector<phsh_string> keys;
      };

    struct S_coalescence
      {
        Source_Location sloc;
        cow_vector<Expression_Unit> branch_null;
        bool assign;
      };

    struct S_global_reference
      {
        Source_Location sloc;
        phsh_string name;
      };

    struct S_variadic_call
      {
        Source_Location sloc;
      };

    struct S_argument_finish
      {
        Source_Location sloc;
        bool by_ref;
      };

    struct S_import_call
      {
        Source_Location sloc;
        uint32_t nargs;
      };

    enum Index : uint8_t
      {
        index_literal           =  0,
        index_named_reference   =  1,
        index_closure_function  =  2,
        index_branch            =  3,
        index_function_call     =  4,
        index_member_access     =  5,
        index_operator_rpn      =  6,
        index_unnamed_array     =  7,
        index_unnamed_object    =  8,
        index_coalescence       =  9,
        index_global_reference  = 10,
        index_variadic_call     = 11,
        index_argument_finish   = 12,
        index_import_call       = 13,
      };

  private:
    using Storage = ::rocket::variant<
      ROCKET_CDR(
        ,S_literal           //  0,
        ,S_named_reference   //  1,
        ,S_closure_function  //  2,
        ,S_branch            //  3,
        ,S_function_call     //  4,
        ,S_member_access     //  5,
        ,S_operator_rpn      //  6,
        ,S_unnamed_array     //  7,
        ,S_unnamed_object    //  8,
        ,S_coalescence       //  9,
        ,S_global_reference  // 10,
        ,S_variadic_call     // 11,
        ,S_argument_finish   // 12,
        ,S_import_call       // 13,
      )>;

    Storage m_stor;

  public:
    ASTERIA_VARIANT_CONSTRUCTOR(Expression_Unit, Storage, XUnitT, xunit)
      : m_stor(::std::forward<XUnitT>(xunit))
      { }

    ASTERIA_VARIANT_ASSIGNMENT(Expression_Unit, Storage, XUnitT, xunit)
      { this->m_stor = ::std::forward<XUnitT>(xunit);
        return *this;  }

  public:
    Index
    index()
    const noexcept
      { return static_cast<Index>(this->m_stor.index());  }

    Expression_Unit&
    swap(Expression_Unit& other)
    noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    cow_vector<AIR_Node>&
    generate_code(cow_vector<AIR_Node>& code, const Compiler_Options& opts,
                  const Analytic_Context& ctx, PTC_Aware ptc)
    const;
  };

inline
void
swap(Expression_Unit& lhs, Expression_Unit& rhs)
noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
