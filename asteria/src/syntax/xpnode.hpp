// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_XPNODE_HPP_
#define ASTERIA_SYNTAX_XPNODE_HPP_

#include "../fwd.hpp"
#include "source_location.hpp"
#include "expression.hpp"
#include "block.hpp"
#include "../runtime/value.hpp"
#include "../runtime/reference.hpp"

namespace Asteria {

class Xpnode
  {
  public:
    enum Xop : std::size_t
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
        rocket::prehashed_string name;
      };
    struct S_bound_reference
      {
        Reference ref;
      };
    struct S_closure_function
      {
        Source_location loc;
        rocket::cow_vector<rocket::prehashed_string> params;
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
        Source_location loc;
        std::size_t arg_cnt;
      };
    struct S_subscript
      {
        rocket::prehashed_string name;  // If this is empty then the subscript is to be popped from the stack.
      };
    struct S_operator_rpn
      {
        Xop xop;
        bool assign;  // This parameter is ignored for `++`, `--` and `=`.
      };
    struct S_unnamed_array
      {
        std::size_t elem_cnt;
      };
    struct S_unnamed_object
      {
        rocket::cow_vector<rocket::prehashed_string> keys;
      };
    struct S_coalescence
      {
        Expression branch_null;
        bool assign;
      };

    enum Index : std::size_t
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
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Variant::index_of<AltT>::value)>
      Xpnode(AltT &&alt)
      : m_stor(std::forward<AltT>(alt))
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Xpnode);

  public:
    Xpnode bind(const Global_context &global, const Analytic_context &ctx) const;
    rocket::binder_first<void (*)(const void *, Reference_stack &, Global_context &, const Executive_context &), const void *> compile() const;

    void enumerate_variables(const Abstract_variable_callback &callback) const;
  };

}

#endif
