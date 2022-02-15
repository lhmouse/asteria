// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_STATEMENT_HPP_
#define ASTERIA_COMPILER_STATEMENT_HPP_

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

    struct S_variables
      {
        bool immutable;
        cow_vector<Source_Location> slocs;
        cow_vector<cow_vector<phsh_string>> decls;
        cow_vector<S_expression> inits;
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
        S_expression cond;
        S_block branch_true;
        S_block branch_false;
      };

    struct S_switch
      {
        S_expression ctrl;
        cow_vector<S_expression> labels;
        cow_vector<S_block> bodies;
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
        phsh_string name_key;
        phsh_string name_mapped;
        S_expression init;
        S_block body;
      };

    struct S_for
      {
        S_block init;
        S_expression cond;
        S_expression step;
        S_block body;
      };

    struct S_try
      {
        Source_Location sloc_try;
        S_block body_try;
        Source_Location sloc_catch;
        phsh_string name_except;
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

    struct S_references
      {
        cow_vector<Source_Location> slocs;
        cow_vector<phsh_string> names;
        cow_vector<S_expression> inits;
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
    ::rocket::variant<
      ROCKET_CDR(
        ,S_expression  //  0,
        ,S_block       //  1,
        ,S_variables   //  2,
        ,S_function    //  3,
        ,S_if          //  4,
        ,S_switch      //  5,
        ,S_do_while    //  6,
        ,S_while       //  7,
        ,S_for_each    //  8,
        ,S_for         //  9,
        ,S_try         // 10,
        ,S_break       // 11,
        ,S_continue    // 12,
        ,S_throw       // 13,
        ,S_return      // 14,
        ,S_assert      // 15,
        ,S_defer       // 16,
        ,S_references  // 17,
      )>
      m_stor;

  public:
    // Constructors and assignment operators
    template<typename XStmtT,
    ROCKET_ENABLE_IF(::std::is_constructible<decltype(m_stor), XStmtT&&>::value)>
    constexpr
    Statement(XStmtT&& xstmt)
       noexcept(::std::is_nothrow_constructible<decltype(m_stor), XStmtT&&>::value)
      : m_stor(::std::forward<XStmtT>(xstmt))
      { }

    template<typename XStmtT,
    ROCKET_ENABLE_IF(::std::is_assignable<decltype(m_stor)&, XStmtT&&>::value)>
    Statement&
    operator=(XStmtT&& xstmt)
      noexcept(::std::is_nothrow_assignable<decltype(m_stor)&, XStmtT&&>::value)
      { this->m_stor = ::std::forward<XStmtT>(xstmt);
        return *this;  }

    Statement&
    swap(Statement& other) noexcept
      { this->m_stor.swap(other.m_stor);
        return *this;  }

  public:
    Index
    index() const noexcept
      { return static_cast<Index>(this->m_stor.index());  }

    bool
    is_empty_return() const noexcept
      {
        if(this->index() != index_return)
          return false;
        else
          return this->m_stor.as<index_return>().expr.units.empty();
      }

    cow_vector<AIR_Node>&
    generate_code(cow_vector<AIR_Node>& code, cow_vector<phsh_string>* names_opt,
                  const Global_Context& global, Analytic_Context& ctx,
                  const Compiler_Options& opts, PTC_Aware ptc) const;
  };

inline void
swap(Statement& lhs, Statement& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
