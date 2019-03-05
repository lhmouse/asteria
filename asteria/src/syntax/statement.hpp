// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_STATEMENT_HPP_
#define ASTERIA_SYNTAX_STATEMENT_HPP_

#include "../fwd.hpp"
#include "source_location.hpp"
#include "../rocket/preprocessor_utilities.h"
#include "../rocket/variant.hpp"

namespace Asteria {

class Statement
  {
  public:
    enum Target : std::uint8_t
      {
        target_unspec  = 0,
        target_switch  = 1,
        target_while   = 2,
        target_for     = 3,
      };

    struct S_expression
      {
        Cow_Vector<Xpnode> expr;
      };
    struct S_block
      {
        Cow_Vector<Statement> body;
      };
    struct S_variable
      {
        Source_Location sloc;
        PreHashed_String name;
        bool immutable;
        Cow_Vector<Xpnode> init;
      };
    struct S_function
      {
        Source_Location sloc;
        PreHashed_String name;
        Cow_Vector<PreHashed_String> params;
        Cow_Vector<Statement> body;
      };
    struct S_if
      {
        bool neg;
        Cow_Vector<Xpnode> cond;
        Cow_Vector<Statement> branch_true;
        Cow_Vector<Statement> branch_false;
      };
    struct S_switch
      {
        Cow_Vector<Xpnode> ctrl;
        Cow_Vector<std::pair<Cow_Vector<Xpnode>,  // This is empty on `default` clauses and non-empty on `case` clauses.
                             Cow_Vector<Statement>>> clauses;
      };
    struct S_do_while
      {
        Cow_Vector<Statement> body;
        bool neg;
        Cow_Vector<Xpnode> cond;
      };
    struct S_while
      {
        bool neg;
        Cow_Vector<Xpnode> cond;
        Cow_Vector<Statement> body;
      };
    struct S_for
      {
        Cow_Vector<Statement> init;
        Cow_Vector<Xpnode> cond;
        Cow_Vector<Xpnode> step;
        Cow_Vector<Statement> body;
      };
    struct S_for_each
      {
        PreHashed_String key_name;
        PreHashed_String mapped_name;
        Cow_Vector<Xpnode> init;
        Cow_Vector<Statement> body;
      };
    struct S_try
      {
        Cow_Vector<Statement> body_try;
        Source_Location sloc;
        PreHashed_String except_name;
        Cow_Vector<Statement> body_catch;
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
        Source_Location sloc;
        Cow_Vector<Xpnode> expr;
      };
    struct S_return
      {
        bool by_ref;
        Cow_Vector<Xpnode> expr;
      };
    struct S_assert
      {
        Cow_Vector<Xpnode> expr;
        Cow_String msg;
      };

    enum Index : std::uint8_t
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
    static_assert(rocket::is_nothrow_copy_constructible<Variant>::value, "???");

  private:
    Variant m_stor;

  public:
    // This constructor does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Variant::index_of<AltT>::value)> Statement(AltT &&alt)
      : m_stor(std::forward<AltT>(alt))
      {
      }
    // This assignment operator does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Variant::index_of<AltT>::value)> Statement & operator=(AltT &&alt)
      {
        this->m_stor = std::forward<AltT>(alt);
        return *this;
      }

  public:
    void generate_code(Cow_Vector<RefCnt_Object<Air_Node>> &code_out, const Analytic_Context &ctx) const;
  };

}

#endif
