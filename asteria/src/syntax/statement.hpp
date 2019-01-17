// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_STATEMENT_HPP_
#define ASTERIA_SYNTAX_STATEMENT_HPP_

#include "../fwd.hpp"
#include "expression.hpp"
#include "source_location.hpp"
#include "block.hpp"
#include "../rocket/preprocessor_utilities.h"
#include "../rocket/variant.hpp"
#include "../rocket/cow_vector.hpp"

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

    struct S_expression
      {
        Expression expr;
      };
    struct S_block
      {
        Block body;
      };
    struct S_variable
      {
        Source_Location loc;
        rocket::prehashed_string name;
        bool immutable;
        Expression init;
      };
    struct S_function
      {
        Source_Location loc;
        rocket::prehashed_string name;
        rocket::cow_vector<rocket::prehashed_string> params;
        Block body;
      };
    struct S_if
      {
        bool neg;
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
        bool neg;
        Expression cond;
      };
    struct S_while
      {
        bool neg;
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
        Source_Location loc;
        Expression expr;
      };
    struct S_return
      {
        bool by_ref;
        Expression expr;
      };
    struct S_assert
      {
        Source_Location loc;
        Expression expr;
        rocket::cow_string msg;
      };

    enum Index : std::size_t
      {
        index_expression  =  0,
        index_block       =  1,
        index_variable    =  2,
        index_function    =  3,
        index_if          =  4,
        index_switch      =  5,
        index_do_while    =  6,
        index_while       =  7,
        index_for         =  8,
        index_for_each    =  9,
        index_try         = 10,
        index_break       = 11,
        index_continue    = 12,
        index_throw       = 13,
        index_return      = 14,
        index_assert      = 15,
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_expression  //  0,
        , S_block       //  1,
        , S_variable    //  2,
        , S_function    //  3,
        , S_if          //  4,
        , S_switch      //  5,
        , S_do_while    //  6,
        , S_while       //  7,
        , S_for         //  8,
        , S_for_each    //  9,
        , S_try         // 10,
        , S_break       // 11,
        , S_continue    // 12,
        , S_throw       // 13,
        , S_return      // 14,
        , S_assert      // 15,
      )>;

  private:
    Variant m_stor;

  public:
    // This constructor does not accept lvalues.
    template<typename AltT,
             ROCKET_ENABLE_IF_HAS_VALUE(Variant::index_of<AltT>::value)>
      Statement(AltT &&alt)
      : m_stor(std::forward<AltT>(alt))
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Statement);

  public:
    void fly_over_in_place(Abstract_Context &ctx_io) const;
    void bind_in_place(rocket::cow_vector<Statement> &stmts_out, Analytic_Context &ctx_io, const Global_Context &global) const;
    void compile(rocket::cow_vector<Block::Compiled_Instruction> &cinsts_out) const;

    void enumerate_variables(const Abstract_Variable_Callback &callback) const;
  };

}

#endif
