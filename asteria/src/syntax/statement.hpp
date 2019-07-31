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
    enum Target : uint8_t
      {
        target_unspec  = 0,
        target_switch  = 1,
        target_while   = 2,
        target_for     = 3,
      };

    struct S_expression
      {
        cow_vector<Xprunit> expr;
      };
    struct S_block
      {
        cow_vector<Statement> body;
      };
    struct S_variable
      {
        Source_Location sloc;
        bool immutable;
        cow_bivector<phsh_string, cow_vector<Xprunit>> vars;
      };
    struct S_function
      {
        Source_Location sloc;
        phsh_string name;
        cow_vector<phsh_string> params;
        cow_vector<Statement> body;
      };
    struct S_if
      {
        bool negative;
        cow_vector<Xprunit> cond;
        cow_vector<Statement> branch_true;
        cow_vector<Statement> branch_false;
      };
    struct S_switch
      {
        cow_vector<Xprunit> ctrl;
        cow_bivector<cow_vector<Xprunit>, cow_vector<Statement>> clauses;
      };
    struct S_do_while
      {
        cow_vector<Statement> body;
        bool negative;
        cow_vector<Xprunit> cond;
      };
    struct S_while
      {
        bool negative;
        cow_vector<Xprunit> cond;
        cow_vector<Statement> body;
      };
    struct S_for_each
      {
        phsh_string key_name;
        phsh_string mapped_name;
        cow_vector<Xprunit> init;
        cow_vector<Statement> body;
      };
    struct S_for
      {
        cow_vector<Statement> init;
        cow_vector<Xprunit> cond;
        cow_vector<Xprunit> step;
        cow_vector<Statement> body;
      };
    struct S_try
      {
        cow_vector<Statement> body_try;
        Source_Location sloc;
        phsh_string except_name;
        cow_vector<Statement> body_catch;
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
        cow_vector<Xprunit> expr;
      };
    struct S_return
      {
        bool by_ref;
        cow_vector<Xprunit> expr;
      };
    struct S_assert
      {
        Source_Location sloc;
        bool negative;
        cow_vector<Xprunit> expr;
        cow_string msg;
      };

    enum Index : uint8_t
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
    using Xvariant = variant<
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
    template<typename XstmtT, ASTERIA_SFINAE_CONSTRUCT(Xvariant, XstmtT&&)> Statement(XstmtT&& stmt) noexcept
      : m_stor(rocket::forward<XstmtT>(stmt))
      {
      }
    template<typename XstmtT, ASTERIA_SFINAE_ASSIGN(Xvariant, XstmtT&&)> Statement& operator=(XstmtT&& stmt) noexcept
      {
        this->m_stor = rocket::forward<XstmtT>(stmt);
        return *this;
      }

  public:
    Index index() const noexcept
      {
        return static_cast<Index>(this->m_stor.index());
      }

    void generate_code(cow_vector<uptr<Air_Node>>& code, cow_vector<phsh_string>* names_opt, Analytic_Context& ctx,
                       const Compiler_Options& options) const;
  };

}  // namespace Asteria

#endif
