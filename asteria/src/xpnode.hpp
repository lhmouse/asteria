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
    enum Xop : Uint8
      {
        // Postfix operators
        xop_postfix_inc    = 10,  // ++
        xop_postfix_dec    = 11,  // --
        xop_postfix_at     = 12,  // []
        // Prefix operators
        xop_prefix_pos     = 30,  // +
        xop_prefix_neg     = 31,  // -
        xop_prefix_notb    = 32,  // ~
        xop_prefix_notl    = 33,  // !
        xop_prefix_inc     = 34,  // ++
        xop_prefix_dec     = 35,  // --
        xop_prefix_unset   = 36,  // unset
        // Infix relational operators
        xop_infix_cmp_eq   = 50,  // ==
        xop_infix_cmp_ne   = 51,  // !=
        xop_infix_cmp_lt   = 52,  // <
        xop_infix_cmp_gt   = 53,  // >
        xop_infix_cmp_lte  = 54,  // <=
        xop_infix_cmp_gte  = 55,  // >=
        // Infix general operators
        xop_infix_add      = 60,  // +
        xop_infix_sub      = 61,  // -
        xop_infix_mul      = 62,  // *
        xop_infix_div      = 63,  // /
        xop_infix_mod      = 64,  // %
        xop_infix_sll      = 65,  // <<<
        xop_infix_srl      = 66,  // >>>
        xop_infix_sla      = 67,  // <<
        xop_infix_sra      = 68,  // >>
        xop_infix_andb     = 69,  // &
        xop_infix_orb      = 70,  // |
        xop_infix_xorb     = 71,  // ^
        xop_infix_assign   = 72,  // =
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
        String file;
        Uint64 line;
        Vector<String> params;
        Block body;
      };
    struct S_branch
      {
        Expression branch_true;
        Expression branch_false;
        bool compound_assign;
      };
    struct S_function_call
      {
        String file;
        Uint64 line;
        Size arg_cnt;
      };
    struct S_operator_rpn
      {
        Xop xop;
        bool compound_assign;  // This parameter is ignored for `++`, `--`, `[]`, `=` and all relational operators.
      };
    struct S_unnamed_array
      {
        Size elem_cnt;
      };
    struct S_unnamed_object
      {
        Vector<String> keys;
      };

    enum Index : Uint8
      {
        index_literal           = 0,
        index_named_reference   = 1,
        index_bound_reference   = 2,
        index_closure_function  = 3,
        index_branch            = 4,
        index_function_call     = 5,
        index_operator_rpn      = 6,
        index_unnamed_array     = 7,
        index_unnamed_object    = 8,
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_literal           // 0,
        , S_named_reference   // 1,
        , S_bound_reference   // 2,
        , S_closure_function  // 3,
        , S_branch            // 4,
        , S_function_call     // 5,
        , S_operator_rpn      // 6,
        , S_unnamed_array     // 7,
        , S_unnamed_object    // 8,
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
    ~Xpnode();

  public:
    Xpnode bind(const Analytic_context &ctx) const;
    void evaluate(Vector<Reference> &stack_io, const Executive_context &ctx) const;
  };

}

#endif
