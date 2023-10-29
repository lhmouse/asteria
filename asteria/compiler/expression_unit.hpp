// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_EXPRESSION_UNIT_
#define ASTERIA_COMPILER_EXPRESSION_UNIT_

#include "../fwd.hpp"
#include "../value.hpp"
#include "../source_location.hpp"
namespace asteria {

class Expression_Unit
  {
  public:
    struct S_literal
      {
        Value value;
      };

    struct S_local_reference
      {
        Source_Location sloc;
        phsh_string name;
      };

    struct S_closure_function
      {
        Source_Location sloc;
        cow_string unique_name;
        cow_vector<phsh_string> params;
        cow_vector<Statement> body;
      };

    enum branch_type : uint8_t
      {
        branch_type_false = 0,
        branch_type_true  = 1,
        branch_type_null  = 2,
      };

    struct branch
      {
        branch_type type;
        cow_vector<Expression_Unit> units;
      };

    struct S_branch
      {
        Source_Location sloc;
        cow_vector<branch> branches;
        bool assign;
      };

    struct argument
      {
        cow_vector<Expression_Unit> units;
      };

    struct S_function_call
      {
        Source_Location sloc;
        cow_vector<argument> args;
      };

    struct S_operator_rpn
      {
        Source_Location sloc;
        Xop xop;
        bool assign;
      };

    struct S_unnamed_array
      {
        Source_Location sloc;
        uint32_t nelems;
      };

    struct S_unnamed_object
      {
        Source_Location sloc;
        cow_vector<phsh_string> keys;
      };

    struct S_global_reference
      {
        Source_Location sloc;
        phsh_string name;
      };

    struct S_variadic_call
      {
        Source_Location sloc;
        cow_vector<argument> args;
      };

    struct S_check_argument
      {
        Source_Location sloc;
        bool by_ref;
      };

    struct S_import_call
      {
        Source_Location sloc;
        cow_vector<argument> args;
      };

    struct S_catch
      {
        cow_vector<Expression_Unit> operand;
      };

    enum Index : uint8_t
      {
        index_literal           =  0,
        index_local_reference   =  1,
        index_closure_function  =  2,
        index_branch            =  3,
        index_function_call     =  4,
        index_operator_rpn      =  5,
        index_unnamed_array     =  6,
        index_unnamed_object    =  7,
        index_global_reference  =  8,
        index_variadic_call     =  9,
        index_check_argument    = 10,
        index_import_call       = 11,
        index_catch             = 12,
      };

  private:
    ASTERIA_VARIANT(
      m_stor
        , S_literal           //  0
        , S_local_reference   //  1
        , S_closure_function  //  2
        , S_branch            //  3
        , S_function_call     //  4
        , S_operator_rpn      //  5
        , S_unnamed_array     //  6
        , S_unnamed_object    //  7
        , S_global_reference  //  8
        , S_variadic_call     //  9
        , S_check_argument    // 10
        , S_import_call       // 11
        , S_catch             // 12
      );

  public:
    // Constructors and assignment operators
    template<typename XUnitT,
    ROCKET_ENABLE_IF(::std::is_constructible<decltype(m_stor), XUnitT&&>::value)>
    constexpr
    Expression_Unit(XUnitT&& xunit)
      noexcept(::std::is_nothrow_constructible<decltype(m_stor), XUnitT&&>::value)
      :
        m_stor(::std::forward<XUnitT>(xunit))
      { }

    template<typename XUnitT,
    ROCKET_ENABLE_IF(::std::is_assignable<decltype(m_stor)&, XUnitT&&>::value)>
    Expression_Unit&
    operator=(XUnitT&& xunit) &
      noexcept(::std::is_nothrow_assignable<decltype(m_stor)&, XUnitT&&>::value)
      {
        this->m_stor = ::std::forward<XUnitT>(xunit);
        return *this;
      }

    Expression_Unit&
    swap(Expression_Unit& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    // Checks whether this expression unit may use `alt_stack`.
    bool
    clobbers_alt_stack() const noexcept;

    // Generate IR nodes into `code`.
    void
    generate_code(cow_vector<AIR_Node>& code, const Compiler_Options& opts,
                  const Global_Context& global, const Analytic_Context& ctx,
                  PTC_Aware ptc) const;
  };

inline
void
swap(Expression_Unit& lhs, Expression_Unit& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace asteria
#endif
