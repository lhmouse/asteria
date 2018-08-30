// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_XPNODE_HPP_
#define ASTERIA_XPNODE_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"
#include "value.hpp"
#include "reference.hpp"
#include "expression.hpp"
#include "block.hpp"

namespace Asteria {

class Xpnode
  {
  public:
    enum Xop : std::uint8_t
      {
        // Postfix operators
        xop_postfix_inc    = 10, // ++
        xop_postfix_dec    = 11, // --
        xop_postfix_at     = 12, // []
        // Prefix operators
        xop_prefix_pos     = 30, // +
        xop_prefix_neg     = 31, // -
        xop_prefix_notb    = 32, // ~
        xop_prefix_notl    = 33, // !
        xop_prefix_inc     = 34, // ++
        xop_prefix_dec     = 35, // --
        xop_prefix_unset   = 36, // unset
        // Infix relational operators
        xop_infix_cmp_eq   = 50, // ==
        xop_infix_cmp_ne   = 51, // !=
        xop_infix_cmp_lt   = 52, // <
        xop_infix_cmp_gt   = 53, // >
        xop_infix_cmp_lte  = 54, // <=
        xop_infix_cmp_gte  = 55, // >=
        // Infix general operators
        xop_infix_add      = 60, // +
        xop_infix_sub      = 61, // -
        xop_infix_mul      = 62, // *
        xop_infix_div      = 63, // /
        xop_infix_mod      = 64, // %
        xop_infix_sll      = 65, // <<<
        xop_infix_srl      = 66, // >>>
        xop_infix_sla      = 67, // <<
        xop_infix_sra      = 68, // >>
        xop_infix_andb     = 69, // &
        xop_infix_orb      = 70, // |
        xop_infix_xorb     = 71, // ^
        xop_infix_assign   = 72, // =
      };

    enum Index : std::uint8_t
      {
        index_literal           =  0,
        index_named_reference   =  1,
        index_bound_reference   =  2,
        index_subexpression     =  3,
        index_closure_function  =  4,
        index_branch            =  5,
        index_function_call     =  6,
        index_operator_rpn      =  7,
        index_unnamed_array     =  8,
        index_unnamed_object    =  9,
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
    struct S_subexpression
      {
        Expression expr;
      };
    struct S_closure_function
      {
        Vector<String> params;
        String file;
        Unsigned line;
        Block body;
      };
    struct S_branch
      {
        Expression branch_true;
        Expression branch_false;
      };
    struct S_function_call
      {
        String file;
        Unsigned line;
        std::size_t arg_cnt;
      };
    struct S_operator_rpn
      {
        Xop xop;
        bool compound_assign;  // This parameter is ignored for `++`, `--`, `[]`, `=` and all relational operators.
      };
    struct S_unnamed_array
      {
        Vector<Expression> elems;
      };
    struct S_unnamed_object
      {
        Dictionary<Expression> pairs;
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_literal           //  0,
        , S_named_reference   //  1,
        , S_bound_reference   //  2,
        , S_subexpression     //  3,
        , S_closure_function  //  4,
        , S_branch            //  5,
        , S_function_call     //  6,
        , S_operator_rpn      //  7,
        , S_unnamed_array     //  8,
        , S_unnamed_object    //  9,
      )>;

  public:
    static const char * get_operator_name(Xop xop) noexcept;

  private:
    Variant m_stor;

  public:
    template<typename AltT, typename std::enable_if<std::is_constructible<Variant, AltT &&>::value>::type * = nullptr>
      Xpnode(AltT &&alt)
        : m_stor(std::forward<AltT>(alt))
        {
        }
    ~Xpnode();

    Xpnode(Xpnode &&) noexcept;
    Xpnode & operator=(Xpnode &&) noexcept;

  public:
    Xpnode bind(const Analytic_context &ctx) const;
    void evaluate(Vector<Reference> &stack_inout, const Executive_context &ctx) const;
  };

}

#endif
