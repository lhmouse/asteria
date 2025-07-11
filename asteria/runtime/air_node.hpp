// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_AIR_NODE_
#define ASTERIA_RUNTIME_AIR_NODE_

#include "../fwd.hpp"
#include "reference.hpp"
#include "../value.hpp"
#include "../source_location.hpp"
namespace asteria {

class AIR_Node
  {
  public:
    struct S_clear_stack
      {
      };

    struct S_execute_block
      {
        cow_vector<AIR_Node> code_body;
      };

    struct S_declare_variable
      {
        Source_Location sloc;
        phcow_string name;
      };

    struct S_initialize_variable
      {
        Source_Location sloc;
        bool immutable;
      };

    struct S_if_statement
      {
        bool negative;
        cow_vector<AIR_Node> code_true;
        cow_vector<AIR_Node> code_false;
      };

    struct switch_clause
      {
        Switch_Clause_Type type;
        bool lower_closed;
        bool upper_closed;
        cow_vector<AIR_Node> code_labels;
        cow_vector<AIR_Node> code_body;
        cow_vector<phcow_string> names_added;
      };

    struct S_switch_statement
      {
        cow_vector<switch_clause> clauses;
      };

    struct S_do_while_statement
      {
        cow_vector<AIR_Node> code_body;
        bool negative;
        cow_vector<AIR_Node> code_cond;
        cow_vector<AIR_Node> code_complete;
      };

    struct S_while_statement
      {
        bool negative;
        cow_vector<AIR_Node> code_cond;
        cow_vector<AIR_Node> code_body;
        cow_vector<AIR_Node> code_complete;
      };

    struct S_for_each_statement
      {
        phcow_string name_key;
        phcow_string name_mapped;
        Source_Location sloc_init;
        cow_vector<AIR_Node> code_init;
        cow_vector<AIR_Node> code_body;
        cow_vector<AIR_Node> code_complete;
      };

    struct S_for_statement
      {
        cow_vector<AIR_Node> code_init;
        cow_vector<AIR_Node> code_cond;
        cow_vector<AIR_Node> code_step;
        cow_vector<AIR_Node> code_body;
        cow_vector<AIR_Node> code_complete;
      };

    struct S_try_statement
      {
        Source_Location sloc_try;
        cow_vector<AIR_Node> code_try;
        Source_Location sloc_catch;
        phcow_string name_except;
        cow_vector<AIR_Node> code_catch;
      };

    struct S_throw_statement
      {
        Source_Location sloc;
      };

    struct S_assert_statement
      {
        Source_Location sloc;
        cow_string msg;
      };

    struct S_simple_status
      {
        AIR_Status status;
      };

    struct S_check_argument
      {
        Source_Location sloc;
        bool by_ref;
      };

    struct S_push_global_reference
      {
        Source_Location sloc;
        phcow_string name;
      };

    struct S_push_local_reference
      {
        Source_Location sloc;
        uint16_t depth;
        phcow_string name;
      };

    struct S_push_bound_reference
      {
        Reference ref;
      };

    struct S_define_function
      {
        Compiler_Options opts;
        Source_Location sloc;
        cow_string func;
        cow_vector<phcow_string> params;
        cow_vector<AIR_Node> code_body;
      };

    struct S_branch_expression
      {
        Source_Location sloc;
        cow_vector<AIR_Node> code_true;
        cow_vector<AIR_Node> code_false;
        bool assign;
      };

    struct S_function_call
      {
        Source_Location sloc;
        uint32_t nargs;
        PTC_Aware ptc;
      };

    struct S_push_unnamed_array
      {
        Source_Location sloc;
        uint32_t nelems;
      };

    struct S_push_unnamed_object
      {
        Source_Location sloc;
        cow_vector<phcow_string> keys;
      };

    struct S_apply_operator
      {
        Source_Location sloc;
        Xop xop;
        bool assign;
      };

    struct S_unpack_array
      {
        Source_Location sloc;
        bool immutable;
        uint32_t nelems;
      };

    struct S_unpack_object
      {
        Source_Location sloc;
        bool immutable;
        cow_vector<phcow_string> keys;
      };

    struct S_define_null_variable
      {
        Source_Location sloc;
        bool immutable;
        phcow_string name;
      };

    struct S_single_step_trap
      {
        Source_Location sloc;
      };

    struct S_variadic_call
      {
        Source_Location sloc;
        PTC_Aware ptc;
      };

    struct S_defer_expression
      {
        Source_Location sloc;
        cow_vector<AIR_Node> code_body;
      };

    struct S_import_call
      {
        Compiler_Options opts;
        Source_Location sloc;
        uint32_t nargs;
      };

    struct S_declare_reference
      {
        phcow_string name;
      };

    struct S_initialize_reference
      {
        Source_Location sloc;
        phcow_string name;
      };

    struct S_catch_expression
      {
        cow_vector<AIR_Node> code_body;
      };

    struct S_return_statement
      {
        Source_Location sloc;
        bool by_ref;
        bool is_void;
      };

    struct S_push_constant
      {
        Value val;
      };

    struct S_alt_clear_stack
      {
      };

    struct S_alt_function_call
      {
        Source_Location sloc;
        PTC_Aware ptc;
      };

