// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STATEMENT_HPP_
#define ASTERIA_STATEMENT_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"
#include "expression.hpp"
#include "block.hpp"

namespace Asteria {

class Statement
  {
  public:
    enum Target : std::uint8_t
      {
        target_scope_unspec  = 0,
        target_scope_switch  = 1,
        target_scope_while   = 2,
        target_scope_for     = 3,
      };

    enum Index : std::uint8_t
      {
        index_expression  =  0,
        index_var_def     =  1,
        index_func_def    =  2,
        index_if          =  3,
        index_switch      =  4,
        index_do_while    =  5,
        index_while       =  6,
        index_for         =  7,
        index_for_each    =  8,
        index_try         =  9,
        index_break       = 10,
        index_continue    = 11,
        index_throw       = 12,
        index_return      = 13,
        index_export      = 14,
        index_import      = 15,
      };
    struct S_expression
      {
        Expression expr;
      };
    struct S_var_def
      {
        String name;
        bool immutable;
        Expression init;
      };
    struct S_func_def
      {
        String name;
        Vector<String> params;
        String file;
        Unsigned line;
        Block body;
      };
    struct S_if
      {
        Expression cond;
        Block branch_true;
        Block branch_false;
      };
    struct S_switch
      {
        Expression ctrl;
        Bivector<Expression, Block> clauses;
      };
    struct S_do_while
      {
        Block body;
        Expression cond;
      };
    struct S_while
      {
        Expression cond;
        Block body;
      };
    struct S_for
      {
        String var_name;
        bool var_immutable;
        Expression var_init;
        Expression cond;
        Expression step;
        Block body;
      };
    struct S_for_each
      {
        String key_name;
        String mapped_name;
        Expression range_init;
        Block body;
      };
    struct S_try
      {
        Block body_try;
        String except_name;
        Block body_catch;
      };
    struct S_break
      {
        Target target;
      };
    struct S_continue
      {
        Target target;
      };
    struct S_throw
      {
        Expression expr;
      };
    struct S_return
      {
        Expression expr;
      };
    struct S_export
      {
        String name;
      };
    struct S_import
      {
        String path;
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_expression  //  0,
        , S_var_def     //  1,
        , S_func_def    //  2,
        , S_if          //  3,
        , S_switch      //  4,
        , S_do_while    //  5,
        , S_while       //  6,
        , S_for         //  7,
        , S_for_each    //  8,
        , S_try         //  9,
        , S_break       // 10,
        , S_continue    // 11,
        , S_throw       // 12,
        , S_return      // 13,
        , S_export      // 14,
        , S_import      // 15,
      )>;

  private:
    Variant m_stor;

  public:
    template<typename AltT, typename std::enable_if<std::is_constructible<Variant, AltT &&>::value>::type * = nullptr>
      Statement(AltT &&alt)
        : m_stor(std::forward<AltT>(alt))
        {
        }
    ~Statement();

    Statement(Statement &&) noexcept;
    Statement & operator=(Statement &&) noexcept;

  public:
    void fly_over_in_place(Abstract_context &ctx_inout) const;
    Statement bind_in_place(Analytic_context &ctx_inout) const;
    Block::Status execute_in_place(Reference &ref_out, Executive_context &ctx_inout) const;
  };

}

#endif
