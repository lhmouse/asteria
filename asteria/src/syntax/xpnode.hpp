// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_XPNODE_HPP_
#define ASTERIA_SYNTAX_XPNODE_HPP_

#include "../fwd.hpp"
#include "source_location.hpp"
#include "expression.hpp"
#include "block.hpp"
#include "../runtime/value.hpp"
#include "../runtime/reference.hpp"
#include "../rocket/preprocessor_utilities.h"
#include "../rocket/variant.hpp"

namespace Asteria {

class Xpnode
  {
  public:
    enum Xop : std::uint8_t
      {
        // Postfix operators
        xop_postfix_inc      = 10,  // ++
        xop_postfix_dec      = 11,  // --
        xop_postfix_at       = 12,  // []
        // Prefix operators
        xop_prefix_pos       = 30,  // +
        xop_prefix_neg       = 31,  // -
        xop_prefix_notb      = 32,  // ~
        xop_prefix_notl      = 33,  // !
        xop_prefix_inc       = 34,  // ++
        xop_prefix_dec       = 35,  // --
        xop_prefix_unset     = 36,  // unset
        xop_prefix_lengthof  = 37,  // lengthof
        xop_prefix_typeof    = 38,  // typeof
        // Infix relational operators
        xop_infix_cmp_eq     = 50,  // ==
        xop_infix_cmp_ne     = 51,  // !=
        xop_infix_cmp_lt     = 52,  // <
        xop_infix_cmp_gt     = 53,  // >
        xop_infix_cmp_lte    = 54,  // <=
        xop_infix_cmp_gte    = 55,  // >=
        xop_infix_cmp_3way   = 56,  // <=>
        // Infix general operators
        xop_infix_add        = 60,  // +
        xop_infix_sub        = 61,  // -
        xop_infix_mul        = 62,  // *
        xop_infix_div        = 63,  // /
        xop_infix_mod        = 64,  // %
        xop_infix_sll        = 65,  // <<<
        xop_infix_srl        = 66,  // >>>
        xop_infix_sla        = 67,  // <<
        xop_infix_sra        = 68,  // >>
        xop_infix_andb       = 69,  // &
        xop_infix_orb        = 70,  // |
        xop_infix_xorb       = 71,  // ^
        xop_infix_assign     = 72,  // =
      };

    struct S_literal
      {
        Value value;
      };
    struct S_named_reference
      {
        PreHashed_String name;
      };
    struct S_closure_function
      {
        Source_Location sloc;
        Cow_Vector<PreHashed_String> params;
        Block body;
      };
    struct S_branch
      {
        Expression branch_true;
        Expression branch_false;
        bool assign;
      };
    struct S_function_call
      {
        Source_Location sloc;
        std::size_t nargs;
      };
    struct S_member_access
      {
        PreHashed_String name;
      };
    struct S_operator_rpn
      {
        Xop xop;
        bool assign;  // This parameter is ignored for `++`, `--`, `[]` and `=`.
      };
    struct S_unnamed_array
      {
        std::size_t nelems;
      };
    struct S_unnamed_object
      {
        Cow_Vector<PreHashed_String> keys;
      };
    struct S_coalescence
      {
        Expression branch_null;
        bool assign;
      };
    struct S_bound_reference
      {
        Reference ref;
      };

    enum Index : std::uint8_t
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
        index_bound_reference   = 10,
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_literal           //  0,
        , S_named_reference   //  1,
        , S_closure_function  //  2,
        , S_branch            //  3,
        , S_function_call     //  4,
        , S_member_access     //  5,
        , S_operator_rpn      //  6,
        , S_unnamed_array     //  7,
        , S_unnamed_object    //  8,
        , S_coalescence       //  9,
        , S_bound_reference   // 10,
      )>;
    static_assert(rocket::is_nothrow_copy_constructible<Variant>::value, "???");

  public:
    ROCKET_PURE_FUNCTION static const char * get_operator_name(Xop xop) noexcept;

  private:
    Variant m_stor;

  public:
    // This constructor does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Variant::index_of<AltT>::value)> Xpnode(AltT &&alt)
      : m_stor(std::forward<AltT>(alt))
      {
      }
    // This assignment operator does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Variant::index_of<AltT>::value)> Xpnode & operator=(AltT &&alt)
      {
        this->m_stor = std::forward<AltT>(alt);
        return *this;
      }

  public:
    void bind(Cow_Vector<Xpnode> &nodes_out, const Global_Context &global, const Analytic_Context &ctx) const;
    void compile(Cow_Vector<Expression::Compiled_Instruction> &cinsts_out) const;

    void enumerate_variables(const Abstract_Variable_Callback &callback) const;
  };

}

#endif