    struct S_coalesce_expression
      {
        Source_Location sloc;
        cow_vector<AIR_Node> code_null;
        bool assign;
      };

    struct S_member_access
      {
        Source_Location sloc;
        phcow_string key;
      };

    struct S_apply_operator_bi32
      {
        Source_Location sloc;
        Xop xop;
        bool assign;
        int32_t irhs;
      };

    struct S_return_statement_bi32
      {
        Source_Location sloc;
        Type type;
        int32_t irhs;
      };

    struct S_check_null
      {
        Source_Location sloc;
        bool negative;
      };

    enum Index : uint8_t
      {
        index_clear_stack            =  0,
        index_execute_block          =  1,
        index_declare_variable       =  2,
        index_initialize_variable    =  3,
        index_if_statement           =  4,
        index_switch_statement       =  5,
        index_do_while_statement     =  6,
        index_while_statement        =  7,
        index_for_each_statement     =  8,
        index_for_statement          =  9,
        index_try_statement          = 10,
        index_throw_statement        = 11,
        index_assert_statement       = 12,
        index_simple_status          = 13,
        index_check_argument         = 14,
        index_push_global_reference  = 15,
        index_push_local_reference   = 16,
        index_push_bound_reference   = 17,
        index_define_function        = 18,
        index_branch_expression      = 19,
        index_function_call          = 20,
        index_push_unnamed_array     = 21,
        index_push_unnamed_object    = 22,
        index_apply_operator         = 23,
        index_unpack_array           = 24,
        index_unpack_object          = 25,
        index_define_null_variable   = 26,
        index_single_step_trap       = 27,
        index_variadic_call          = 28,
        index_defer_expression       = 29,
        index_import_call            = 30,
        index_declare_reference      = 31,
        index_initialize_reference   = 32,
        index_catch_expression       = 33,
        index_return_statement       = 34,
        index_push_constant          = 35,
        index_alt_clear_stack        = 36,
        index_alt_function_call      = 37,
        index_coalesce_expression    = 38,
        index_member_access          = 39,
        index_apply_operator_bi32    = 40,
        index_return_statement_bi32  = 41,
        index_check_null             = 42,
      };

  private:
    ASTERIA_VARIANT(
      m_stor
        , S_clear_stack            //  0
        , S_execute_block          //  1
        , S_declare_variable       //  2
        , S_initialize_variable    //  3
        , S_if_statement           //  4
        , S_switch_statement       //  5
        , S_do_while_statement     //  6
        , S_while_statement        //  7
        , S_for_each_statement     //  8
        , S_for_statement          //  9
        , S_try_statement          // 10
        , S_throw_statement        // 11
        , S_assert_statement       // 12
        , S_simple_status          // 13
        , S_check_argument         // 14
        , S_push_global_reference  // 15
        , S_push_local_reference   // 16
        , S_push_bound_reference   // 17
        , S_define_function        // 18
        , S_branch_expression      // 19
        , S_function_call          // 20
        , S_push_unnamed_array     // 21
        , S_push_unnamed_object    // 22
        , S_apply_operator         // 23
        , S_unpack_array           // 24
        , S_unpack_object          // 25
        , S_define_null_variable   // 26
        , S_single_step_trap       // 27
        , S_variadic_call          // 28
        , S_defer_expression       // 29
        , S_import_call            // 30
        , S_declare_reference      // 31
        , S_initialize_reference   // 32
        , S_catch_expression       // 33
        , S_return_statement       // 34
        , S_push_constant          // 35
        , S_alt_clear_stack        // 36,
        , S_alt_function_call      // 37,
        , S_coalesce_expression    // 38,
        , S_member_access          // 39,
        , S_apply_operator_bi32    // 40,
        , S_return_statement_bi32  // 41,
        , S_check_null             // 42,
      );

  public:
    // Constructors and assignment operators
    template<typename xNode,
    ROCKET_ENABLE_IF(::std::is_constructible<decltype(m_stor), xNode&&>::value)>
    constexpr AIR_Node(xNode&& xnode)
      noexcept(::std::is_nothrow_constructible<decltype(m_stor), xNode&&>::value)
      :
        m_stor(forward<xNode>(xnode))
      { }

    template<typename xNode,
    ROCKET_ENABLE_IF(::std::is_assignable<decltype(m_stor)&, xNode&&>::value)>
    AIR_Node&
    operator=(xNode&& xnode) &
      noexcept(::std::is_nothrow_assignable<decltype(m_stor)&, xNode&&>::value)
      {
        this->m_stor = forward<xNode>(xnode);
        return *this;
      }

    AIR_Node&
    swap(AIR_Node& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    // Gets the constant value, if any.
    opt<Value>
    get_constant_opt() const noexcept;

    // Checks whether this node terminates the control flow, which renders all
    // subsequent nodes unreachable.
    bool
    is_terminator() const noexcept;

    // If this node denotes a local reference which is allocated in an executive
    // context, replace it with a copy of the reference.
    opt<AIR_Node>
    rebind_opt(const Abstract_Context& ctx) const;

    // This is necessary because the body of a closure shall not have been
    // solidified.
    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const;

    // Compress this IR node into `rod` for execution.
    void
    solidify(AVM_Rod& rod) const;
  };

inline
void
swap(AIR_Node& lhs, AIR_Node& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria
#endif
