// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_XPRUNIT_HPP_
#define ASTERIA_SYNTAX_XPRUNIT_HPP_

#include "../fwd.hpp"
#include "source_location.hpp"
#include "../runtime/value.hpp"
#include "../runtime/reference.hpp"

namespace Asteria {

class Xprunit
  {
  public:
    enum Xop : std::uint8_t
      {
        // Postfix operators
        xop_postfix_inc      = 10,  // ++
        xop_postfix_dec      = 11,  // --
        xop_postfix_at       = 12,  // []
        // Prefix operators
        xop_prefix_pos       = 20,  // +
        xop_prefix_neg       = 21,  // -
        xop_prefix_notb      = 22,  // ~
        xop_prefix_notl      = 23,  // !
        xop_prefix_inc       = 24,  // ++
        xop_prefix_dec       = 25,  // --
        xop_prefix_unset     = 26,  // unset
        xop_prefix_lengthof  = 27,  // lengthof
        xop_prefix_typeof    = 28,  // typeof
        xop_prefix_sqrt      = 29,  // __sqrt
        xop_prefix_isnan     = 30,  // __isnan
        xop_prefix_isinf     = 31,  // __isinf
        xop_prefix_abs       = 32,  // __abs
        xop_prefix_round     = 33,  // __round
        xop_prefix_floor     = 34,  // __floor
        xop_prefix_ceil      = 35,  // __ceil
        xop_prefix_trunc     = 36,  // __trunc
        xop_prefix_iround    = 37,  // __iround
        xop_prefix_ifloor    = 38,  // __ifloor
        xop_prefix_iceil     = 39,  // __iceil
        xop_prefix_itrunc    = 40,  // __itrunc
        // Infix relational operators
        xop_infix_cmp_eq     = 70,  // ==
        xop_infix_cmp_ne     = 71,  // !=
        xop_infix_cmp_lt     = 72,  // <
        xop_infix_cmp_gt     = 73,  // >
        xop_infix_cmp_lte    = 74,  // <=
        xop_infix_cmp_gte    = 75,  // >=
        xop_infix_cmp_3way   = 76,  // <=>
        // Infix general operators
        xop_infix_add        = 80,  // +
        xop_infix_sub        = 81,  // -
        xop_infix_mul        = 82,  // *
        xop_infix_div        = 83,  // /
        xop_infix_mod        = 84,  // %
        xop_infix_sll        = 85,  // <<<
        xop_infix_srl        = 86,  // >>>
        xop_infix_sla        = 87,  // <<
        xop_infix_sra        = 88,  // >>
        xop_infix_andb       = 89,  // &
        xop_infix_orb        = 90,  // |
        xop_infix_xorb       = 91,  // ^
        xop_infix_assign     = 92,  // =
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
        Cow_Vector<Statement> body;
      };
    struct S_branch
      {
        Cow_Vector<Xprunit> branch_true;
        Cow_Vector<Xprunit> branch_false;
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
        Cow_Vector<Xprunit> branch_null;
        bool assign;
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
      };
    using Xvariant = Variant<
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
      )>;
    static_assert(std::is_nothrow_copy_assignable<Xvariant>::value, "???");

  public:
    ROCKET_PURE_FUNCTION static const char* get_operator_name(Xop xop) noexcept;

  private:
    Xvariant m_stor;

  public:
    // This constructor does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Xvariant::index_of<AltT>::value)> Xprunit(AltT&& altr)
      : m_stor(rocket::forward<AltT>(altr))
      {
      }
    // This assignment operator does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Xvariant::index_of<AltT>::value)> Xprunit& operator=(AltT&& altr)
      {
        this->m_stor = rocket::forward<AltT>(altr);
        return *this;
      }

  public:
    void generate_code(Cow_Vector<Air_Node>& code, const Analytic_Context& ctx) const;
  };

}  // namespace Asteria

#endif
