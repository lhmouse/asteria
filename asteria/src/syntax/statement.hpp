// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_STATEMENT_HPP_
#define ASTERIA_SYNTAX_STATEMENT_HPP_

#include "../fwd.hpp"
#include "expression.hpp"
#include "source_location.hpp"
#include "block.hpp"
#include "../rocket/variant.hpp"
#include "../rocket/refcounted_ptr.hpp"

namespace Asteria {

class Statement
  {
  public:
    enum Target : std::size_t
      {
        target_unspec  = 0,
        target_switch  = 1,
        target_while   = 2,
        target_for     = 3,
      };

    struct S_expr
      {
        Expression expr;
      };
    struct S_block
      {
        Block body;
      };
    struct S_var_def
      {
        Source_location loc;
        rocket::prehashed_string name;
        bool immutable;
        Expression init;
      };
    struct S_func_def
      {
        Source_location loc;
        rocket::prehashed_string name;
        rocket::cow_vector<rocket::prehashed_string> params;
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
        rocket::cow_vector<std::pair<Expression, Block>> clauses;
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
        Block init;
        Expression cond;
        Expression step;
        Block body;
      };
    struct S_for_each
      {
        rocket::prehashed_string key_name;
        rocket::prehashed_string mapped_name;
        Expression init;
        Block body;
      };
    struct S_try
      {
        Block body_try;
        rocket::prehashed_string except_name;
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
        Source_location loc;
        Expression expr;
      };
    struct S_return
      {
        bool by_ref;
        Expression expr;
      };

    enum Index : std::size_t
      {
        index_expr      =  0,
        index_block     =  1,
        index_var_def   =  2,
        index_func_def  =  3,
        index_if        =  4,
        index_switch    =  5,
        index_do_while  =  6,
        index_while     =  7,
        index_for       =  8,
        index_for_each  =  9,
        index_try       = 10,
        index_break     = 11,
        index_continue  = 12,
        index_throw     = 13,
        index_return    = 14,
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_expr      //  0,
        , S_block     //  1,
        , S_var_def   //  2,
        , S_func_def  //  3,
        , S_if        //  4,
        , S_switch    //  5,
        , S_do_while  //  6,
        , S_while     //  7,
        , S_for       //  8,
        , S_for_each  //  9,
        , S_try       // 10,
        , S_break     // 11,
        , S_continue  // 12,
        , S_throw     // 13,
        , S_return    // 14,
      )>;

  private:
    Variant m_stor;

  public:
    // This constructor does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Variant::index_of<AltT>::value)>
      Statement(AltT &&alt)
      : m_stor(std::forward<AltT>(alt))
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Statement);

  public:
    void fly_over_in_place(Abstract_context &ctx_io) const;
    Statement bind_in_place(Analytic_context &ctx_io, const Global_context &global) const;
    Block::Status execute_in_place(Reference &ref_out, Executive_context &ctx_io, Global_context &global) const;

    void enumerate_variables(const Abstract_variable_callback &callback) const;
  };

}

#endif
