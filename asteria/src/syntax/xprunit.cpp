// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "xprunit.hpp"
#include "statement.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/evaluation_stack.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/executive_context.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../runtime/traceable_exception.hpp"
#include "../utilities.hpp"

namespace Asteria {

const char * Xprunit::get_operator_name(Xprunit::Xop xop) noexcept
  {
    switch(xop) {
    case xop_postfix_inc:
      {
        return "postfix increment";
      }
    case xop_postfix_dec:
      {
        return "postfix decrement";
      }
    case xop_postfix_at:
      {
        return "postfix subscript";
      }
    case xop_prefix_pos:
      {
        return "unary promotion";
      }
    case xop_prefix_neg:
      {
        return "unary negation";
      }
    case xop_prefix_notb:
      {
        return "bitwise not";
      }
    case xop_prefix_notl:
      {
        return "logical not";
      }
    case xop_prefix_inc:
      {
        return "prefix increment";
      }
    case xop_prefix_dec:
      {
        return "prefix decrement";
      }
    case xop_prefix_unset:
      {
        return "prefix `unset`";
      }
    case xop_prefix_lengthof:
      {
        return "prefix `lengthof`";
      }
    case xop_prefix_typeof:
      {
        return "prefix `typeof`";
      }
    case xop_infix_cmp_eq:
      {
        return "equality comparison";
      }
    case xop_infix_cmp_ne:
      {
        return "inequality comparison";
      }
    case xop_infix_cmp_lt:
      {
        return "less-than comparison";
      }
    case xop_infix_cmp_gt:
      {
        return "greater-than comparison";
      }
    case xop_infix_cmp_lte:
      {
        return "less-than-or-equal comparison";
      }
    case xop_infix_cmp_gte:
      {
        return "greater-than-or-equal comparison";
      }
    case xop_infix_cmp_3way:
      {
        return "three-way comparison";
      }
    case xop_infix_add:
      {
        return "addition";
      }
    case xop_infix_sub:
      {
        return "subtraction";
      }
    case xop_infix_mul:
      {
        return "multiplication";
      }
    case xop_infix_div:
      {
        return "division";
      }
    case xop_infix_mod:
      {
        return "modulo";
      }
    case xop_infix_sll:
      {
        return "logical left shift";
      }
    case xop_infix_srl:
      {
        return "arithmetic left shift";
      }
    case xop_infix_sla:
      {
        return "logical right shift";
      }
    case xop_infix_sra:
      {
        return "arithmetic right shift";
      }
    case xop_infix_andb:
      {
        return "bitwise and";
      }
    case xop_infix_orb:
      {
        return "bitwise or";
      }
    case xop_infix_xorb:
      {
        return "bitwise xor";
      }
    case xop_infix_assign:
      {
        return "assginment";
      }
    default:
      return "<unknown operator>";
    }
  }

    namespace {

    ROCKET_PURE_FUNCTION bool do_operator_not(bool rhs)
      {
        return !rhs;
      }

    ROCKET_PURE_FUNCTION bool do_operator_and(bool lhs, bool rhs)
      {
        return lhs & rhs;
      }

    ROCKET_PURE_FUNCTION bool do_operator_or(bool lhs, bool rhs)
      {
        return lhs | rhs;
      }

