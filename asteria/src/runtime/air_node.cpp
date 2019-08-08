// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_node.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "evaluation_stack.hpp"
#include "instantiated_function.hpp"
#include "exception.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    AIR_Status do_execute_statement_list(Executive_Context& ctx, const cow_vector<AIR_Node>& code)
      {
        // Execute all nodes sequentially.
        auto status = air_status_next;
        rocket::any_of(code, [&](const AIR_Node& node) { return (status = node.execute(ctx)) != air_status_next;  });
        return status;
      }

    AIR_Status do_execute_block(const cow_vector<AIR_Node>& code, const Executive_Context& ctx)
      {
        // Execute the block on a new context.
        Executive_Context ctx_body(1, ctx);
        auto status = do_execute_statement_list(ctx_body, code);
        return status;
      }

    AIR_Status do_execute_catch(const cow_vector<AIR_Node>& code, const phsh_string& name_except, const Exception& except, const Executive_Context& ctx)
      {
        // Create a fresh context.
        Executive_Context ctx_catch(1, ctx);
        // Set the exception reference.
        Reference_Root::S_temporary xref_except = { except.get_value() };
        ctx_catch.open_named_reference(name_except) = rocket::move(xref_except);
        // Set backtrace frames.
        G_array backtrace;
        for(size_t i = 0; i != except.count_frames(); ++i) {
          const auto& frame = except.get_frame(i);
          G_object r;
          // Translate each frame into a human-readable format.
          r.try_emplace(rocket::sref("frame"), G_string(rocket::sref(frame.what_type())));
          r.try_emplace(rocket::sref("file"), G_string(frame.file()));
          r.try_emplace(rocket::sref("line"), G_integer(frame.line()));
          r.try_emplace(rocket::sref("value"), frame.value());
          // Append this frame.
          backtrace.emplace_back(rocket::move(r));
        }
        ASTERIA_DEBUG_LOG("Exception backtrace:\n", Value(backtrace));
        Reference_Root::S_constant xref_bt = { rocket::move(backtrace) };
        ctx_catch.open_named_reference(rocket::sref("__backtrace")) = rocket::move(xref_bt);
        // Execute the block now.
        auto status = do_execute_statement_list(ctx_catch, code);
        return status;
      }

    const Value& do_evaluate(Executive_Context& ctx, const cow_vector<AIR_Node>& code)
      {
        if(code.empty()) {
          // Return a static `null`.
          return Value::null();
        }
        // Clear the stack.
        ctx.stack().clear_references();
        // Evaluate all nodes.
        rocket::for_each(code, [&](const AIR_Node& node) { node.execute(ctx);  });
        // The result will have been pushed onto the top.
        return ctx.stack().get_top_reference().read();
      }

    AIR_Status do_evaluate_branch(const cow_vector<AIR_Node>& code, /*const*/ Executive_Context& ctx, bool assign)
      {
        if(code.empty()) {
          // Do nothing.
          return air_status_next;
        }
        // Evaluate all nodes.
        rocket::for_each(code, [&](const AIR_Node& node) { node.execute(ctx);  });
        // Exactly one new reference will have been push onto the top.
        ctx.stack().pop_next_reference(assign);
        return air_status_next;
      }

    struct S_xfunction_call
      {
        rcobj<Abstract_Function> target;
        cow_vector<Reference> args;
      };

    S_xfunction_call do_unpack_function_call(Executive_Context& ctx, const cow_vector<bool>& args_by_refs)
      {
        Value val;
        // Allocate the argument vector.
        cow_vector<Reference> args;
        args.resize(args_by_refs.size());
        for(auto it = args.mut_rbegin(); it != args.rend(); ++it) {
          // Convert the argument to an rvalue if it shouldn't be passed by reference.
          bool by_ref = *(it - args.rbegin() + args_by_refs.rbegin());
          if(!by_ref) {
            ctx.stack().open_top_reference().convert_to_rvalue();
          }
          // Fill an argument.
          *it = rocket::move(ctx.stack().open_top_reference());
          ctx.stack().pop_reference();
        }
        // Get the target reference.
        val = ctx.stack().get_top_reference().read();
        if(!val.is_function()) {
          ASTERIA_THROW_RUNTIME_ERROR("An attempt was made to invoke `", val, "` which is not a function.");
        }
        // Pack arguments.
        return { rocket::move(val.mut_function()), rocket::move(args) };
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_not(const G_boolean& rhs)
      {
        return !rhs;
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_and(const G_boolean& lhs, const G_boolean& rhs)
      {
        return lhs & rhs;
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_or(const G_boolean& lhs, const G_boolean& rhs)
      {
        return lhs | rhs;
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_xor(const G_boolean& lhs, const G_boolean& rhs)
      {
        return lhs ^ rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_neg(const G_integer& rhs)
      {
        if(rhs == INT64_MIN) {
          ASTERIA_THROW_RUNTIME_ERROR("The opposite of `", rhs, "` cannot be represented as an `integer`.");
        }
        return -rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_sqrt(const G_integer& rhs)
      {
        return std::sqrt(G_real(rhs));
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isinf(const G_integer& /*rhs*/)
      {
        return false;
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isnan(const G_integer& /*rhs*/)
      {
        return false;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_abs(const G_integer& rhs)
      {
        if(rhs == INT64_MIN) {
          ASTERIA_THROW_RUNTIME_ERROR("The absolute value of `", rhs, "` cannot be represented as an `integer`.");
        }
        return std::abs(rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_signb(const G_integer& rhs)
      {
        return rhs >> 63;
      }

    [[noreturn]] void do_throw_integral_overflow(const char* op, const G_integer& lhs, const G_integer& rhs)
      {
        ASTERIA_THROW_RUNTIME_ERROR("Integral ", op, " of `", lhs, "` and `", rhs, "` would result in overflow.");
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_add(const G_integer& lhs, const G_integer& rhs)
      {
        if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs)) {
          do_throw_integral_overflow("addition", lhs, rhs);
        }
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sub(const G_integer& lhs, const G_integer& rhs)
      {
        if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs)) {
          do_throw_integral_overflow("subtraction", lhs, rhs);
        }
        return lhs - rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_mul(const G_integer& lhs, const G_integer& rhs)
      {
        if((lhs == 0) || (rhs == 0)) {
          return 0;
        }
        if((lhs == 1) || (rhs == 1)) {
          return (lhs ^ rhs) ^ 1;
        }
        if((lhs == INT64_MIN) || (rhs == INT64_MIN)) {
          do_throw_integral_overflow("multiplication", lhs, rhs);
        }
        if((lhs == -1) || (rhs == -1)) {
          return (lhs ^ rhs) + 1;
        }
        // absolute lhs and signed rhs
        auto m = lhs >> 63;
        auto alhs = (lhs ^ m) - m;
        auto srhs = (rhs ^ m) - m;
        // `alhs` may only be positive here.
        if((srhs >= 0) ? (alhs > INT64_MAX / srhs) : (alhs > INT64_MIN / srhs)) {
          do_throw_integral_overflow("multiplication", lhs, rhs);
        }
        return alhs * srhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_div(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs == 0) {
          ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
        }
        if((lhs == INT64_MIN) && (rhs == -1)) {
          do_throw_integral_overflow("division", lhs, rhs);
        }
        return lhs / rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_mod(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs == 0) {
          ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
        }
        if((lhs == INT64_MIN) && (rhs == -1)) {
          do_throw_integral_overflow("division", lhs, rhs);
        }
        return lhs % rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sll(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return 0;
        }
        return G_integer(static_cast<uint64_t>(lhs) << rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_srl(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return 0;
        }
        return G_integer(static_cast<uint64_t>(lhs) >> rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sla(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(lhs == 0) {
          return 0;
        }
        if(rhs >= 64) {
          ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
        }
        auto bc = static_cast<int>(63 - rhs);
        auto mask_out = static_cast<uint64_t>(lhs) >> bc << bc;
        auto mask_sbt = static_cast<uint64_t>(lhs >> 63) << bc;
        if(mask_out != mask_sbt) {
          ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
        }
        return G_integer(static_cast<uint64_t>(lhs) << rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sra(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return lhs >> 63;
        }
        return lhs >> rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_not(const G_integer& rhs)
      {
        return ~rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_and(const G_integer& lhs, const G_integer& rhs)
      {
        return lhs & rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_or(const G_integer& lhs, const G_integer& rhs)
      {
        return lhs | rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_xor(const G_integer& lhs, const G_integer& rhs)
      {
        return lhs ^ rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_neg(const G_real& rhs)
      {
        return -rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_sqrt(const G_real& rhs)
      {
        return std::sqrt(rhs);
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isinf(const G_real& rhs)
      {
        return std::isinf(rhs);
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isnan(const G_real& rhs)
      {
        return std::isnan(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_abs(const G_real& rhs)
      {
        return std::fabs(rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_signb(const G_real& rhs)
      {
        return std::signbit(rhs) ? -1 : 0;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_round(const G_real& rhs)
      {
        return std::round(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_floor(const G_real& rhs)
      {
        return std::floor(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_ceil(const G_real& rhs)
      {
        return std::ceil(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_trunc(const G_real& rhs)
      {
        return std::trunc(rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_icast(const G_real& value)
      {
        if(!std::islessequal(INT64_MIN, value) || !std::islessequal(value, INT64_MAX)) {
          ASTERIA_THROW_RUNTIME_ERROR("The `real` value `", value, "` cannot be represented as an `integer`.");
        }
        return G_integer(value);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_iround(const G_real& rhs)
      {
        return do_icast(std::round(rhs));
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_ifloor(const G_real& rhs)
      {
        return do_icast(std::floor(rhs));
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_iceil(const G_real& rhs)
      {
        return do_icast(std::ceil(rhs));
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_itrunc(const G_real& rhs)
      {
        return do_icast(std::trunc(rhs));
      }

    ROCKET_PURE_FUNCTION G_real do_operator_add(const G_real& lhs, const G_real& rhs)
      {
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_sub(const G_real& lhs, const G_real& rhs)
      {
        return lhs - rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_mul(const G_real& lhs, const G_real& rhs)
      {
        return lhs * rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_div(const G_real& lhs, const G_real& rhs)
      {
        return lhs / rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_mod(const G_real& lhs, const G_real& rhs)
      {
        return std::fmod(lhs, rhs);
      }

    ROCKET_PURE_FUNCTION G_string do_operator_add(const G_string& lhs, const G_string& rhs)
      {
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_mul(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String duplication count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        auto nchars = lhs.size();
        if((nchars == 0) || (rhs == 0)) {
          return res;
        }
        if(nchars > res.max_size() / static_cast<uint64_t>(rhs)) {
          ASTERIA_THROW_RUNTIME_ERROR("Duplication of `", lhs, "` up to `", rhs, "` times would result in an overlong string that cannot be allocated.");
        }
        auto times = static_cast<size_t>(rhs);
        if(nchars == 1) {
          // Fast fill.
          res.assign(times, lhs.front());
          return res;
        }
        // Reserve space for the result string.
        auto ptr = res.assign(nchars * times, '*').mut_data();
        // Copy the source string once.
        std::memcpy(ptr, lhs.data(), nchars);
        // Append the result string to itself, doubling its length, until more than half of the result string has been populated.
        while(nchars <= res.size() / 2) {
          std::memcpy(ptr + nchars, ptr, nchars);
          nchars *= 2;
        }
        // Copy remaining characters, if any.
        if(nchars < res.size()) {
          std::memcpy(ptr + nchars, ptr, res.size() - nchars);
        }
        return res;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_mul(const G_integer& lhs, const G_string& rhs)
      {
        return do_operator_mul(rhs, lhs);
      }

    ROCKET_PURE_FUNCTION G_string do_operator_sll(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        // Reserve space for the result string.
        auto ptr = rocket::unfancy(res.insert(res.begin(), lhs.size(), ' '));
        if(static_cast<uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<size_t>(rhs);
        // Copy the substring in the right.
        std::memcpy(ptr, lhs.data() + count, lhs.size() - count);
        return res;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_srl(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        // Reserve space for the result string.
        auto ptr = rocket::unfancy(res.insert(res.begin(), lhs.size(), ' '));
        if(static_cast<uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<size_t>(rhs);
        // Copy the substring in the left.
        std::memcpy(ptr + count, lhs.data(), lhs.size() - count);
        return res;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_sla(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        if(static_cast<uint64_t>(rhs) >= res.max_size() - lhs.size()) {
          ASTERIA_THROW_RUNTIME_ERROR("Shifting `", lhs, "` to the left by `", rhs, "` bytes would result in an overlong string that cannot be allocated.");
        }
        auto count = static_cast<size_t>(rhs);
        // Append spaces in the right and return the result.
        res.assign(G_string::shallow_type(lhs));
        res.append(count, ' ');
        return res;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_sra(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        if(static_cast<uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<size_t>(rhs);
        // Return the substring in the left.
        res.append(lhs.data(), lhs.size() - count);
        return res;
      }

    }

AIR_Status AIR_Node::execute(Executive_Context& ctx) const
  {
    switch(this->index()) {
    case index_clear_stack:
      {
        // Clear the stack.
        ctx.stack().clear_references();
        return air_status_next;
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
        return air_status_next;
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
        return air_status_next;
      }
    case index_if_statement:
      {
        const auto& altr = this->m_stor.as<index_if_statement>();
        // Pick a branch basing on the condition.
        if(ctx.stack().get_top_reference().read().test() != altr.negative) {
          return do_execute_block(altr.code_true, ctx);
        }
        else {
          return do_execute_block(altr.code_false, ctx);
        }
      }
    case index_switch_statement:
      {
        const auto& altr = this->m_stor.as<index_switch_statement>();
        // Read the value of the control expression.
        auto value = ctx.stack().get_top_reference().read();
        // Find a target clause.
        // This is different from a C `switch` statement where `case` labels must have constant operands.
        size_t tpos = SIZE_MAX;
        for(size_t i = 0; i != altr.clauses.size(); ++i) {
          const auto& code_case = altr.clauses[i].first;
          if(code_case.empty()) {
            // This is a `default` label.
            if(tpos != SIZE_MAX) {
              ASTERIA_THROW_RUNTIME_ERROR("Multiple `default` clauses have been found in this `switch` statement.");
            }
            tpos = i;
            continue;
          }
          // This is a `case` label.
          // Evaluate the operand and check whether it equals `value`.
          if(value.compare(do_evaluate(ctx, code_case)) == Value::compare_equal) {
            tpos = i;
            break;
          }
        }
        if(tpos != SIZE_MAX) {
          // Jump to the clause denoted by `tpos`.
          // Note that all clauses share the same context.
          Executive_Context ctx_body(1, ctx);
          // Skip clauses that precede `tpos`.
          for(size_t i = 0; i != tpos; ++i) {
            const auto& names = altr.clauses[i].second.second;
            rocket::for_each(names, [&](const phsh_string& name) { ctx_body.open_named_reference(name) = Reference_Root::S_null();  });
          }
          // Execute all clauses from `tpos`.
          for(size_t i = tpos; i != altr.clauses.size(); ++i) {
            const auto& code_clause = altr.clauses[i].second.first;
            auto status = do_execute_statement_list(ctx_body, code_clause);
            if(rocket::is_any_of(status, { air_status_break_unspec, air_status_break_switch })) {
              break;
            }
            if(status != air_status_next) {
              // Forward any status codes unexpected to the caller.
              return status;
            }
          }
        }
        return air_status_next;
      }
    case index_do_while_statement:
      {
        const auto& altr = this->m_stor.as<index_do_while_statement>();
        do {
          // Execute the body.
          auto status = do_execute_block(altr.code_body, ctx);
          if(rocket::is_any_of(status, { air_status_break_unspec, air_status_break_while })) {
            break;
          }
          if(rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_while })) {
            // Forward any status codes unexpected to the caller.
            return status;
          }
          // Check the condition after each iteration.
        } while(do_evaluate(ctx, altr.code_cond).test() != altr.negative);
        return air_status_next;
      }
    case index_while_statement:
      {
        const auto& altr = this->m_stor.as<index_while_statement>();
        // Check the condition before every iteration.
        while(do_evaluate(ctx, altr.code_cond).test() != altr.negative) {
          // Execute the body.
          auto status = do_execute_block(altr.code_body, ctx);
          if(rocket::is_any_of(status, { air_status_break_unspec, air_status_break_while })) {
            break;
          }
          if(rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_while })) {
            // Forward any status codes unexpected to the caller.
            return status;
          }
        }
        return air_status_next;
      }
    case index_for_each_statement:
      {
        const auto& altr = this->m_stor.as<index_for_each_statement>();
        // Note that the key and value references outlasts every iteration, so we have to create an outer contexts here.
        Executive_Context ctx_for(1, ctx);
        // Create a variable for the key.
        auto key = ctx_for.global().create_variable();
        key->reset(Source_Location(rocket::sref("<ranged for>"), 0), G_null(), true);
        Reference_Root::S_variable xref_key = { key };
        ctx_for.open_named_reference(altr.name_key) = rocket::move(xref_key);
        // Create the mapped reference.
        auto& mapped = ctx_for.open_named_reference(altr.name_mapped);
        mapped = Reference_Root::S_null();
        // Clear the stack.
        ctx_for.stack().clear_references();
        // Evaluate the range initializer.
        ROCKET_ASSERT(!altr.code_init.empty());
        rocket::for_each(altr.code_init, [&](const AIR_Node& node) { node.execute(ctx_for);  });
        // Set the range up.
        mapped = rocket::move(ctx_for.stack().open_top_reference());
        auto range = mapped.read();
        // The range value has been saved. This ensures we are immune to dangling pointers if the loop body attempts to modify it.
        // Also be advised that the mapped parameter is a reference rather than a value.
        if(range.is_array()) {
          const auto& array = range.as_array();
          // The key is the subscript of an element of the array.
          for(size_t i = 0; i != array.size(); ++i) {
            // Set up the key and mapped arguments.
            key->reset(G_integer(i), true);
            Reference_Modifier::S_array_index xmod = { static_cast<int64_t>(i) };
            mapped.zoom_in(rocket::move(xmod));
            // Execute the loop body.
            auto status = do_execute_block(altr.code_body, ctx_for);
            if(rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for })) {
              break;
            }
            if(rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_for })) {
              // Forward any status codes unexpected to the caller.
              return status;
            }
            // Restore the mapped reference.
            mapped.zoom_out();
          }
          return air_status_next;
        }
        else if(range.is_object()) {
          const auto& object = range.as_object();
          // The key is a string.
          for(auto q = object.begin(); q != object.end(); ++q) {
            // Set up the key and mapped arguments.
            key->reset(G_string(q->first), true);
            Reference_Modifier::S_object_key xmod = { q->first };
            mapped.zoom_in(rocket::move(xmod));
            // Execute the loop body.
            auto status = do_execute_block(altr.code_body, ctx_for);
            if(rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for })) {
              break;
            }
            if(rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_for })) {
              // Forward any status codes unexpected to the caller.
              return status;
            }
            // Restore the mapped reference.
            mapped.zoom_out();
          }
          return air_status_next;
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("The `for each` statement does not accept a range of type `", range.gtype_name(), "`.");
        }
      }
    case index_for_statement:
      {
        const auto& altr = this->m_stor.as<index_for_statement>();
        // Note that names declared in the first segment of a for-statement outlasts every iteration, so we have to create an outer contexts here.
        Executive_Context ctx_for(1, ctx);
        // Execute the loop initializer.
        // XXX: Techinically it should only be a definition or an expression statement.
        auto status = do_execute_statement_list(ctx_for, altr.code_init);
        if(status != air_status_next) {
          // Forward any status codes unexpected to the caller.
          return status;
        }
        // If the condition is empty, the loop is infinite.
        while(altr.code_cond.empty() || do_evaluate(ctx_for, altr.code_cond).test()) {
          // Execute the body.
          status = do_execute_block(altr.code_body, ctx_for);
          if(rocket::is_any_of(status, { air_status_break_unspec, air_status_break_for })) {
            break;
          }
          if(rocket::is_none_of(status, { air_status_next, air_status_continue_unspec, air_status_continue_for })) {
            // Forward any status codes unexpected to the caller.
            return status;
          }
          // Execute the loop increment.
          do_evaluate(ctx_for, altr.code_step);
        }
        return air_status_next;
      }
    case index_try_statement:
      {
        const auto& altr = this->m_stor.as<index_try_statement>();
        // This is almost identical to JavaScript.
        try {
          // Execute the `try` clause. If no exception is thrown, this will have little overhead.
          return do_execute_block(altr.code_try, ctx);
        }
        catch(Exception& except) {
          // Reuse the exception object. Don't bother allocating a new one.
          except.push_frame_catch(altr.sloc);
          ASTERIA_DEBUG_LOG("Caught `Asteria::Exception`: ", except);
          return do_execute_catch(altr.code_catch, altr.name_except, except, ctx);
        }
        catch(const std::exception& stdex) {
          // Translate the exception.
          Exception except(stdex);
          except.push_frame_catch(altr.sloc);
          ASTERIA_DEBUG_LOG("Translated `std::exception`: ", except);
          return do_execute_catch(altr.code_catch, altr.name_except, except, ctx);
        }
      }
    case index_throw_statement:
      {
        const auto& altr = this->m_stor.as<index_throw_statement>();
        // Read the value to throw.
        // Note that the operand must not have been empty for this code.
        auto value = ctx.stack().get_top_reference().read();
        // Unpack the nested exception, if any.
        opt<Exception> qnested;
        // Rethrow the current exception to get its effective type.
        auto eptr = std::current_exception();
        if(eptr) {
          try {
            std::rethrow_exception(eptr);
          }
          catch(Exception& except) {
            // Modify the exception in place. Don't bother allocating a new one.
            except.push_frame_throw(altr.sloc, rocket::move(value));
            throw;
          }
          catch(const std::exception& stdex) {
            // Translate the exception.
            qnested.emplace(stdex);
          }
        }
        if(!qnested) {
          // If no nested exception exists, construct a fresh one.
          qnested.emplace(altr.sloc, rocket::move(value));
        }
        throw *qnested;
      }
    case index_assert_statement:
      {
        const auto& altr = this->m_stor.as<index_assert_statement>();
        // Read the value to check.
        if(ROCKET_EXPECT(ctx.stack().get_top_reference().read().test() != altr.negative)) {
          // The assertion has succeeded.
          return air_status_next;
        }
        // The assertion has failed.
        cow_osstream fmtss;
        fmtss.imbue(std::locale::classic());
        fmtss << "Assertion failed at \'" << altr.sloc << '\'';
        // Append the message if one is provided.
        if(!altr.msg.empty())
          fmtss << ": " << altr.msg;
        else
          fmtss << '!';
        // Throw a `Runtime_Error`.
        throw_runtime_error(__func__, fmtss.extract_string());
      }
    case index_simple_status:
      {
        const auto& altr = this->m_stor.as<index_simple_status>();
        // Just return the status.
        return altr.status;
      }
    case index_return_by_value:
      {
        // The result will have been pushed onto the top.
        auto& ref = ctx.stack().open_top_reference();
        // Convert the result to an rvalue.
        // TCO wrappers are forwarded as is.
        if(ROCKET_UNEXPECT(ref.is_lvalue())) {
          ref.convert_to_rvalue();
        }
        return air_status_return;
      }
    case index_push_literal:
      {
        const auto& altr = this->m_stor.as<index_push_literal>();
        // Push a constant.
        Reference_Root::S_constant xref = { altr.val };
        ctx.stack().push_reference(rocket::move(xref));
        return air_status_next;
      }
    case index_push_global_reference:
      {
        const auto& altr = this->m_stor.as<index_push_global_reference>();
        // Look for the name in the global context.
        auto qref = ctx.global().get_named_reference_opt(altr.name);
        if(!qref) {
          ASTERIA_THROW_RUNTIME_ERROR("The identifier `", altr.name, "` has not been declared yet.");
        }
        // Push a copy of it.
        ctx.stack().push_reference(*qref);
        return air_status_next;
      }
    case index_push_local_reference:
      {
        const auto& altr = this->m_stor.as<index_push_local_reference>();
        // Get the context.
        const Executive_Context* qctx = std::addressof(ctx);
        rocket::ranged_for(size_t(0), altr.depth, [&](size_t) { qctx = qctx->get_parent_opt();  });
        ROCKET_ASSERT(qctx);
        // Look for the name in the target context.
        auto qref = qctx->get_named_reference_opt(altr.name);
        if(!qref) {
          ASTERIA_THROW_RUNTIME_ERROR("The identifier `", altr.name, "` has not been declared yet.");
        }
        // Push a copy of it.
        ctx.stack().push_reference(*qref);
        return air_status_next;
      }
    case index_push_bound_reference:
      {
        const auto& altr = this->m_stor.as<index_push_bound_reference>();
        // Push a copy of the bound reference.
        ctx.stack().push_reference(altr.bref);
        return air_status_next;
      }
    case index_define_function:
      {
        const auto& altr = this->m_stor.as<index_define_function>();
        // Instantiate the function.
        auto qtarget = rocket::make_refcnt<Instantiated_Function>(altr.options, altr.sloc, altr.func, std::addressof(ctx), altr.params, altr.body);
        ASTERIA_DEBUG_LOG("New function: ", *qtarget);
        // Push the function as a temporary.
        Reference_Root::S_temporary xref = { G_function(rocket::move(qtarget)) };
        ctx.stack().push_reference(rocket::move(xref));
        return air_status_next;
      }
    case index_branch_expression:
      {
        const auto& altr = this->m_stor.as<index_branch_expression>();
        // Pick a branch basing on the condition.
        // If the target branch is empty, leave the condition on the stack.
        if(ctx.stack().get_top_reference().read().test()) {
          return do_evaluate_branch(altr.code_true, ctx, altr.assign);
        }
        else {
          return do_evaluate_branch(altr.code_false, ctx, altr.assign);
        }
      }
    case index_coalescence:
      {
        const auto& altr = this->m_stor.as<index_coalescence>();
        // Pick a branch basing on the condition.
        // If the target branch is empty, leave the condition on the stack.
        if(ctx.stack().get_top_reference().read().is_null()) {
          return do_evaluate_branch(altr.code_null, ctx, altr.assign);
        }
        else {
          return air_status_next;
        }
      }
    case index_function_call_tail:
      {
        const auto& altr = this->m_stor.as<index_function_call_tail>();
        // Prepare arguments.
        auto pargs = do_unpack_function_call(ctx, altr.args_by_refs);
        const auto& func = ctx.zvarg()->get_function_signature();
        auto& self = ctx.stack().open_top_reference();
        self.zoom_out();
        // Pack arguments.
        pargs.args.emplace_back(rocket::move(self));
        // Create a TCO wrapper. The caller shall unwrap the proxy reference when appropriate.
        Reference_Root::S_tail_call xref = { altr.sloc, func, altr.tco_aware, rocket::move(pargs.target), rocket::move(pargs.args) };
        self = rocket::move(xref);
        return air_status_return;
      }
    case index_function_call_plain:
      {
        const auto& altr = this->m_stor.as<index_function_call_plain>();
        // Prepare arguments.
        auto pargs = do_unpack_function_call(ctx, altr.args_by_refs);
        const auto& func = ctx.zvarg()->get_function_signature();
        auto& self = ctx.stack().open_top_reference();
        self.zoom_out();
        try {
          ASTERIA_DEBUG_LOG("Initiating function call at \'", altr.sloc, "\' inside `", func, "`: target = ", pargs.target);
          // Call the function now.
          pargs.target->invoke(self, ctx.global(), rocket::move(pargs.args));
          self.finish_call(ctx.global());
          // The result will have been stored into `self`.
          ASTERIA_DEBUG_LOG("Returned from function call at \'", altr.sloc, "\' inside `", func, "`: target = ", pargs.target);
          return air_status_next;
        }
        catch(Exception& except) {
          ASTERIA_DEBUG_LOG("Caught `Asteria::Exception` thrown inside function call at \'", altr.sloc, "\' inside `", func, "`: ", except.get_value());
          // Append the current frame and rethrow the exception.
          except.push_frame_func(altr.sloc, func);
          throw;
        }
        catch(const std::exception& stdex) {
          ASTERIA_DEBUG_LOG("Caught `std::exception` thrown inside function call at \'", altr.sloc, "\' inside `", func, "`: ", stdex.what());
          // Translate the exception, append the current frame, and throw the new exception.
          Exception except(stdex);
          except.push_frame_func(altr.sloc, func);
          throw except;
        }
      }
    case index_member_access:
      {
        const auto& altr = this->m_stor.as<index_member_access>();
        // Append a modifier to the reference at the top.
        Reference_Modifier::S_object_key xmod = { altr.name };
        ctx.stack().open_top_reference().zoom_in(rocket::move(xmod));
        return air_status_next;
      }
    case index_push_unnamed_array:
      {
        const auto& altr = this->m_stor.as<index_push_unnamed_array>();
        // Pop some elements from the stack to create an array.
        G_array array;
        array.resize(altr.nelems);
        for(auto it = array.mut_rbegin(); it != array.rend(); ++it) {
          *it = ctx.stack().get_top_reference().read();
          ctx.stack().pop_reference();
        }
        // Push the array as a temporary.
        Reference_Root::S_temporary xref = { rocket::move(array) };
        ctx.stack().push_reference(rocket::move(xref));
        return air_status_next;
      }
    case index_push_unnamed_object:
      {
        const auto& altr = this->m_stor.as<index_push_unnamed_object>();
        // Pop some elements from the stack to create an object.
        G_object object;
        object.reserve(altr.keys.size());
        for(auto it = altr.keys.rbegin(); it != altr.keys.rend(); ++it) {
          object.insert_or_assign(*it, ctx.stack().get_top_reference().read());
          ctx.stack().pop_reference();
        }
        // Push the object as a temporary.
        Reference_Root::S_temporary xref = { rocket::move(object) };
        ctx.stack().push_reference(rocket::move(xref));
        return air_status_next;
      }
    case index_apply_xop_inc_post:
      {
        // This operator is unary.
        auto& lhs = ctx.stack().get_top_reference().open();
        // Increment the operand and return the old value. `altr.assign` is ignored.
        if(lhs.is_integer()) {
          auto& reg = lhs.mut_integer();
          ctx.stack().set_temporary_reference(false, rocket::move(lhs));
          reg = do_operator_add(reg, G_integer(1));
        }
        else if(lhs.is_real()) {
          auto& reg = lhs.mut_real();
          ctx.stack().set_temporary_reference(false, rocket::move(lhs));
          reg = do_operator_add(reg, G_real(1));
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Postfix increment is not defined for `", lhs, "`.");
        }
        return air_status_next;
      }
    case index_apply_xop_dec_post:
      {
        // This operator is unary.
        auto& lhs = ctx.stack().get_top_reference().open();
        // Decrement the operand and return the old value. `altr.assign` is ignored.
        if(lhs.is_integer()) {
          auto& reg = lhs.mut_integer();
          ctx.stack().set_temporary_reference(false, rocket::move(lhs));
          reg = do_operator_sub(reg, G_integer(1));
        }
        else if(lhs.is_real()) {
          auto& reg = lhs.mut_real();
          ctx.stack().set_temporary_reference(false, rocket::move(lhs));
          reg = do_operator_sub(reg, G_real(1));
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Postfix decrement is not defined for `", lhs, "`.");
        }
        return air_status_next;
      }
    case index_apply_xop_subscr:
      {
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        auto& lref = ctx.stack().open_top_reference();
        // Append a reference modifier. `altr.assign` is ignored.
        if(rhs.is_integer()) {
          auto& reg = rhs.mut_integer();
          Reference_Modifier::S_array_index xmod = { rocket::move(reg) };
          lref.zoom_in(rocket::move(xmod));
        }
        else if(rhs.is_string()) {
          auto& reg = rhs.mut_string();
          Reference_Modifier::S_object_key xmod = { rocket::move(reg) };
          lref.zoom_in(rocket::move(xmod));
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("The value `", rhs, "` cannot be used as a subscript.");
        }
        return air_status_next;
      }
    case index_apply_xop_pos:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_pos>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Copy the operand to create a temporary value, then return it.
        // N.B. This is one of the few operators that work on all types.
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_neg:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_neg>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Get the opposite of the operand as a temporary value, then return it.
        if(rhs.is_integer()) {
          auto& reg = rhs.mut_integer();
          reg = do_operator_neg(reg);
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.mut_real();
          reg = do_operator_neg(reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix negation is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_notb:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_notb>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Perform bitwise NOT operation on the operand to create a temporary value, then return it.
        if(rhs.is_boolean()) {
          auto& reg = rhs.mut_boolean();
          reg = do_operator_not(reg);
        }
        else if(rhs.is_integer()) {
          auto& reg = rhs.mut_integer();
          reg = do_operator_not(reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix bitwise NOT is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_notl:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_notl>();
        // This operator is unary.
        const auto& rhs = ctx.stack().get_top_reference().read();
        // Perform logical NOT operation on the operand to create a temporary value, then return it.
        // N.B. This is one of the few operators that work on all types.
        ctx.stack().set_temporary_reference(altr.assign, do_operator_not(rhs.test()));
        return air_status_next;
      }
    case index_apply_xop_inc:
      {
        // This operator is unary.
        auto& rhs = ctx.stack().get_top_reference().open();
        // Increment the operand and return it. `altr.assign` is ignored.
        if(rhs.is_integer()) {
          auto& reg = rhs.mut_integer();
          reg = do_operator_add(reg, G_integer(1));
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.mut_real();
          reg = do_operator_add(reg, G_real(1));
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix increment is not defined for `", rhs, "`.");
        }
        return air_status_next;
      }
    case index_apply_xop_dec:
      {
        // This operator is unary.
        auto& rhs = ctx.stack().get_top_reference().open();
        // Decrement the operand and return it. `altr.assign` is ignored.
        if(rhs.is_integer()) {
          auto& reg = rhs.mut_integer();
          reg = do_operator_sub(reg, G_integer(1));
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.mut_real();
          reg = do_operator_sub(reg, G_real(1));
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix increment is not defined for `", rhs, "`.");
        }
        return air_status_next;
      }
    case index_apply_xop_unset:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_unset>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().unset();
        // Unset the reference and return the old value.
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_lengthof:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_lengthof>();
        // This operator is unary.
        const auto& rhs = ctx.stack().get_top_reference().read();
        // Return the number of elements in the operand.
        size_t nelems;
        if(rhs.is_null()) {
          nelems = 0;
        }
        else if(rhs.is_string()) {
          nelems = rhs.as_string().size();
        }
        else if(rhs.is_array()) {
          nelems = rhs.as_array().size();
        }
        else if(rhs.is_object()) {
          nelems = rhs.as_object().size();
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `lengthof` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, G_integer(nelems));
        return air_status_next;
      }
    case index_apply_xop_typeof:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_typeof>();
        // This operator is unary.
        const auto& rhs = ctx.stack().get_top_reference().read();
        // Return the type name of the operand.
        // N.B. This is one of the few operators that work on all types.
        ctx.stack().set_temporary_reference(altr.assign, G_string(rocket::sref(rhs.gtype_name())));
        return air_status_next;
      }
    case index_apply_xop_sqrt:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_sqrt>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Get the square root of the operand as a temporary value, then return it.
        if(rhs.is_integer()) {
          // Note that `rhs` does not have type `G_real`, thus this branch can't be optimized.
          rhs = do_operator_sqrt(rhs.as_integer());
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.mut_real();
          reg = do_operator_sqrt(reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__sqrt` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_isnan:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_isnan>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Check whether the operand is a NaN, store the result in a temporary value, then return it.
        if(rhs.is_integer()) {
          // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
          rhs = do_operator_isnan(rhs.as_integer());
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
          rhs = do_operator_isnan(rhs.as_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__isnan` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_isinf:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_isinf>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Check whether the operand is an infinity, store the result in a temporary value, then return it.
        if(rhs.is_integer()) {
          // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
          rhs = do_operator_isinf(rhs.as_integer());
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
          rhs = do_operator_isinf(rhs.as_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__isinf` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_abs:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_abs>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Get the absolute value of the operand as a temporary value, then return it.
        if(rhs.is_integer()) {
          auto& reg = rhs.mut_integer();
          reg = do_operator_abs(reg);
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.mut_real();
          reg = do_operator_abs(reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__abs` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_signb:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_signb>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Get the sign bit of the operand as a temporary value, then return it.
        if(rhs.is_integer()) {
          auto& reg = rhs.mut_integer();
          reg = do_operator_signb(reg);
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
          rhs = do_operator_signb(rhs.as_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__signb` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_round:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_round>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Round the operand to the nearest integer as a temporary value, then return it.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.mut_real();
          reg = do_operator_round(reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__round` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_floor:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_floor>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Round the operand towards negative infinity as a temporary value, then return it.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.mut_real();
          reg = do_operator_floor(reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__floor` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_ceil:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_ceil>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Round the operand towards negative infinity as a temporary value, then return it.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.mut_real();
          reg = do_operator_ceil(reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__ceil` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_trunc:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_trunc>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Round the operand towards negative infinity as a temporary value, then return it.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          auto& reg = rhs.mut_real();
          reg = do_operator_trunc(reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__trunc` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_iround:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_iround>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Round the operand to the nearest integer as a temporary value, then return it as an `integer`.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
          rhs = do_operator_iround(rhs.as_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__iround` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_ifloor:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_ifloor>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Round the operand towards negative infinity as a temporary value, then return it as an `integer`.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
          rhs = do_operator_ifloor(rhs.as_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__ifloor` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_iceil:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_iceil>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Round the operand towards negative infinity as a temporary value, then return it as an `integer`.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
          rhs = do_operator_iceil(rhs.as_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__iceil` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_itrunc:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_itrunc>();
        // This operator is unary.
        auto rhs = ctx.stack().get_top_reference().read();
        // Round the operand towards negative infinity as a temporary value, then return it as an `integer`.
        if(rhs.is_integer()) {
          // No conversion is required.
          // Return `rhs` as is.
        }
        else if(rhs.is_real()) {
          // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
          rhs = do_operator_itrunc(rhs.as_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Prefix `__itrunc` is not defined for `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_cmp_xeq:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_cmp_xeq>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        rhs = G_boolean((comp == Value::compare_equal) != altr.negative);
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_cmp_xrel:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_cmp_xrel>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        if(comp == Value::compare_unordered) {
          ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs, "` and `", rhs, "` are unordered.");
        }
        rhs = G_boolean((comp == altr.expect) != altr.negative);
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_cmp_3way:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_cmp_3way>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        if(comp == Value::compare_unordered) {
          rhs = G_string(rocket::sref("<unordered>"));
        }
        else if(comp == Value::compare_less) {
          rhs = G_integer(-1);
        }
        else if(comp == Value::compare_greater) {
          rhs = G_integer(+1);
        }
        else {
          rhs = G_integer(0);
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_add:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_add>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_boolean() && rhs.is_boolean()) {
          // For the `boolean` type, return the logical OR'd result of both operands.
          auto& reg = rhs.mut_boolean();
          reg = do_operator_or(lhs.as_boolean(), reg);
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` and `real` types, return the sum of both operands.
          auto& reg = rhs.mut_integer();
          reg = do_operator_add(lhs.as_integer(), reg);
        }
        else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
          // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
          rhs = do_operator_add(lhs.convert_to_real(), rhs.convert_to_real());
        }
        else if(lhs.is_string() && rhs.is_string()) {
          // For the `string` type, concatenate the operands in lexical order to create a new string, then return it.
          auto& reg = rhs.mut_string();
          reg = do_operator_add(lhs.as_string(), reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Infix addition is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_sub:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_sub>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_boolean() && rhs.is_boolean()) {
          // For the `boolean` type, return the logical XOR'd result of both operands.
          auto& reg = rhs.mut_boolean();
          reg = do_operator_xor(lhs.as_boolean(), reg);
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` and `real` types, return the difference of both operands.
          auto& reg = rhs.mut_integer();
          reg = do_operator_sub(lhs.as_integer(), reg);
        }
        else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
          // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
          rhs = do_operator_sub(lhs.convert_to_real(), rhs.convert_to_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Infix subtraction is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_mul:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_mul>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_boolean() && rhs.is_boolean()) {
          // For the `boolean` type, return the logical AND'd result of both operands.
          auto& reg = rhs.mut_boolean();
          reg = do_operator_and(lhs.as_boolean(), reg);
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` and `real` types, return the product of both operands.
          auto& reg = rhs.mut_integer();
          reg = do_operator_mul(lhs.as_integer(), reg);
        }
        else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
          // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
          rhs = do_operator_mul(lhs.convert_to_real(), rhs.convert_to_real());
        }
        else if(lhs.is_string() && rhs.is_integer()) {
          // If either operand has type `string` and the other has type `integer`, duplicate the string up to the specified number of times and return the result.
          // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
          rhs = do_operator_mul(lhs.as_string(), rhs.as_integer());
        }
        else if(lhs.is_integer() && rhs.is_string()) {
          auto& reg = rhs.mut_string();
          reg = do_operator_mul(lhs.as_integer(), reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Infix multiplication is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_div:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_div>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` and `real` types, return the quotient of both operands.
          auto& reg = rhs.mut_integer();
          reg = do_operator_div(lhs.as_integer(), reg);
        }
        else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
          // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
          rhs = do_operator_div(lhs.convert_to_real(), rhs.convert_to_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Infix division is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_mod:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_mod>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` and `real` types, return the remainder of both operands.
          auto& reg = rhs.mut_integer();
          reg = do_operator_mod(lhs.as_integer(), reg);
        }
        else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
          // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
          rhs = do_operator_mod(lhs.convert_to_real(), rhs.convert_to_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Infix modulo operation is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_sll:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_sll>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_integer() && rhs.is_integer()) {
          // If the LHS operand has type `integer`, shift the LHS operand to the left by the number of bits specified by the RHS operand.
          // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
          auto& reg = rhs.mut_integer();
          reg = do_operator_sll(lhs.as_integer(), reg);
        }
        else if(lhs.is_string() && rhs.is_integer()) {
          // If the LHS operand has type `string`, fill space characters in the right and discard characters from the left.
          // The number of bytes in the LHS operand will be preserved.
          // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
          rhs = do_operator_sll(lhs.as_string(), rhs.as_integer());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Infix logical shift to the left is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_srl:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_srl>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_integer() && rhs.is_integer()) {
          // If the LHS operand has type `integer`, shift the LHS operand to the right by the number of bits specified by the RHS operand.
          // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
          auto& reg = rhs.mut_integer();
          reg = do_operator_srl(lhs.as_integer(), reg);
        }
        else if(lhs.is_string() && rhs.is_integer()) {
          // If the LHS operand has type `string`, fill space characters in the left and discard characters from the right.
          // The number of bytes in the LHS operand will be preserved.
          // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
          rhs = do_operator_srl(lhs.as_string(), rhs.as_integer());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Infix logical shift to the right is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_sla:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_sla>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_integer() && rhs.is_integer()) {
          // If the LHS operand is of type `integer`, shift the LHS operand to the left by the number of bits specified by the RHS operand.
          // Bits shifted out that are equal to the sign bit are discarded. Bits shifted in are filled with zeroes.
          // If any bits that are different from the sign bit would be shifted out, an exception is thrown.
          auto& reg = rhs.mut_integer();
          reg = do_operator_sla(lhs.as_integer(), reg);
        }
        else if(lhs.is_string() && rhs.is_integer()) {
          // If the LHS operand has type `string`, fill space characters in the right.
          // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
          rhs = do_operator_sla(lhs.as_string(), rhs.as_integer());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Infix arithmetic shift to the left is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_sra:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_sra>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_integer() && rhs.is_integer()) {
          // If the LHS operand is of type `integer`, shift the LHS operand to the right by the number of bits specified by the RHS operand.
          // Bits shifted out are discarded. Bits shifted in are filled with the sign bit.
          auto& reg = rhs.mut_integer();
          reg = do_operator_sra(lhs.as_integer(), reg);
        }
        else if(lhs.is_string() && rhs.is_integer()) {
          // If the LHS operand has type `string`, discard characters from the right.
          // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
          rhs = do_operator_sra(lhs.as_string(), rhs.as_integer());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Infix arithmetic shift to the right is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_andb:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_andb>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_boolean() && rhs.is_boolean()) {
          // For the `boolean` type, return the logical AND'd result of both operands.
          auto& reg = rhs.mut_boolean();
          reg = do_operator_and(lhs.as_boolean(), reg);
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` type, return bitwise AND'd result of both operands.
          auto& reg = rhs.mut_integer();
          reg = do_operator_and(lhs.as_integer(), reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Infix bitwise AND is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_orb:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_orb>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_boolean() && rhs.is_boolean()) {
          // For the `boolean` type, return the logical OR'd result of both operands.
          auto& reg = rhs.mut_boolean();
          reg = do_operator_or(lhs.as_boolean(), reg);
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` type, return bitwise OR'd result of both operands.
          auto& reg = rhs.mut_integer();
          reg = do_operator_or(lhs.as_integer(), reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Infix bitwise OR is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_xorb:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_xorb>();
        // This operator is binary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_boolean() && rhs.is_boolean()) {
          // For the `boolean` type, return the logical XOR'd result of both operands.
          auto& reg = rhs.mut_boolean();
          reg = do_operator_xor(lhs.as_boolean(), reg);
        }
        else if(lhs.is_integer() && rhs.is_integer()) {
          // For the `integer` type, return bitwise XOR'd result of both operands.
          auto& reg = rhs.mut_integer();
          reg = do_operator_xor(lhs.as_integer(), reg);
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Infix bitwise XOR is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_assign:
      {
        // Pop the RHS operand.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        // Copy the value to the LHS operand which is write-only. `altr.assign` is ignored.
        ctx.stack().set_temporary_reference(true, rocket::move(rhs));
        return air_status_next;
      }
    case index_apply_xop_fma:
      {
        const auto& altr = this->m_stor.as<index_apply_xop_fma>();
        // This operator is ternary.
        auto rhs = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        auto mid = ctx.stack().get_top_reference().read();
        ctx.stack().pop_reference();
        const auto& lhs = ctx.stack().get_top_reference().read();
        if(lhs.is_convertible_to_real() && mid.is_convertible_to_real() && rhs.is_convertible_to_real()) {
          // Calculate the fused multiply-add result of the operands.
          // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
          rhs = std::fma(lhs.convert_to_real(), mid.convert_to_real(), rhs.convert_to_real());
        }
        else {
          ASTERIA_THROW_RUNTIME_ERROR("Fused multiply-add is not defined for `", lhs, "` and `", rhs, "`.");
        }
        ctx.stack().set_temporary_reference(altr.assign, rocket::move(rhs));
        return air_status_next;
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
        altr.bref.enumerate_variables(callback);
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
