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
    enum Target : Uint8
      {
        target_unspec  = 0,
        target_switch  = 1,
        target_while   = 2,
        target_for     = 3,
      };

    struct S_export
      {
        String name;
      };
    struct S_import
      {
        String path;
      };
    struct S_block
      {
        Block body;
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
        Uint64 line;
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
    struct S_while
      {
        bool has_do;
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
    struct S_expr
      {
        Expression expr;
      };

    enum Index : Uint8
      {
        index_export    =  0,
        index_import    =  1,
        index_block     =  2,
        index_var_def   =  3,
        index_func_def  =  4,
        index_if        =  5,
        index_switch    =  6,
        index_while     =  7,
        index_for       =  8,
        index_for_each  =  9,
        index_try       = 10,
        index_break     = 11,
        index_continue  = 12,
        index_throw     = 13,
        index_return    = 14,
        index_expr      = 15,
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_export    //  0,
        , S_import    //  1,
        , S_block     //  2,
        , S_var_def   //  3,
        , S_func_def  //  4,
        , S_if        //  5,
        , S_switch    //  6,
        , S_while     //  7,
        , S_for       //  8,
        , S_for_each  //  9,
        , S_try       // 10,
        , S_break     // 11,
        , S_continue  // 12,
        , S_throw     // 13,
        , S_return    // 14,
        , S_expr      // 15,
      )>;

  private:
    Variant m_stor;

  public:
    Statement() noexcept
      : m_stor()
      {
      }
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
