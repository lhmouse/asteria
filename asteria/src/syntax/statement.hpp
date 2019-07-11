// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_STATEMENT_HPP_
#define ASTERIA_SYNTAX_STATEMENT_HPP_

#include "../fwd.hpp"
#include "source_location.hpp"

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
        Cow_Vector<Xprunit> expr;
      };
    struct S_block
      {
        Cow_Vector<Statement> body;
      };
    struct S_variable
      {
        Source_Location sloc;
        bool immutable;
        Cow_Vector<Pair<PreHashed_String,  // name
                        Cow_Vector<Xprunit>  // initializer
                   >> vars;
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
        bool negative;
        Cow_Vector<Xprunit> cond;
        Cow_Vector<Statement> branch_true;
        Cow_Vector<Statement> branch_false;
      };
    struct S_switch
      {
        Cow_Vector<Xprunit> ctrl;
        Cow_Vector<Pair<Cow_Vector<Xprunit>,  // condition (empty for `default` and non-empty for `case`)
                        Cow_Vector<Statement>  // body
                   >> clauses;
      };
    struct S_do_while
      {
        Cow_Vector<Statement> body;
        bool negative;
        Cow_Vector<Xprunit> cond;
      };
    struct S_while
      {
        bool negative;
        Cow_Vector<Xprunit> cond;
        Cow_Vector<Statement> body;
      };
    struct S_for_each
      {
        PreHashed_String key_name;
        PreHashed_String mapped_name;
        Cow_Vector<Xprunit> init;
        Cow_Vector<Statement> body;
      };
    struct S_for
      {
        Cow_Vector<Statement> init;
        Cow_Vector<Xprunit> cond;
        Cow_Vector<Xprunit> step;
        Cow_Vector<Statement> body;
      };
    struct S_try
      {
        Cow_Vector<Statement> body_try;
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
        Cow_Vector<Xprunit> expr;
      };
    struct S_return
      {
        Cow_Vector<Xprunit> expr;
      };
    struct S_assert
      {
        Source_Location sloc;
        bool negative;
        Cow_Vector<Xprunit> expr;
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
        index_for_each    =  8,
        index_for         =  9,
        index_try         = 10,
        index_break       = 11,
        index_continue    = 12,
        index_throw       = 13,
        index_return      = 14,
        index_assert      = 15,
      };
    using Xvariant = Variant<
      ROCKET_CDR(
        , S_expression  //  0,
        , S_block       //  1,
        , S_variable    //  2,
        , S_function    //  3,
        , S_if          //  4,
        , S_switch      //  5,
        , S_do_while    //  6,
        , S_while       //  7,
        , S_for_each    //  8,
        , S_for         //  9,
        , S_try         // 10,
        , S_break       // 11,
        , S_continue    // 12,
        , S_throw       // 13,
        , S_return      // 14,
        , S_assert      // 15,
      )>;
    static_assert(std::is_nothrow_copy_assignable<Xvariant>::value, "???");

  private:
    Xvariant m_stor;

  public:
    // This constructor does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Xvariant::index_of<AltT>::value)> Statement(AltT&& altr)
      : m_stor(rocket::forward<AltT>(altr))
      {
      }
    // This assignment operator does not accept lvalues.
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Xvariant::index_of<AltT>::value)> Statement& operator=(AltT&& altr)
      {
        this->m_stor = rocket::forward<AltT>(altr);
        return *this;
      }

  public:
    void generate_code(Cow_Vector<Air_Node>& code, Cow_Vector<PreHashed_String>* names_opt, Analytic_Context& ctx) const;
  };

}  // namespace Asteria

#endif
