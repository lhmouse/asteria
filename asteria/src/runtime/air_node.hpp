// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_AIR_NODE_HPP_
#define ASTERIA_RUNTIME_AIR_NODE_HPP_

#include "../fwd.hpp"
#include "reference.hpp"
#include "../value.hpp"
#include "../source_location.hpp"

namespace Asteria {

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
        cow_vector<AIR_Node> code_try;
        Source_Location sloc;
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
    struct S_simple_status
      {
        AIR_Status status;
      };
    struct S_return_by_value
      {
      };
    struct S_push_literal
      {
        Value val;
      };
    struct S_push_global_reference
      {
        phsh_string name;
      };
    struct S_push_local_reference
      {
        uint32_t depth;
        phsh_string name;
      };
    struct S_push_bound_reference
      {
        Reference ref;
      };
    struct S_instantiate_function
      {
        Compiler_Options opts;
        Source_Location sloc;
        cow_string name;
        cow_vector<phsh_string> params;
        cow_vector<Statement> body;
      };
    struct S_branch_expression
      {
        cow_vector<AIR_Node> code_true;
        cow_vector<AIR_Node> code_false;
        bool assign;
      };
    struct S_coalescence
      {
        cow_vector<AIR_Node> code_null;
        bool assign;
      };
    struct S_function_call
      {
        Source_Location sloc;
        cow_vector<bool> args_by_refs;
        TCO_Aware tco_aware;
      };
    struct S_member_access
      {
        phsh_string name;
      };
    struct S_push_unnamed_array
      {
        uint32_t nelems;
      };
    struct S_push_unnamed_object
      {
        cow_vector<phsh_string> keys;
      };
    struct S_apply_operator
      {
        Xop xop;
        bool assign;
      };
    struct S_unpack_struct_array
      {
        bool immutable;
        uint32_t nelems;
      };
    struct S_unpack_struct_object
      {
        bool immutable;
        cow_vector<phsh_string> keys;
      };
    struct S_define_null_variable
      {
        bool immutable;
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
        index_simple_status          = 13,
        index_return_by_value        = 14,
        index_push_literal           = 15,
        index_push_global_reference  = 16,
        index_push_local_reference   = 17,
        index_push_bound_reference   = 18,
        index_instantiate_function   = 19,
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
      };
    using Xvariant = variant<
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
        , S_simple_status          // 13,
        , S_return_by_value        // 14,
        , S_push_literal           // 15,
        , S_push_global_reference  // 16,
        , S_push_local_reference   // 17,
        , S_push_bound_reference   // 18,
        , S_instantiate_function   // 19,
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
      )>;
    static_assert(::std::is_nothrow_copy_assignable<Xvariant>::value, "");

  private:
    Xvariant m_stor;

  public:
    template<typename XnodeT, ASTERIA_SFINAE_CONSTRUCT(Xvariant, XnodeT&&)> AIR_Node(XnodeT&& xnode)
      :
        m_stor(::rocket::forward<XnodeT>(xnode))
      {
      }
    template<typename XnodeT, ASTERIA_SFINAE_ASSIGN(Xvariant, XnodeT&&)> AIR_Node& operator=(XnodeT&& xnode)
      {
        this->m_stor = ::rocket::forward<XnodeT>(xnode);
        return *this;
      }

  public:
    Index index() const noexcept
      {
        return static_cast<Index>(this->m_stor.index());
      }

    AIR_Node& swap(AIR_Node& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    // Perform dead code elimination on this node.
    // This function is meant to be called recursively.
    DCE_Result optimize_dce();

    // Compress this IR node.
    // Be advised that solid nodes cannot be copied or moved because they occupy variant numbers of bytes.
    // Solidification is performed as two passes: The total number of bytes is calculated, which are allocated
    // as a whole at the end of the first pass, where nodes are constructed in the second pass.
    // The argument for `ipass` shall be `0` for the first pass and `1` for the second pass.
    AVMC_Queue& solidify(AVMC_Queue& queue, uint8_t ipass) const;
    // Instantiate this IR node to create a function. This node must hold a value of type `S_instantiate_function`.
    // This function is provided merely for convenience elsewhere. It is not intended for public use.
    rcptr<Abstract_Function> instantiate_function(const Abstract_Context* parent_opt) const;
  };

inline void swap(AIR_Node& lhs, AIR_Node& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
