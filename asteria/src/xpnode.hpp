// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_XPNODE_HPP_
#define ASTERIA_XPNODE_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"
#include "value.hpp"
#include "reference.hpp"

namespace Asteria
{

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
        Vector<Xpnode> expr;
      };
    struct S_closure_function
      {
        String src_file;
        Unsigned src_line;
        Vector<String> params;
        // TODO Vector<Statement> body;
      };
    struct S_branch
      {
        Vector<Xpnode> branch_true;
        Vector<Xpnode> branch_false;
      };
    struct S_function_call
      {
        std::size_t arg_cnt;
      };
    struct S_operator_rpn
      {
        Xop xop;
        bool compound_assign;  // This parameter is ignored for `++`, `--`, `[]`, `=` and all relational operators.
      };
    struct S_unnamed_array
      {
        Vector<Vector<Xpnode>> elems;
      };
    struct S_unnamed_object
      {
        Dictionary<Vector<Xpnode>> pairs;
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

  private:
    Variant m_variant;

  public:
    template<typename CandidateT, typename std::enable_if<std::is_constructible<Variant, CandidateT &&>::value>::type * = nullptr>
      Xpnode(CandidateT &&cand)
        : m_variant(std::forward<CandidateT>(cand))
        {
        }
    Xpnode(Xpnode &&) noexcept;
    Xpnode & operator=(Xpnode &&) noexcept;
    ~Xpnode();

  public:
    Index which() const noexcept
      {
        return static_cast<Index>(m_variant.index());
      }
    template<typename ExpectT>
      const ExpectT * opt() const noexcept
        {
          return m_variant.get<ExpectT>();
        }
    template<typename ExpectT>
      const ExpectT & as() const
        {
          return m_variant.as<ExpectT>();
        }
  };

extern const char * get_operator_name(Xpnode::Xop xop) noexcept;

extern Xpnode bind_xpnode_partial(const Xpnode &node, Spref<Context> ctx);
extern void evaluate_xpnode_partial(Vector<Reference> &stack_inout, const Xpnode &node, Spref<Context> ctx);

extern Vector<Xpnode> bind_expression(const Vector<Xpnode> &expr, Spref<Context> ctx);
extern Reference evaluate_expression(const Vector<Xpnode> &expr, Spref<Context> ctx);

}

#endif
