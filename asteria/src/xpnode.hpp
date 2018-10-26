// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_XPNODE_HPP_
#define ASTERIA_XPNODE_HPP_

#include "fwd.hpp"
#include "value.hpp"
#include "reference.hpp"
#include "function_header.hpp"
#include "expression.hpp"
#include "block.hpp"
#include "rocket/variant.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

class Xpnode
  {
  public:
    enum Xop : Uint8
      {
        // Postfix operators
        xop_postfix_inc      = 10,  // ++
        xop_postfix_dec      = 11,  // --
        // Prefix operators
        xop_prefix_pos       = 30,  // +
        xop_prefix_neg       = 31,  // -
        xop_prefix_notb      = 32,  // ~
        xop_prefix_notl      = 33,  // !
        xop_prefix_inc       = 34,  // ++
        xop_prefix_dec       = 35,  // --
        xop_prefix_unset     = 36,  // unset
        xop_prefix_lengthof  = 37,  // lengthof
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
        String name;
      };
    struct S_bound_reference
      {
        Reference ref;
      };
    struct S_closure_function
      {
        Function_header head;
        Block body;
      };
    struct S_branch
      {
        bool assign;
        Expression branch_true;
        Expression branch_false;
      };
    struct S_function_call
      {
        Source_location loc;
        Size arg_cnt;
      };
    struct S_subscript
      {
        String name;  // If this is empty then the subscript is to be popped from the stack.
      };
    struct S_operator_rpn
      {
        Xop xop;
        bool assign;  // This parameter is ignored for `++`, `--`, `=` and all relational operators.
      };
    struct S_unnamed_array
      {
        Size elem_cnt;
      };
    struct S_unnamed_object
      {
        Vector<String> keys;
      };
    struct S_coalescence
      {
        bool assign;
        Expression branch_null;
      };

    enum Index : Uint8
      {
        index_literal           =  0,
        index_named_reference   =  1,
        index_bound_reference   =  2,
        index_closure_function  =  3,
        index_branch            =  4,
        index_function_call     =  5,
        index_subscript         =  6,
        index_operator_rpn      =  7,
        index_unnamed_array     =  8,
        index_unnamed_object    =  9,
        index_coalescence       = 10,
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_literal           //  0,
        , S_named_reference   //  1,
        , S_bound_reference   //  2,
        , S_closure_function  //  3,
        , S_branch            //  4,
        , S_function_call     //  5,
        , S_subscript         //  6,
        , S_operator_rpn      //  7,
        , S_unnamed_array     //  8,
        , S_unnamed_object    //  9,
        , S_coalescence       // 10,
      )>;

  public:
    static const char * get_operator_name(Xop xop) noexcept;

  private:
    Variant m_stor;

  public:
    // This constructor does not accept lvalues.
    template<typename AltT, typename std::enable_if<(Variant::index_of<AltT>::value || true)>::type * = nullptr>
      Xpnode(AltT &&alt)
      : m_stor(std::forward<AltT>(alt))
      {
      }
    ROCKET_DECLARE_COPYABLE_DESTRUCTOR(Xpnode);

  public:
    Xpnode bind(const Global_context &global, const Analytic_context &ctx) const;
    void evaluate(Reference_stack &stack_io, Global_context &global, const Executive_context &ctx) const;

    void enumerate_variables(const Abstract_variable_callback &callback) const;
  };

}

#endif
