// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STATEMENT_HPP_
#define ASTERIA_STATEMENT_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"
#include "xpnode.hpp"
#include "reference.hpp"

namespace Asteria {

class Statement
  {
  public:
    enum Target_scope : std::uint8_t
      {
        target_scope_unspec  = 0,
        target_scope_switch  = 1,
        target_scope_while   = 2,
        target_scope_for     = 3,
      };
    enum Status : std::uint8_t
      {
        status_next             = 0,
        status_break_unspec     = 1,
        status_break_switch     = 2,
        status_break_while      = 3,
        status_break_for        = 4,
        status_continue_unspec  = 5,
        status_continue_while   = 6,
        status_continue_for     = 7,
        status_return           = 8,
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
      };
    struct S_expression
      {
        Vector<Xpnode> expr;
      };
    struct S_var_def
      {
        String name;
        bool immutable;
        Vector<Xpnode> init;
      };
    struct S_func_def
      {
        String name;
        Vector<String> params;
        String file;
        Unsigned line;
        Vector<Statement> body;
      };
    struct S_if
      {
        Vector<Xpnode> cond;
        Vector<Statement> branch_true;
        Vector<Statement> branch_false;
      };
    struct S_switch
      {
        Vector<Xpnode> ctrl;
        Bivector<Vector<Xpnode>, Vector<Statement>> clauses;
      };
    struct S_do_while
      {
        Vector<Statement> body;
        Vector<Xpnode> cond;
      };
    struct S_while
      {
        Vector<Xpnode> cond;
        Vector<Statement> body;
      };
    struct S_for
      {
        String var_name;
        bool var_immutable;
        Vector<Xpnode> var_init;
        Vector<Xpnode> cond;
        Vector<Xpnode> step;
        Vector<Statement> body;
      };
    struct S_for_each
      {
        String key_name;
        String mapped_name;
        Vector<Xpnode> range_init;
        Vector<Statement> body;
      };
    struct S_try
      {
        Vector<Statement> body_try;
        String except_name;
        Vector<Statement> body_catch;
      };
    struct S_break
      {
        Target_scope target;
      };
    struct S_continue
      {
        Target_scope target;
      };
    struct S_throw
      {
        Vector<Xpnode> expr;
      };
    struct S_return
      {
        Vector<Xpnode> expr;
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
      )>;

  private:
    Variant m_variant;

  public:
    template<typename CandidateT, typename std::enable_if<std::is_constructible<Variant, CandidateT &&>::value>::type * = nullptr>
      Statement(CandidateT &&cand)
        : m_variant(std::forward<CandidateT>(cand))
        {
        }
    Statement(Statement &&) noexcept;
    Statement & operator=(Statement &&) noexcept;
    ~Statement();

  public:
    Index index() const noexcept
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

extern Statement bind_statement_partial(Analytic_context &ctx_inout, const Statement &stmt);
extern Vector<Statement> bind_block_in_place(Analytic_context &ctx_inout, const Vector<Statement> &block);
extern Vector<Statement> bind_block(const Vector<Statement> &block, const Analytic_context &ctx);

extern Statement::Status execute_statement_partial(Reference &ref_out, Executive_context &ctx_inout, const Statement &stmt);
extern Statement::Status execute_block_in_place(Reference &ref_out, Executive_context &ctx_inout, const Vector<Statement> &block);
extern Statement::Status execute_block(Reference &ref_out, const Vector<Statement> &block, const Executive_context &ctx);

}

#endif
