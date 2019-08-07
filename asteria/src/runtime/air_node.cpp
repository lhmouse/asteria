// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_node.hpp"
#include "analytic_context.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "evaluation_stack.hpp"
#include "instantiated_function.hpp"
#include "exception.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    AIR_Node::Status do_execute_statement_list(Executive_Context& ctx, const cow_vector<AIR_Node>& code)
      {
        // Execute all nodes sequentially.
        auto status = AIR_Node::status_next;
        rocket::any_of(code, [&](const AIR_Node& node) { return (status = node.execute(ctx)) != AIR_Node::status_next;  });
        return status;
      }

    AIR_Node::Status do_execute_block(const cow_vector<AIR_Node>& code, const Executive_Context& ctx)
      {
        // Execute the block on a new context.
        Executive_Context ctx_body(1, ctx);
        auto status = do_execute_statement_list(ctx_body, code);
        return status;
      }

    }

AIR_Node::Status AIR_Node::execute(Executive_Context& ctx) const
  {
    switch(this->index()) {
    case index_clear_stack:
      {
        // Clear the stack.
        ctx.stack().clear_references();
        return status_next;
      }
    case index_execute_block:
      {
        const auto& altr = this->m_stor.as<index_execute_block>();
        // Execute the block in a new context.
        return do_execute_block(altr.code_body, ctx);
      }
    case index_declare_variable:
      {
        const auto& altr = this->m_stor.as<index_declare_variable>();
        // Allocate a variable and initialize it to `null`.
        auto var = ctx.global().create_variable();
        var->reset(altr.sloc, G_null(), altr.immutable);
        // Inject the variable into the current context.
        Reference_Root::S_variable xref = { rocket::move(var) };
        ctx.open_named_reference(altr.name) = xref;
        // Push a copy of the reference onto the stack.
        ctx.stack().push_reference(rocket::move(xref));
        return status_next;
      }
    case index_initialize_variable:
      {
        const auto& altr = this->m_stor.as<index_initialize_variable>();
        // Read the value of the initializer.
        // Note that the initializer must not have been empty for this code.
        auto value = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        // Get the variable back.
        auto var = ctx.stack().get_top_reference().get_variable_opt();
        ROCKET_ASSERT(var);
        // Initialize it.
        var->reset(rocket::move(value), altr.immutable);
        return status_next;
      }
    case index_if_statement:
      {
        const auto& altr = this->m_stor.as<index_if_statement>();
        // Pick a branch basing on the condition.
        bool cond = ctx.stack().get_top_reference().read().test() != altr.negative;
        if(cond) {
          return do_execute_block(altr.code_true, ctx);
        }
        return do_execute_block(altr.code_false, ctx);
      }
    default:
      ASTERIA_TERMINATE("An unknown AIR node type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

Variable_Callback& AIR_Node::enumerate_variables(Variable_Callback& callback) const
  {
    switch(this->index()) {
    case index_clear_stack:
      {
        return callback;
      }
    case index_execute_block:
      {
        const auto& altr = this->m_stor.as<index_execute_block>();
        // Enumerate all nodes of the body.
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_declare_variable:
    case index_initialize_variable:
      {
        return callback;
      }
    case index_if_statement:
      {
        const auto& altr = this->m_stor.as<index_if_statement>();
        // Enumerate all nodes of both branches.
        rocket::for_each(altr.code_true, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_false, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_switch_statement:
      {
        const auto& altr = this->m_stor.as<index_switch_statement>();
        for(size_t i = 0; i != altr.clauses.size(); ++i) {
          // Enumerate all nodes of both the label and the clause.
          rocket::for_each(altr.clauses[i].first, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
          rocket::for_each(altr.clauses[i].second.first, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        }
        return callback;
      }
    case index_do_while_statement:
      {
        const auto& altr = this->m_stor.as<index_do_while_statement>();
        // Enumerate all nodes of both the body and the condition.
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_while_statement:
      {
        const auto& altr = this->m_stor.as<index_while_statement>();
        // Enumerate all nodes of both the condition and the body.
        rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_for_each_statement:
      {
        const auto& altr = this->m_stor.as<index_for_each_statement>();
        // Enumerate all nodes of both the range initializer and the body.
        rocket::for_each(altr.code_init, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_for_statement:
      {
        const auto& altr = this->m_stor.as<index_for_statement>();
        // Enumerate all nodes of both the triplet and the body.
        rocket::for_each(altr.code_init, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_cond, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_step, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_body, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_try_statement:
      {
        const auto& altr = this->m_stor.as<index_try_statement>();
        // Enumerate all nodes of both clauses.
        rocket::for_each(altr.code_try, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_catch, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_throw_statement:
    case index_assert_statement:
    case index_simple_status:
    case index_return_by_value:
    case index_push_literal:
    case index_push_global_reference:
    case index_push_local_reference:
      {
        return callback;
      }
    case index_push_bound_reference:
      {
        const auto& altr = this->m_stor.as<index_push_bound_reference>();
        // Descend into the bound reference.
        altr.ref.enumerate_variables(callback);
        return callback;
      }
    case index_define_function:
      {
        return callback;
      }
    case index_branch_expression:
      {
        const auto& altr = this->m_stor.as<index_branch_expression>();
        // Enumerate all nodes of both branches.
        rocket::for_each(altr.code_true, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        rocket::for_each(altr.code_false, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_coalescence:
      {
        const auto& altr = this->m_stor.as<index_coalescence>();
        // Enumerate all nodes of the null branch.
        rocket::for_each(altr.code_null, [&](const AIR_Node& node) { node.enumerate_variables(callback);  });
        return callback;
      }
    case index_function_call_tail:
    case index_function_call_plain:
    case index_member_access:
    case index_push_unnamed_array:
    case index_push_unnamed_object:
    case index_apply_xop_inc_post:
    case index_apply_xop_dec_post:
    case index_apply_xop_subscr:
    case index_apply_xop_pos:
    case index_apply_xop_neg:
    case index_apply_xop_notb:
    case index_apply_xop_notl:
    case index_apply_xop_inc:
    case index_apply_xop_dec:
    case index_apply_xop_unset:
    case index_apply_xop_lengthof:
    case index_apply_xop_typeof:
    case index_apply_xop_sqrt:
    case index_apply_xop_isnan:
    case index_apply_xop_isinf:
    case index_apply_xop_abs:
    case index_apply_xop_signb:
    case index_apply_xop_round:
    case index_apply_xop_floor:
    case index_apply_xop_ceil:
    case index_apply_xop_trunc:
    case index_apply_xop_iround:
    case index_apply_xop_ifloor:
    case index_apply_xop_iceil:
    case index_apply_xop_itrunc:
    case index_apply_xop_cmp_xeq:
    case index_apply_xop_cmp_xrel:
    case index_apply_xop_cmp_3way:
    case index_apply_xop_add:
    case index_apply_xop_sub:
    case index_apply_xop_mul:
    case index_apply_xop_div:
    case index_apply_xop_mod:
    case index_apply_xop_sll:
    case index_apply_xop_srl:
    case index_apply_xop_sla:
    case index_apply_xop_sra:
    case index_apply_xop_andb:
    case index_apply_xop_orb:
    case index_apply_xop_xorb:
    case index_apply_xop_assign:
    case index_apply_xop_fma:
      {
        return callback;
      }
    default:
      ASTERIA_TERMINATE("An unknown AIR node type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
