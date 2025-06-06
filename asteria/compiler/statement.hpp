// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_STATEMENT_
#define ASTERIA_COMPILER_STATEMENT_

#include "../fwd.hpp"
#include "../source_location.hpp"
namespace asteria {

class Statement
  {
  public:
    struct S_expression
      {
        Source_Location sloc;
        cow_vector<Expression_Unit> units;
      };

    struct S_block
      {
        cow_vector<Statement> stmts;
      };

    struct variable_declaration
      {
        Source_Location sloc;
        cow_vector<phcow_string> names;
        S_expression init;
      };

    struct S_variables
      {
        bool immutable;
        cow_vector<variable_declaration> decls;
      };

    struct S_function
      {
        Source_Location sloc;
        phcow_string name;
        cow_vector<phcow_string> params;
        cow_vector<Statement> body;
      };

    struct S_if
      {
        bool negative;
        S_expression cond;
        S_block branch_true;
        S_block branch_false;
      };

    struct switch_clause
      {
        Switch_Clause_Type type;
        bool lower_closed;  // lower bound is closed in `each` clause
        bool upper_closed;  // upper bound is closed in `each` clause
        S_expression label_lower;
        S_expression label_upper;
        cow_vector<Statement> body;
      };

    struct S_switch
      {
        S_expression ctrl;
        cow_vector<switch_clause> clauses;
      };

    struct S_do_while
      {
        S_block body;
        bool negative;
        S_expression cond;
      };

    struct S_while
      {
        bool negative;
        S_expression cond;
        S_block body;
      };

    struct S_for_each
      {
        phcow_string name_key;
        phcow_string name_mapped;
        Source_Location sloc_init;
        S_expression init;
        S_block body;
      };

    struct S_for
      {
        cow_vector<Statement> init;
        S_expression cond;
        S_expression step;
        S_block body;
      };

    struct S_try
      {
        Source_Location sloc_try;
        S_block body_try;
        Source_Location sloc_catch;
        phcow_string name_except;
        S_block body_catch;
      };

    struct S_break
      {
        Source_Location sloc;
        Jump_Target target;
      };

    struct S_continue
      {
        Source_Location sloc;
        Jump_Target target;
      };

    struct S_throw
      {
        Source_Location sloc;
        S_expression expr;
      };

    struct S_return
      {
        Source_Location sloc;
        bool by_ref;
        S_expression expr;
      };

    struct S_assert
      {
        Source_Location sloc;
        S_expression expr;
        cow_string msg;
      };

    struct S_defer
      {
        Source_Location sloc;
        S_expression expr;
      };

    struct reference_declaration
      {
        Source_Location sloc;
        phcow_string name;
        S_expression init;
      };

    struct S_references
      {
        cow_vector<reference_declaration> decls;
      };

    enum Index : uint8_t
      {
        index_expression  =  0,
        index_block       =  1,
        index_variables   =  2,
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
        index_defer       = 16,
        index_references  = 17,
      };

  private:
    ASTERIA_VARIANT(
      m_stor
        , S_expression  //  0
        , S_block       //  1
        , S_variables   //  2
        , S_function    //  3
        , S_if          //  4
        , S_switch      //  5
        , S_do_while    //  6
        , S_while       //  7
        , S_for_each    //  8
        , S_for         //  9
        , S_try         // 10
        , S_break       // 11
        , S_continue    // 12
        , S_throw       // 13
        , S_return      // 14
        , S_assert      // 15
        , S_defer       // 16
        , S_references  // 17
      );

  public:
    // Constructors and assignment operators
    template<typename xStatement,
    ROCKET_ENABLE_IF(::std::is_constructible<decltype(m_stor), xStatement&&>::value)>
    constexpr Statement(xStatement&& xstmt)
       noexcept(::std::is_nothrow_constructible<decltype(m_stor), xStatement&&>::value)
      :
        m_stor(forward<xStatement>(xstmt))
      { }

    template<typename xStatement,
    ROCKET_ENABLE_IF(::std::is_assignable<decltype(m_stor)&, xStatement&&>::value)>
    Statement&
    operator=(xStatement&& xstmt) &
      noexcept(::std::is_nothrow_assignable<decltype(m_stor)&, xStatement&&>::value)
      {
        this->m_stor = forward<xStatement>(xstmt);
        return *this;
      }

    Statement&
    swap(Statement& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    // Checks whether this is `return;`, which terminates the control flow with
    // a void reference.
    bool
    is_empty_return() const noexcept
      {
        return (this->m_stor.index() == index_return)
               && this->m_stor.as<S_return>().expr.units.empty();
      }

    // Checks whether this statement does not modify the current context.
    bool
    is_scopeless() const noexcept
      {
        return ::rocket::is_none_of(this->m_stor.index(),
            { index_variables, index_references, index_function, index_defer });
      }

    // Generate IR nodes into `code`. If `names_opt` is not a null pointer and
    // this statement declares new names within `ctx`, those names are appended
    // to `*names_opt`.
    void
    generate_code(cow_vector<AIR_Node>& code, Analytic_Context& ctx,
                  cow_vector<phcow_string>* names_opt, const Global_Context& global,
                  const Compiler_Options& opts, PTC_Aware ptc) const;
  };

inline
void
swap(Statement& lhs, Statement& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria
#endif
