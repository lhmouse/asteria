// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_STATEMENT_HPP_
#define ASTERIA_COMPILER_STATEMENT_HPP_

#include "../fwd.hpp"
#include "../source_location.hpp"

namespace Asteria {

class Statement
  {
  public:
    struct S_expression
      {
        cow_vector<Xprunit> expr;
      };
    struct S_block
      {
        cow_vector<Statement> body;
      };
    struct S_variables
      {
        bool immutable;
        cow_vector<Source_Location> slocs;
        cow_vector<cow_vector<phsh_string>> decls;
        cow_vector<cow_vector<Xprunit>> inits;
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
        cow_vector<cow_vector<Xprunit>> labels;
        cow_vector<cow_vector<Statement>> bodies;
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
        phsh_string name_key;
        phsh_string name_mapped;
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
        phsh_string name_except;
        cow_vector<Statement> body_catch;
      };
    struct S_break
      {
        Jump_Target target;
      };
    struct S_continue
      {
        Jump_Target target;
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
      };
    using Xvariant = variant<
      ROCKET_CDR(
        , S_expression  //  0,
        , S_block       //  1,
        , S_variables   //  2,
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
      :
        m_stor(rocket::forward<XstmtT>(stmt))
      { }
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
    bool is_empty_return() const noexcept
      {
        return (this->m_stor.index() == index_return) && this->m_stor.as<index_return>().expr.empty();
      }

    void swap(Statement& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
      }

    cow_vector<AIR_Node>& generate_code(cow_vector<AIR_Node>& code, cow_vector<phsh_string>* names_opt,
                                        Analytic_Context& ctx, const Compiler_Options& opts, TCO_Aware tco_aware) const;
  };

inline void swap(Statement& lhs, Statement& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