    ROCKET_PURE_FUNCTION bool do_operator_xor(bool lhs, bool rhs)
      {
        return lhs ^ rhs;
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_neg(std::int64_t rhs)
      {
        if(rhs == INT64_MIN) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral negation of `", rhs, "` would result in overflow.");
        }
        return -rhs;
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_add(std::int64_t lhs, std::int64_t rhs)
      {
        if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs)) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral addition of `", lhs, "` and `", rhs, "` would result in overflow.");
        }
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_sub(std::int64_t lhs, std::int64_t rhs)
      {
        if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs)) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral subtraction of `", lhs, "` and `", rhs, "` would result in overflow.");
        }
        return lhs - rhs;
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_mul(std::int64_t lhs, std::int64_t rhs)
      {
        if((lhs == 0) || (rhs == 0)) {
          return 0;
        }
        if((lhs == 1) || (rhs == 1)) {
          return lhs ^ rhs ^ 1;
        }
        if((lhs == INT64_MIN) || (rhs == INT64_MIN)) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication of `", lhs, "` and `", rhs, "` would result in overflow.");
        }
        if((lhs == -1) || (rhs == -1)) {
          return -(lhs ^ rhs ^ -1);
        }
        // signed lhs and absolute rhs
        auto slhs = lhs;
        auto arhs = rhs;
        if(rhs < 0) {
          slhs = -lhs;
          arhs = -rhs;
        }
        if((slhs >= 0) ? (slhs > INT64_MAX / arhs) : (slhs < INT64_MIN / arhs)) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication of `", lhs, "` and `", rhs, "` would result in overflow.");
        }
        return slhs * arhs;
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_div(std::int64_t lhs, std::int64_t rhs)
      {
        if(rhs == 0) {
          ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
        }
        if((lhs == INT64_MIN) && (rhs == -1)) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral division of `", lhs, "` and `", rhs, "` would result in overflow.");
        }
        return lhs / rhs;
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_mod(std::int64_t lhs, std::int64_t rhs)
      {
        if(rhs == 0) {
          ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
        }
        if((lhs == INT64_MIN) && (rhs == -1)) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral division of `", lhs, "` and `", rhs, "` would result in overflow.");
        }
        return lhs % rhs;
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_sll(std::int64_t lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return 0;
        }
        return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) << rhs);
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_srl(std::int64_t lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return 0;
        }
        return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) >> rhs);
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_sla(std::int64_t lhs, std::int64_t rhs)
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
        auto mask_out = static_cast<std::uint64_t>(lhs) >> bc << bc;
        auto mask_sbt = static_cast<std::uint64_t>(lhs >> 63) << bc;
        if(mask_out != mask_sbt) {
          ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
        }
        return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) << rhs);
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_sra(std::int64_t lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return lhs >> 63;
        }
        return lhs >> rhs;
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_not(std::int64_t rhs)
      {
        return ~rhs;
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_and(std::int64_t lhs, std::int64_t rhs)
      {
        return lhs & rhs;
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_or(std::int64_t lhs, std::int64_t rhs)
      {
        return lhs | rhs;
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_xor(std::int64_t lhs, std::int64_t rhs)
      {
        return lhs ^ rhs;
      }

    ROCKET_PURE_FUNCTION double do_operator_neg(double rhs)
      {
        return -rhs;
      }

    ROCKET_PURE_FUNCTION double do_operator_add(double lhs, double rhs)
      {
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION double do_operator_sub(double lhs, double rhs)
      {
        return lhs - rhs;
      }

    ROCKET_PURE_FUNCTION double do_operator_mul(double lhs, double rhs)
      {
        return lhs * rhs;
      }

    ROCKET_PURE_FUNCTION double do_operator_div(double lhs, double rhs)
      {
        return lhs / rhs;
      }

    ROCKET_PURE_FUNCTION double do_operator_mod(double lhs, double rhs)
      {
        return std::fmod(lhs, rhs);
      }

    ROCKET_PURE_FUNCTION Cow_String do_operator_add(const Cow_String &lhs, const Cow_String &rhs)
      {
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION Cow_String do_operator_mul(const Cow_String &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String duplication count `", rhs, "` for `", lhs, "` is negative.");
        }
        Cow_String res;
        auto nchars = lhs.size();
        if((nchars == 0) || (rhs == 0)) {
          return res;
        }
        if(nchars > res.max_size() / static_cast<std::uint64_t>(rhs)) {
          ASTERIA_THROW_RUNTIME_ERROR("Duplication of `", lhs, "` up to `", rhs, "` times would result in an overlong string that cannot be allocated.");
        }
        auto times = static_cast<std::size_t>(rhs);
        // Reserve space for the result string.
        auto ptr = rocket::unfancy(res.insert(res.begin(), nchars * times, ' '));
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

    ROCKET_PURE_FUNCTION Cow_String do_operator_mul(std::int64_t lhs, const Cow_String &rhs)
      {
        return do_operator_mul(rhs, lhs);
      }

    ROCKET_PURE_FUNCTION Cow_String do_operator_sll(const Cow_String &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        Cow_String res;
        // Reserve space for the result string.
        auto ptr = rocket::unfancy(res.insert(res.begin(), lhs.size(), ' '));
        if(static_cast<std::uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<std::size_t>(rhs);
        // Copy the substring in the right.
        std::memcpy(ptr, lhs.data() + count, lhs.size() - count);
        return res;
      }

    ROCKET_PURE_FUNCTION Cow_String do_operator_srl(const Cow_String &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        Cow_String res;
        // Reserve space for the result string.
        auto ptr = rocket::unfancy(res.insert(res.begin(), lhs.size(), ' '));
        if(static_cast<std::uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<std::size_t>(rhs);
        // Copy the substring in the left.
        std::memcpy(ptr + count, lhs.data(), lhs.size() - count);
        return res;
      }

    ROCKET_PURE_FUNCTION Cow_String do_operator_sla(const Cow_String &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        Cow_String res;
        if(static_cast<std::uint64_t>(rhs) >= res.max_size() - lhs.size()) {
          ASTERIA_THROW_RUNTIME_ERROR("Shifting `", lhs, "` to the left by `", rhs, "` bytes would result in an overlong string that cannot be allocated.");
        }
        auto count = static_cast<std::size_t>(rhs);
        // Append spaces in the right and return the result.
        res.assign(Cow_String::shallow_type(lhs));
        res.append(count, ' ');
        return res;
      }

    ROCKET_PURE_FUNCTION Cow_String do_operator_sra(const Cow_String &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        Cow_String res;
        if(static_cast<std::uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<std::size_t>(rhs);
        // Return the substring in the left.
        res.append(lhs.data(), lhs.size() - count);
        return res;
      }

    Air_Node::Status do_push_literal(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                     const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &value = p.at(0).as<Value>();
        // Push the constant.
        Reference_Root::S_constant ref_c = { value };
        stack_io.push_reference(rocket::move(ref_c));
        return Air_Node::status_next;
      }

    Air_Node::Status do_push_bound_reference(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                             const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &ref = p.at(0).as<Reference>();
        // Push the reference as is.
        stack_io.push_reference(ref);
        return Air_Node::status_next;
      }

    Air_Node::Status do_find_named_reference_local(Evaluation_Stack &stack_io, Executive_Context &ctx,
                                                   const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &name = p.at(0).as<PreHashed_String>();
        const auto &depth = static_cast<std::size_t>(p.at(1).as<std::int64_t>());
        // Locate the context.
        const Executive_Context *qctx = &ctx;
        for(std::size_t i = 0; i != depth; ++i) {
          qctx = qctx->get_parent_opt();
          ROCKET_ASSERT(qctx);
        }
        // Search for the name in the target context. It has to exist, or we would be having a bug here.
        auto qref = qctx->get_named_reference_opt(name);
        if(!qref) {
          ASTERIA_THROW_RUNTIME_ERROR("The identifier `", name, "` has not been declared yet.");
        }
        stack_io.push_reference(*qref);
        return Air_Node::status_next;
      }

    Air_Node::Status do_find_named_reference_global(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                    const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context &global)
      {
        // Decode arguments.
        const auto &name = p.at(0).as<PreHashed_String>();
        // Search for the name in the global context.
        auto qref = global.get_named_reference_opt(name);
        if(!qref) {
          ASTERIA_THROW_RUNTIME_ERROR("The identifier `", name, "` has not been declared yet.");
        }
        stack_io.push_reference(*qref);
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_closure_function(Evaluation_Stack &stack_io, Executive_Context &ctx,
                                                 const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &sloc = p.at(0).as<Source_Location>();
        const auto &params = p.at(1).as<Cow_Vector<PreHashed_String>>();
        const auto &body = p.at(2).as<Cow_Vector<Statement>>();
        // Generate code of the function body.
        Cow_Vector<Air_Node> code_body;
        Analytic_Context ctx_func(&ctx);
        ctx_func.prepare_function_parameters(params);
        rocket::for_each(body, [&](const Statement &stmt) { stmt.generate_code(code_body, nullptr, ctx_func);  });
        // Instantiate the function.
        Rcobj<Instantiated_Function> closure(sloc, rocket::sref("<closure function>"), params, rocket::move(code_body));
        ASTERIA_DEBUG_LOG("New closure function: ", closure);
        // Push the function object.
        Reference_Root::S_temporary ref_c = { D_function(rocket::move(closure)) };
        stack_io.push_reference(rocket::move(ref_c));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_branch(Evaluation_Stack &stack_io, Executive_Context &ctx,
                                       const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const auto &code_true = p.at(0).as<Cow_Vector<Air_Node>>();
        const auto &code_false = p.at(1).as<Cow_Vector<Air_Node>>();
        const auto &assign = static_cast<bool>(p.at(2).as<std::int64_t>());
        // Pick a branch basing on the condition.
        if(stack_io.get_top_reference().read().test()) {
          // Execute the true branch. If the branch is empty, leave the condition on the stack.
          if(code_true.empty()) {
            return Air_Node::status_next;
          }
          rocket::for_each(code_true, [&](const Air_Node &node) { node.execute(stack_io, ctx, func, global);  });
          stack_io.forward_result(assign);
          return Air_Node::status_next;
        }
        // Execute the false branch. If the branch is empty, leave the condition on the stack.
        if(code_false.empty()) {
          return Air_Node::status_next;
        }
        rocket::for_each(code_false, [&](const Air_Node &node) { node.execute(stack_io, ctx, func, global);  });
        stack_io.forward_result(assign);
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_function_call(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                              const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const auto &sloc = p.at(0).as<Source_Location>();
        const auto &nargs = static_cast<std::size_t>(p.at(1).as<std::int64_t>());
        // Allocate the argument vector.
        Cow_Vector<Reference> args;
        args.resize(nargs);
        for(auto it = args.mut_rbegin(); it != args.rend(); ++it) {
          *it = rocket::move(stack_io.open_top_reference());
          stack_io.pop_reference();
        }
        // Get the target reference.
        auto target_value = stack_io.get_top_reference().read();
        // Make sure it is really a function.
        if(target_value.type() != type_function) {
          ASTERIA_THROW_RUNTIME_ERROR("An attempt was made to invoke `", target_value, "` which is not a function.");
        }
        const auto &target = target_value.check<D_function>().get();
        // Make the `this` reference. On the function's return it is reused to store the result of the function.
        auto &self_result = stack_io.open_top_reference().zoom_out();
        // Call the function now.
        ASTERIA_DEBUG_LOG("Initiating function call at \'", sloc, "\' inside `", func, "`: target = ", target, ", this = ", self_result.read());
        try {
          target.invoke(self_result, global, rocket::move(args));
        } catch(std::exception &stdex) {
          ASTERIA_DEBUG_LOG("Caught `std::exception` thrown inside function call at \'", sloc, "\' inside `", func, "`: what = ", stdex.what());
          // Translate the exception.
          auto traceable = trace_exception(rocket::move(stdex));
          traceable.append_frame(sloc, func);
          throw traceable;
        }
        // The result will have been written to `self_result`.
        ASTERIA_DEBUG_LOG("Returned from function call at \'", sloc, "\' inside `", func, "`: target = ", target, ", result = ", self_result.read());
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_member_access(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                              const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &name = p.at(0).as<PreHashed_String>();
        // Append a modifier.
        Reference_Modifier::S_object_key mod_c = { name };
        stack_io.open_top_reference().zoom_in(rocket::move(mod_c));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_postfix_inc(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                         const Cow_Vector<Air_Node::Variant> & /*p*/, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Increment the operand and return the old value.
        // `alt.assign` is ignored.
        auto &lhs = stack_io.get_top_reference().open();
        if(lhs.type() == type_integer) {
          auto &reg = lhs.check<D_integer>();
          stack_io.set_temporary_result(false, rocket::move(lhs));
          reg = do_operator_add(reg, D_integer(1));
          goto z;
        }
        if(lhs.type() == type_real) {
          auto &reg = lhs.check<D_real>();
          stack_io.set_temporary_result(false, rocket::move(lhs));
          reg = do_operator_add(reg, D_real(1));
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_postfix_inc), " operation is not defined for `", lhs, "`.");
      z:
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_postfix_dec(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                         const Cow_Vector<Air_Node::Variant> & /*p*/, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decrement the operand and return the old value.
        // `alt.assign` is ignored.
        auto &lhs = stack_io.get_top_reference().open();
        if(lhs.type() == type_integer) {
          auto &reg = lhs.check<D_integer>();
          stack_io.set_temporary_result(false, rocket::move(lhs));
          reg = do_operator_sub(reg, D_integer(1));
          goto z;
        }
        if(lhs.type() == type_real) {
          auto &reg = lhs.check<D_real>();
          stack_io.set_temporary_result(false, rocket::move(lhs));
          reg = do_operator_sub(reg, D_real(1));
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_postfix_dec), " operation is not defined for `", lhs, "`.");
      z:
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_postfix_at(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                        const Cow_Vector<Air_Node::Variant> & /*p*/, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Append a reference modifier.
        // `alt.assign` is ignored.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        if(rhs.type() == type_integer) {
          auto &reg = rhs.check<D_integer>();
          Reference_Modifier::S_array_index mod_c = { rocket::move(reg) };
          stack_io.open_top_reference().zoom_in(rocket::move(mod_c));
          goto z;
        }
        if(rhs.type() == type_string) {
          auto &reg = rhs.check<D_string>();
          Reference_Modifier::S_object_key mod_c = { rocket::move(reg) };
          stack_io.open_top_reference().zoom_in(rocket::move(mod_c));
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The value `", rhs, "` cannot be used as a subscript.");
      z:
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_prefix_pos(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                        const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Copy the operand to create a temporary value, then return it.
        // N.B. This is one of the few operators that work on all types.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_prefix_neg(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                        const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Negate the operand to create a temporary value, then return it.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        if(rhs.type() == type_integer) {
          auto &reg = rhs.check<D_integer>();
          reg = do_operator_neg(reg);
          goto z;
        }
        if(rhs.type() == type_real) {
          auto &reg = rhs.check<D_real>();
          reg = do_operator_neg(reg);
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_prefix_neg), " operation is not defined for `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_prefix_notb(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                         const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Perform bitwise NOT operation on the operand to create a temporary value, then return it.
        auto rhs = stack_io.get_top_reference().read();
        if(rhs.type() == type_boolean) {
          auto &reg = rhs.check<D_boolean>();
          reg = do_operator_not(reg);
          goto z;
        }
        if(rhs.type() == type_integer) {
          auto &reg = rhs.check<D_integer>();
          reg = do_operator_not(reg);
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_prefix_notb), " operation is not defined for `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_prefix_notl(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                         const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Perform logical NOT operation on the operand to create a temporary value, then return it.
        // N.B. This is one of the few operators that work on all types.
        const auto &rhs = stack_io.get_top_reference().read();
        stack_io.set_temporary_result(assign, D_boolean(do_operator_not(rhs.test())));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_prefix_inc(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                        const Cow_Vector<Air_Node::Variant> & /*p*/, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Increment the operand and return it.
        // `alt.assign` is ignored.
        auto &rhs = stack_io.get_top_reference().open();
        if(rhs.type() == type_integer) {
          auto &reg = rhs.check<D_integer>();
          reg = do_operator_add(reg, D_integer(1));
          goto z;
        }
        if(rhs.type() == type_real) {
          auto &reg = rhs.check<D_real>();
          reg = do_operator_add(reg, D_real(1));
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_prefix_inc), " operation is not defined for `", rhs, "`.");
      z:
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_prefix_dec(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                        const Cow_Vector<Air_Node::Variant> & /*p*/, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decrement the operand and return it.
        // `alt.assign` is ignored.
        auto &rhs = stack_io.get_top_reference().open();
        if(rhs.type() == type_integer) {
          auto &reg = rhs.check<D_integer>();
          reg = do_operator_sub(reg, D_integer(1));
          return Air_Node::status_next;
        }
        if(rhs.type() == type_real) {
          auto &reg = rhs.check<D_real>();
          reg = do_operator_sub(reg, D_real(1));
          return Air_Node::status_next;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_prefix_dec), " operation is not defined for `", rhs, "`.");
      }

    Air_Node::Status do_execute_operator_rpn_prefix_unset(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                          const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Unset the reference and return the old value.
        auto rhs = stack_io.get_top_reference().unset();
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_prefix_lengthof(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                             const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Return the number of elements in the operand.
        const auto &rhs = stack_io.get_top_reference().read();
        if(rhs.type() == type_null) {
          stack_io.set_temporary_result(assign, D_integer(0));
          goto z;
        }
        if(rhs.type() == type_string) {
          stack_io.set_temporary_result(assign, D_integer(rhs.check<D_string>().size()));
          goto z;
        }
        if(rhs.type() == type_array) {
          stack_io.set_temporary_result(assign, D_integer(rhs.check<D_array>().size()));
          goto z;
        }
        if(rhs.type() == type_object) {
          stack_io.set_temporary_result(assign, D_integer(rhs.check<D_object>().size()));
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_prefix_lengthof), " operation is not defined for `", rhs, "`.");
      z:
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_prefix_typeof(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                           const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Return the type name of the operand.
        // N.B. This is one of the few operators that work on all types.
        const auto &rhs = stack_io.get_top_reference().read();
        stack_io.set_temporary_result(assign, D_string(rocket::sref(Value::get_type_name(rhs.type()))));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_cmp_xeq(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                           const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        const auto &negative = static_cast<bool>(p.at(1).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        rhs = D_boolean((comp == Value::compare_equal) != negative);
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_cmp_xrel(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                            const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        const auto &expect = static_cast<Value::Compare>(p.at(1).as<std::int64_t>());
        const auto &negative = static_cast<bool>(p.at(2).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        if(comp == Value::compare_unordered) {
          ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs, "` and `", rhs, "` are unordered.");
        }
        rhs = D_boolean((comp == expect) != negative);
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_cmp_3way(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                            const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        auto comp = lhs.compare(rhs);
        if(comp == Value::compare_unordered) {
          rhs = D_string(rocket::sref("<unordered>"));
          goto z;
        }
        if(comp == Value::compare_less) {
          rhs = D_integer(-1);
          goto z;
        }
        if(comp == Value::compare_greater) {
          rhs = D_integer(+1);
          goto z;
        }
        rhs = D_integer(0);
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_add(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                       const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // For the `boolean` type, return the logical OR'd result of both operands.
        if((lhs.type() == type_boolean) && (rhs.type() == type_boolean)) {
          auto &reg = rhs.check<D_boolean>();
          reg = do_operator_or(lhs.check<D_boolean>(), reg);
          goto z;
        }
        // For the `integer` and `real` types, return the sum of both operands.
        if((lhs.type() == type_integer) && (rhs.type() == type_integer)) {
          auto &reg = rhs.check<D_integer>();
          reg = do_operator_add(lhs.check<D_integer>(), reg);
          goto z;
        }
        if((lhs.type() == type_real) && (rhs.type() == type_real)) {
          auto &reg = rhs.check<D_real>();
          reg = do_operator_add(lhs.check<D_real>(), reg);
          goto z;
        }
        // For the `string` type, concatenate the operands in lexical order to create a new string, then return it.
        if((lhs.type() == type_string) && (rhs.type() == type_string)) {
          auto &reg = rhs.check<D_string>();
          reg = do_operator_add(lhs.check<D_string>(), reg);
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_infix_add), " operation is not defined for `", lhs, "` and `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_sub(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                       const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // For the `boolean` type, return the logical XOR'd result of both operands.
        if((lhs.type() == type_boolean) && (rhs.type() == type_boolean)) {
          auto &reg = rhs.check<D_boolean>();
          reg = do_operator_xor(lhs.check<D_boolean>(), reg);
          goto z;
        }
        // For the `integer` and `real` types, return the difference of both operands.
        if((lhs.type() == type_integer) && (rhs.type() == type_integer)) {
          auto &reg = rhs.check<D_integer>();
          reg = do_operator_sub(lhs.check<D_integer>(), reg);
          goto z;
        }
        if((lhs.type() == type_real) && (rhs.type() == type_real)) {
          auto &reg = rhs.check<D_real>();
          reg = do_operator_sub(lhs.check<D_real>(), reg);
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_infix_sub), " operation is not defined for `", lhs, "` and `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_mul(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                       const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // For the `boolean` type, return the logical AND'd result of both operands.
        if((lhs.type() == type_boolean) && (rhs.type() == type_boolean)) {
          auto &reg = rhs.check<D_boolean>();
          reg = do_operator_and(lhs.check<D_boolean>(), reg);
          goto z;
        }
        // For the `integer` and `real` types, return the product of both operands.
        if((lhs.type() == type_integer) && (rhs.type() == type_integer)) {
          auto &reg = rhs.check<D_integer>();
          reg = do_operator_mul(lhs.check<D_integer>(), reg);
          goto z;
        }
        if((lhs.type() == type_real) && (rhs.type() == type_real)) {
          auto &reg = rhs.check<D_real>();
          reg = do_operator_mul(lhs.check<D_real>(), reg);
          goto z;
        }
        // If either operand has type `string` and the other has type `integer`, duplicate the string up to the specified number of times and return the result.
        if((lhs.type() == type_string) && (rhs.type() == type_integer)) {
          // Note that `rhs` does not have type `D_string`, thus this branch can't be optimized.
          rhs = do_operator_mul(lhs.check<D_string>(), rhs.check<D_integer>());
          goto z;
        }
        if((lhs.type() == type_integer) && (rhs.type() == type_string)) {
          auto &reg = rhs.check<D_string>();
          reg = do_operator_mul(lhs.check<D_integer>(), reg);
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_infix_mul), " operation is not defined for `", lhs, "` and `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_div(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                       const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // For the `integer` and `real` types, return the quotient of both operands.
        if((lhs.type() == type_integer) && (rhs.type() == type_integer)) {
          auto &reg = rhs.check<D_integer>();
          reg = do_operator_div(lhs.check<D_integer>(), reg);
          goto z;
        }
        if((lhs.type() == type_real) && (rhs.type() == type_real)) {
          auto &reg = rhs.check<D_real>();
          reg = do_operator_div(lhs.check<D_real>(), reg);
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_infix_div), " operation is not defined for `", lhs, "` and `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_mod(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                       const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // For the `integer` and `real` types, return the remainder of both operands.
        if((lhs.type() == type_integer) && (rhs.type() == type_integer)) {
          auto &reg = rhs.check<D_integer>();
          reg = do_operator_mod(lhs.check<D_integer>(), reg);
          goto z;
        }
        if((lhs.type() == type_real) && (rhs.type() == type_real)) {
          auto &reg = rhs.check<D_real>();
          reg = do_operator_mod(lhs.check<D_real>(), reg);
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_infix_mod), " operation is not defined for `", lhs, "` and `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_sll(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                       const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // The RHS operand shall be of type `integer`.
        if(rhs.type() == type_integer) {
          // If the LHS operand has type `integer`, shift the LHS operand to the left by the number of bits specified by the RHS operand.
          // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
          if(lhs.type() == type_integer) {
            auto &reg = rhs.check<D_integer>();
            rhs = do_operator_sll(lhs.check<D_integer>(), reg);
            goto z;
          }
          // If the LHS operand has type `string`, fill space characters in the right and discard characters from the left.
          // The number of bytes in the LHS operand will be preserved.
          if(lhs.type() == type_string) {
            // Note that `rhs` does not have type `D_string`, thus this branch can't be optimized.
            rhs = do_operator_sll(lhs.check<D_string>(), rhs.check<D_integer>());
            goto z;
          }
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_infix_sll), " operation is not defined for `", lhs, "` and `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_srl(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                       const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // The RHS operand shall be of type `integer`.
        if(rhs.type() == type_integer) {
          // If the LHS operand has type `integer`, shift the LHS operand to the right by the number of bits specified by the RHS operand.
          // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
          if(lhs.type() == type_integer) {
            auto &reg = rhs.check<D_integer>();
            rhs = do_operator_srl(lhs.check<D_integer>(), reg);
            goto z;
          }
          // If the LHS operand has type `string`, fill space characters in the left and discard characters from the right.
          // The number of bytes in the LHS operand will be preserved.
          if(lhs.type() == type_string) {
            // Note that `rhs` does not have type `D_string`, thus this branch can't be optimized.
            rhs = do_operator_srl(lhs.check<D_string>(), rhs.check<D_integer>());
            goto z;
          }
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_infix_srl), " operation is not defined for `", lhs, "` and `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_sla(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                       const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // The RHS operand shall be of type `integer`.
        if(rhs.type() == type_integer) {
          // If the LHS operand is of type `integer`, shift the LHS operand to the left by the number of bits specified by the RHS operand.
          // Bits shifted out that are equal to the sign bit are discarded. Bits shifted in are filled with zeroes.
          // If any bits that are different from the sign bit would be shifted out, an exception is thrown.
          if(lhs.type() == type_integer) {
            auto &reg = rhs.check<D_integer>();
            rhs = do_operator_sla(lhs.check<D_integer>(), reg);
            goto z;
          }
          // If the LHS operand has type `string`, fill space characters in the right.
          if(lhs.type() == type_string) {
            // Note that `rhs` does not have type `D_string`, thus this branch can't be optimized.
            rhs = do_operator_sla(lhs.check<D_string>(), rhs.check<D_integer>());
            goto z;
          }
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_infix_sla), " operation is not defined for `", lhs, "` and `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_sra(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                       const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // The RHS operand shall be of type `integer`.
        if(rhs.type() == type_integer) {
          // If the LHS operand is of type `integer`, shift the LHS operand to the right by the number of bits specified by the RHS operand.
          // Bits shifted out are discarded. Bits shifted in are filled with the sign bit.
          if(lhs.type() == type_integer) {
            auto &reg = rhs.check<D_integer>();
            rhs = do_operator_sra(lhs.check<D_integer>(), reg);
            goto z;
          }
          // If the LHS operand has type `string`, discard characters from the right.
          if(lhs.type() == type_string) {
            // Note that `rhs` does not have type `D_string`, thus this branch can't be optimized.
            rhs = do_operator_sra(lhs.check<D_string>(), rhs.check<D_integer>());
            goto z;
          }
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_infix_sra), " operation is not defined for `", lhs, "` and `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_andb(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                        const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // For the `boolean` type, return the logical AND'd result of both operands.
        if((lhs.type() == type_boolean) && (rhs.type() == type_boolean)) {
          auto &reg = rhs.check<D_boolean>();
          reg = do_operator_and(lhs.check<D_boolean>(), reg);
          goto z;
        }
        // For the `integer` type, return bitwise AND'd result of both operands.
        if((lhs.type() == type_integer) && (rhs.type() == type_integer)) {
          auto &reg = rhs.check<D_integer>();
          reg = do_operator_and(lhs.check<D_integer>(), reg);
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_infix_andb), " operation is not defined for `", lhs, "` and `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_orb(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                       const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // For the `boolean` type, return the logical OR'd result of both operands.
        if((lhs.type() == type_boolean) && (rhs.type() == type_boolean)) {
          auto &reg = rhs.check<D_boolean>();
          reg = do_operator_or(lhs.check<D_boolean>(), reg);
          goto z;
        }
        // For the `integer` type, return bitwise OR'd result of both operands.
        if((lhs.type() == type_integer) && (rhs.type() == type_integer)) {
          auto &reg = rhs.check<D_integer>();
          reg = do_operator_or(lhs.check<D_integer>(), reg);
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_infix_orb), " operation is not defined for `", lhs, "` and `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_xorb(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                        const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &assign = static_cast<bool>(p.at(0).as<std::int64_t>());
        // Pop the RHS operand followed by the LHS operand.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        const auto &lhs = stack_io.get_top_reference().read();
        // For the `boolean` type, return the logical XOR'd result of both operands.
        if((lhs.type() == type_boolean) && (rhs.type() == type_boolean)) {
          auto &reg = rhs.check<D_boolean>();
          reg = do_operator_xor(lhs.check<D_boolean>(), reg);
          goto z;
        }
        // For the `integer` type, return bitwise XOR'd result of both operands.
        if((lhs.type() == type_integer) && (rhs.type() == type_integer)) {
          auto &reg = rhs.check<D_integer>();
          reg = do_operator_xor(lhs.check<D_integer>(), reg);
          goto z;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xprunit::get_operator_name(Xprunit::xop_infix_orb), " operation is not defined for `", lhs, "` and `", rhs, "`.");
      z:
        stack_io.set_temporary_result(assign, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_operator_rpn_infix_assign(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                                          const Cow_Vector<Air_Node::Variant> & /*p*/, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Pop the RHS operand followed.
        auto rhs = stack_io.get_top_reference().read();
        stack_io.pop_reference();
        // Copy the value to the LHS operand which is write-only.
        // `alt.assign` is ignored.
        stack_io.set_temporary_result(true, rocket::move(rhs));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_unnamed_array(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                              const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &nelems = static_cast<std::size_t>(p.at(0).as<std::int64_t>());
        // Pop references to create an array.
        D_array array;
        array.resize(nelems);
        for(auto it = array.mut_rbegin(); it != array.rend(); ++it) {
          *it = stack_io.get_top_reference().read();
          stack_io.pop_reference();
        }
        // Push the array as a temporary.
        Reference_Root::S_temporary ref_c = { rocket::move(array) };
        stack_io.push_reference(rocket::move(ref_c));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_unnamed_object(Evaluation_Stack &stack_io, Executive_Context & /*ctx*/,
                                               const Cow_Vector<Air_Node::Variant> &p, const Cow_String & /*func*/, const Global_Context & /*global*/)
      {
        // Decode arguments.
        const auto &keys = p.at(0).as<Cow_Vector<PreHashed_String>>();
        // Pop references to create an object.
        D_object object;
        object.reserve(keys.size());
        for(auto it = keys.rbegin(); it != keys.rend(); ++it) {
          object.try_emplace(*it, stack_io.get_top_reference().read());
          stack_io.pop_reference();
        }
        // Push the object as a temporary.
        Reference_Root::S_temporary ref_c = { rocket::move(object) };
        stack_io.push_reference(rocket::move(ref_c));
        return Air_Node::status_next;
      }

    Air_Node::Status do_execute_coalescence(Evaluation_Stack &stack_io, Executive_Context &ctx,
                                            const Cow_Vector<Air_Node::Variant> &p, const Cow_String &func, const Global_Context &global)
      {
        // Decode arguments.
        const auto &code_null = p.at(0).as<Cow_Vector<Air_Node>>();
        const auto &assign = static_cast<bool>(p.at(1).as<std::int64_t>());
        // Pick a branch basing on the condition.
        if(stack_io.get_top_reference().read().type() != type_null) {
          // Leave the condition on the stack.
          return Air_Node::status_next;
        }
        if(code_null.empty()) {
          // Leave the condition on the stack.
          return Air_Node::status_next;
        }
        // Evaluate the branch.
        rocket::for_each(code_null, [&](const Air_Node &node) { node.execute(stack_io, ctx, func, global);  });
        stack_io.forward_result(assign);
        return Air_Node::status_next;
      }

    }  // namespace

void Xprunit::generate_code(Cow_Vector<Air_Node> &code_out, const Analytic_Context &ctx) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_literal:
      {
        const auto &alt = this->m_stor.as<S_literal>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(alt.value);  // 0
        code_out.emplace_back(&do_push_literal, rocket::move(p));
        return;
      }
    case index_named_reference:
      {
        const auto &alt = this->m_stor.as<S_named_reference>();
        // Perform early lookup when the expression is defined.
        // If a named reference is found, it will not be replaced or hidden by a later-declared one.
        const Abstract_Context *qctx = &ctx;
        std::size_t depth = 0;
        for(;;) {
          auto qref = qctx->get_named_reference_opt(alt.name);
          if(qref) {
            if(!qctx->is_analytic()) {
              // Bind the reference.
              Cow_Vector<Air_Node::Variant> p;
              p.emplace_back(*qref);  // 0
              code_out.emplace_back(&do_push_bound_reference, rocket::move(p));
              return;
            }
            // A later-declared reference has been found.
            // Record the context depth for later lookups.
            Cow_Vector<Air_Node::Variant> p;
            p.emplace_back(alt.name);  // 0
            p.emplace_back(static_cast<std::int64_t>(depth));  // 1
            code_out.emplace_back(&do_find_named_reference_local, rocket::move(p));
            return;
          }
          qctx = qctx->get_parent_opt();
          if(!qctx) {
            break;
          }
          ++depth;
        }
        // No name has been found so far.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(alt.name);  // 0
        code_out.emplace_back(&do_find_named_reference_global, rocket::move(p));
        return;
      }
    case index_closure_function:
      {
        const auto &alt = this->m_stor.as<S_closure_function>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(alt.sloc);  // 0
        p.emplace_back(alt.params);  // 1
        p.emplace_back(alt.body);  // 2
        code_out.emplace_back(&do_execute_closure_function, rocket::move(p));
        return;
      }
    case index_branch:
      {
        const auto &alt = this->m_stor.as<S_branch>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        Cow_Vector<Air_Node> code;
        rocket::for_each(alt.branch_true, [&](const Xprunit &xpru) { xpru.generate_code(code, ctx);  });
        p.emplace_back(rocket::move(code));  // 0
        code.clear();
        rocket::for_each(alt.branch_false, [&](const Xprunit &xpru) { xpru.generate_code(code, ctx);  });
        p.emplace_back(rocket::move(code));  // 1
        p.emplace_back(static_cast<std::int64_t>(alt.assign));  // 2
        code_out.emplace_back(&do_execute_branch, rocket::move(p));
        return;
      }
    case index_function_call:
      {
        const auto &alt = this->m_stor.as<S_function_call>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(alt.sloc);  // 0
        p.emplace_back(static_cast<std::int64_t>(alt.nargs));  // 1
        code_out.emplace_back(&do_execute_function_call, rocket::move(p));
        return;
      }
    case index_member_access:
      {
        const auto &alt = this->m_stor.as<S_member_access>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(alt.name);  // 0
        code_out.emplace_back(&do_execute_member_access, rocket::move(p));
        return;
      }
    case index_operator_rpn:
      {
        const auto &alt = this->m_stor.as<S_operator_rpn>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(static_cast<std::int64_t>(alt.assign));  // 0
        switch(alt.xop) {
        case xop_postfix_inc:
          {
            code_out.emplace_back(&do_execute_operator_rpn_postfix_inc, rocket::move(p));
            return;
          }
        case xop_postfix_dec:
          {
            code_out.emplace_back(&do_execute_operator_rpn_postfix_dec, rocket::move(p));
            return;
          }
        case xop_postfix_at:
          {
            code_out.emplace_back(&do_execute_operator_rpn_postfix_at, rocket::move(p));
            return;
          }
        case xop_prefix_pos:
          {
            code_out.emplace_back(&do_execute_operator_rpn_prefix_pos, rocket::move(p));
            return;
          }
        case xop_prefix_neg:
          {
            code_out.emplace_back(&do_execute_operator_rpn_prefix_neg, rocket::move(p));
            return;
          }
        case xop_prefix_notb:
          {
            code_out.emplace_back(&do_execute_operator_rpn_prefix_notb, rocket::move(p));
            return;
          }
        case xop_prefix_notl:
          {
            code_out.emplace_back(&do_execute_operator_rpn_prefix_notl, rocket::move(p));
            return;
          }
        case xop_prefix_inc:
          {
            code_out.emplace_back(&do_execute_operator_rpn_prefix_inc, rocket::move(p));
            return;
          }
        case xop_prefix_dec:
          {
            code_out.emplace_back(&do_execute_operator_rpn_prefix_dec, rocket::move(p));
            return;
          }
        case xop_prefix_unset:
          {
            code_out.emplace_back(&do_execute_operator_rpn_prefix_unset, rocket::move(p));
            return;
          }
        case xop_prefix_lengthof:
          {
            code_out.emplace_back(&do_execute_operator_rpn_prefix_lengthof, rocket::move(p));
            return;
          }
        case xop_prefix_typeof:
          {
            code_out.emplace_back(&do_execute_operator_rpn_prefix_typeof, rocket::move(p));
            return;
          }
        case xop_infix_cmp_eq:
          {
            p.emplace_back(static_cast<std::int64_t>(false));  // 1
            code_out.emplace_back(&do_execute_operator_rpn_infix_cmp_xeq, rocket::move(p));
            return;
          }
        case xop_infix_cmp_ne:
          {
            p.emplace_back(static_cast<std::int64_t>(true));  // 1
            code_out.emplace_back(&do_execute_operator_rpn_infix_cmp_xeq, rocket::move(p));
            return;
          }
        case xop_infix_cmp_lt:
          {
            p.emplace_back(static_cast<std::int64_t>(Value::compare_less));  // 1
            p.emplace_back(static_cast<std::int64_t>(false));  // 2
            code_out.emplace_back(&do_execute_operator_rpn_infix_cmp_xrel, rocket::move(p));
            return;
          }
        case xop_infix_cmp_gt:
          {
            p.emplace_back(static_cast<std::int64_t>(Value::compare_greater));  // 1
            p.emplace_back(static_cast<std::int64_t>(false));  // 2
            code_out.emplace_back(&do_execute_operator_rpn_infix_cmp_xrel, rocket::move(p));
            return;
          }
        case xop_infix_cmp_lte:
          {
            p.emplace_back(static_cast<std::int64_t>(Value::compare_greater));  // 1
            p.emplace_back(static_cast<std::int64_t>(true));  // 2
            code_out.emplace_back(&do_execute_operator_rpn_infix_cmp_xrel, rocket::move(p));
            return;
          }
        case xop_infix_cmp_gte:
          {
            p.emplace_back(static_cast<std::int64_t>(Value::compare_less));  // 1
            p.emplace_back(static_cast<std::int64_t>(true));  // 2
            code_out.emplace_back(&do_execute_operator_rpn_infix_cmp_xrel, rocket::move(p));
            return;
          }
        case xop_infix_cmp_3way:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_cmp_3way, rocket::move(p));
            return;
          }
        case xop_infix_add:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_add, rocket::move(p));
            return;
          }
        case xop_infix_sub:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_sub, rocket::move(p));
            return;
          }
        case xop_infix_mul:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_mul, rocket::move(p));
            return;
          }
        case xop_infix_div:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_div, rocket::move(p));
            return;
          }
        case xop_infix_mod:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_mod, rocket::move(p));
            return;
          }
        case xop_infix_sll:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_sll, rocket::move(p));
            return;
          }
        case xop_infix_srl:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_srl, rocket::move(p));
            return;
          }
        case xop_infix_sla:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_sla, rocket::move(p));
            return;
          }
        case xop_infix_sra:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_sra, rocket::move(p));
            return;
          }
        case xop_infix_andb:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_andb, rocket::move(p));
            return;
          }
        case xop_infix_orb:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_orb, rocket::move(p));
            return;
          }
        case xop_infix_xorb:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_xorb, rocket::move(p));
            return;
          }
        case xop_infix_assign:
          {
            code_out.emplace_back(&do_execute_operator_rpn_infix_assign, rocket::move(p));
            return;
          }
        default:
          ASTERIA_TERMINATE("An unknown operator type enumeration `", alt.xop, "` has been encountered.");
        }
      }
    case index_unnamed_array:
      {
        const auto &alt = this->m_stor.as<S_unnamed_array>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(static_cast<std::int64_t>(alt.nelems));  // 0
        code_out.emplace_back(&do_execute_unnamed_array, rocket::move(p));
        return;
      }
    case index_unnamed_object:
      {
        const auto &alt = this->m_stor.as<S_unnamed_object>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        p.emplace_back(alt.keys);  // 0
        code_out.emplace_back(&do_execute_unnamed_object, rocket::move(p));
        return;
      }
    case index_coalescence:
      {
        const auto &alt = this->m_stor.as<S_coalescence>();
        // Encode arguments.
        Cow_Vector<Air_Node::Variant> p;
        Cow_Vector<Air_Node> code;
        rocket::for_each(alt.branch_null, [&](const Xprunit &xpru) { xpru.generate_code(code, ctx);  });
        p.emplace_back(rocket::move(code));  // 0
        p.emplace_back(static_cast<std::int64_t>(alt.assign));  // 1
        code_out.emplace_back(&do_execute_coalescence, rocket::move(p));
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown expression unit type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

}  // namespace Asteria
