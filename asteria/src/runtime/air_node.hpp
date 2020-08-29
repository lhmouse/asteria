// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_AIR_NODE_HPP_
#define ASTERIA_RUNTIME_AIR_NODE_HPP_

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
        phsh_string name;
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

    struct S_switch_statement
      {
        cow_vector<cow_vector<AIR_Node>> code_labels;
        cow_vector<cow_vector<AIR_Node>> code_bodies;
        cow_vector<cow_vector<phsh_string>> names_added;
      };

    struct S_do_while_statement
      {
        cow_vector<AIR_Node> code_body;
        bool negative;
        cow_vector<AIR_Node> code_cond;
      };

    struct S_while_statement
      {
        bool negative;
        cow_vector<AIR_Node> code_cond;
        cow_vector<AIR_Node> code_body;
      };

    struct S_for_each_statement
      {
        phsh_string name_key;
        phsh_string name_mapped;
        cow_vector<AIR_Node> code_init;
        cow_vector<AIR_Node> code_body;
      };

    struct S_for_statement
      {
        cow_vector<AIR_Node> code_init;
        cow_vector<AIR_Node> code_cond;
        cow_vector<AIR_Node> code_step;
        cow_vector<AIR_Node> code_body;
      };

    struct S_try_statement
      {
        Source_Location sloc_try;
        cow_vector<AIR_Node> code_try;
        Source_Location sloc_catch;
        phsh_string name_except;
        cow_vector<AIR_Node> code_catch;
      };

    struct S_throw_statement
      {
        Source_Location sloc;
      };

    struct S_assert_statement
      {
        Source_Location sloc;
        bool negative;
        cow_string msg;
      };

    struct S_return_statement
      {
        AIR_Status status;
      };

    struct S_glvalue_to_prvalue
      {
        Source_Location sloc;
      };

    struct S_push_immediate
      {
        Value value;
      };

    struct S_push_global_reference
      {
        Source_Location sloc;
        phsh_string name;
      };

    struct S_push_local_reference
      {
        Source_Location sloc;
        uint32_t depth;
        phsh_string name;
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
        cow_vector<phsh_string> params;
        cow_vector<AIR_Node> code_body;
      };

    struct S_branch_expression
      {
        Source_Location sloc;
        cow_vector<AIR_Node> code_true;
        cow_vector<AIR_Node> code_false;
        bool assign;
      };

    struct S_coalescence
      {
        Source_Location sloc;
        cow_vector<AIR_Node> code_null;
        bool assign;
      };

    struct S_function_call
      {
        Source_Location sloc;
        uint32_t nargs;
        PTC_Aware ptc;
      };

    struct S_member_access
      {
        Source_Location sloc;
        phsh_string name;
      };

    struct S_push_unnamed_array
      {
        Source_Location sloc;
        uint32_t nelems;
      };

    struct S_push_unnamed_object
      {
        Source_Location sloc;
        cow_vector<phsh_string> keys;
      };

    struct S_apply_operator
      {
        Source_Location sloc;
        Xop xop;
        bool assign;
      };

    struct S_unpack_struct_array
      {
        Source_Location sloc;
        bool immutable;
        uint32_t nelems;
      };

    struct S_unpack_struct_object
      {
        Source_Location sloc;
        bool immutable;
        cow_vector<phsh_string> keys;
      };

    struct S_define_null_variable
      {
        bool immutable;
        Source_Location sloc;
        phsh_string name;
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

    struct S_break_or_continue
      {
        Source_Location sloc;
        AIR_Status status;
      };

    struct S_declare_reference
      {
        phsh_string name;
      };

    struct S_initialize_reference
      {
        Source_Location sloc;
        phsh_string name;
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
        index_return_statement       = 13,
        index_glvalue_to_prvalue     = 14,
        index_push_immediate         = 15,
        index_push_global_reference  = 16,
        index_push_local_reference   = 17,
        index_push_bound_reference   = 18,
        index_define_function        = 19,
        index_branch_expression      = 20,
        index_coalescence            = 21,
        index_function_call          = 22,
        index_member_access          = 23,
        index_push_unnamed_array     = 24,
        index_push_unnamed_object    = 25,
        index_apply_operator         = 26,
        index_unpack_struct_array    = 27,
        index_unpack_struct_object   = 28,
        index_define_null_variable   = 29,
        index_single_step_trap       = 30,
        index_variadic_call          = 31,
        index_defer_expression       = 32,
        index_import_call            = 33,
        index_break_or_continue      = 34,
        index_declare_reference      = 35,
        index_initialize_reference   = 36,
      };

    using Storage = variant<
      ROCKET_CDR(
      , S_clear_stack            //  0,
      , S_execute_block          //  1,
      , S_declare_variable       //  2,
      , S_initialize_variable    //  3,
      , S_if_statement           //  4,
      , S_switch_statement       //  5,
      , S_do_while_statement     //  6,
      , S_while_statement        //  7,
      , S_for_each_statement     //  8,
      , S_for_statement          //  9,
      , S_try_statement          // 10,
      , S_throw_statement        // 11,
      , S_assert_statement       // 12,
      , S_return_statement       // 13,
      , S_glvalue_to_prvalue     // 14,
      , S_push_immediate         // 15,
      , S_push_global_reference  // 16,
      , S_push_local_reference   // 17,
      , S_push_bound_reference   // 18,
      , S_define_function        // 19,
      , S_branch_expression      // 20,
      , S_coalescence            // 21,
      , S_function_call          // 22,
      , S_member_access          // 23,
      , S_push_unnamed_array     // 24,
      , S_push_unnamed_object    // 25,
      , S_apply_operator         // 26,
      , S_unpack_struct_array    // 27,
      , S_unpack_struct_object   // 28,
      , S_define_null_variable   // 29,
      , S_single_step_trap       // 30,
      , S_variadic_call          // 31,
      , S_defer_expression       // 32,
      , S_import_call            // 33,
      , S_break_or_continue      // 34,
      , S_declare_reference      // 35,
      , S_initialize_reference   // 36,
      )>;

    static_assert(::std::is_nothrow_copy_assignable<Storage>::value);

  private:
    Storage m_stor;

  public:
    ASTERIA_VARIANT_CONSTRUCTOR(AIR_Node, Storage, XNodeT, xnode)
      : m_stor(::std::forward<XNodeT>(xnode))
      { }

    ASTERIA_VARIANT_ASSIGNMENT(AIR_Node, Storage, XNodeT, xnode)
      {
        this->m_stor = ::std::forward<XNodeT>(xnode);
        return *this;
      }

  public:
    Index
    index()
    const noexcept
      { return static_cast<Index>(this->m_stor.index());  }

    AIR_Node&
    swap(AIR_Node& other)
    noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    // Rebind this node.
    // If this node refers to a local reference, which has been allocated in an
    // executive context now, we need to replace `*this` with a copy of it.
    opt<AIR_Node>
    rebind_opt(const Abstract_Context& ctx)
    const;

    // Compress this IR node.
    // The return value indicates whether this node terminates control flow i.e.
    // all subsequent nodes are unreachable.
    bool
    solidify(AVMC_Queue& queue)
    const;

    // This is needed because the body of a closure should not be solidified.
    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const;
  };

inline
void
swap(AIR_Node& lhs, AIR_Node& rhs)
noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
