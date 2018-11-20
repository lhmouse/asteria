// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "reference_stack.hpp"
#include "analytic_context.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "abstract_function.hpp"
#include "instantiated_function.hpp"
#include "exception.hpp"
#include "utilities.hpp"

namespace Asteria {

const char * Xpnode::get_operator_name(Xpnode::Xop xop) noexcept
  {
    switch(xop) {
      case xop_postfix_inc: {
        return "postfix increment";
      }
      case xop_postfix_dec: {
        return "postfix decrement";
      }
      case xop_prefix_pos: {
        return "unary plus";
      }
      case xop_prefix_neg: {
        return "unary negation";
      }
      case xop_prefix_notb: {
        return "bitwise not";
      }
      case xop_prefix_notl: {
        return "logical not";
      }
      case xop_prefix_inc: {
        return "prefix increment";
      }
      case xop_prefix_dec: {
        return "prefix decrement";
      }
      case xop_prefix_unset: {
        return "prefix `unset`";
      }
      case xop_prefix_lengthof: {
        return "prefix `lengthof`";
      }
      case xop_infix_cmp_eq: {
        return "equality comparison";
      }
      case xop_infix_cmp_ne: {
        return "inequality comparison";
      }
      case xop_infix_cmp_lt: {
        return "less-than comparison";
      }
      case xop_infix_cmp_gt: {
        return "greater-than comparison";
      }
      case xop_infix_cmp_lte: {
        return "less-than-or-equal comparison";
      }
      case xop_infix_cmp_gte: {
        return "greater-than-or-equal comparison";
      }
      case xop_infix_cmp_3way: {
        return "three-way comparison";
      }
      case xop_infix_add: {
        return "addition";
      }
      case xop_infix_sub: {
        return "subtraction";
      }
      case xop_infix_mul: {
        return "multiplication";
      }
      case xop_infix_div: {
        return "division";
      }
      case xop_infix_mod: {
        return "modulo";
      }
      case xop_infix_sll: {
        return "logical left shift";
      }
      case xop_infix_srl: {
        return "arithmetic left shift";
      }
      case xop_infix_sla: {
        return "logical right shift";
      }
      case xop_infix_sra: {
        return "arithmetic right shift";
      }
      case xop_infix_andb: {
        return "bitwise and";
      }
      case xop_infix_orb: {
        return "bitwise or";
      }
      case xop_infix_xorb: {
        return "bitwise xor";
      }
      case xop_infix_assign: {
        return "assginment";
      }
      default: {
        return "<unknown operator>";
      }
    }
  }

Xpnode::~Xpnode()
  {
  }

    namespace {

    std::pair<std::reference_wrapper<const Abstract_context>,
      std::reference_wrapper<const Reference>> do_name_lookup(const Global_context &global, const Abstract_context &ctx, const rocket::prehashed_string &name)
      {
        auto spare = &global;
        auto qctx = &ctx;
        do {
          const auto qref = qctx->get_named_reference_opt(name);
          if(qref) {
            return std::make_pair(std::ref(*qctx), std::ref(*qref));
          }
          qctx = qctx->get_parent_opt();
          if(!qctx) {
            qctx = rocket::exchange(spare, nullptr);
            if(!qctx) {
              ASTERIA_THROW_RUNTIME_ERROR("The identifier `", name, "` has not been declared yet.");
            }
          }
        } while(true);
      }

    }

Xpnode Xpnode::bind(const Global_context &global, const Analytic_context &ctx) const
  {
    switch(Index(this->m_stor.index())) {
      case index_literal: {
        const auto &alt = this->m_stor.as<S_literal>();
        // Copy it as-is.
        Xpnode::S_literal alt_bnd = { alt.value };
        return std::move(alt_bnd);
      }
      case index_named_reference: {
        const auto &alt = this->m_stor.as<S_named_reference>();
        // Only references with non-reserved names can be bound.
        if(alt.name.rdstr().starts_with("__")) {
          // Copy it as-is.
          Xpnode::S_named_reference alt_bnd = { alt.name };
          return std::move(alt_bnd);
        }
        // Look for the reference in the current context.
        auto pair = do_name_lookup(global, ctx, alt.name);
        if(pair.first.get().is_analytic()) {
          // Don't bind it onto something in a analytic context which will soon get destroyed.
          Xpnode::S_named_reference alt_bnd = { alt.name };
          return std::move(alt_bnd);
        }
        Xpnode::S_bound_reference alt_bnd = { pair.second };
        return std::move(alt_bnd);
      }
      case index_bound_reference: {
        const auto &alt = this->m_stor.as<S_bound_reference>();
        // Copy it as-is.
        Xpnode::S_bound_reference alt_bnd = { alt.ref };
        return std::move(alt_bnd);
      }
      case index_closure_function: {
        const auto &alt = this->m_stor.as<S_closure_function>();
        // Bind the body recursively.
        Analytic_context ctx_next(&ctx);
        ctx_next.initialize_for_function(alt.head);
        auto body_bnd = alt.body.bind_in_place(ctx_next, global);
        Xpnode::S_closure_function alt_bnd = { alt.head, std::move(body_bnd) };
        return std::move(alt_bnd);
      }
      case index_branch: {
        const auto &alt = this->m_stor.as<S_branch>();
        // Bind both branches recursively.
        auto branch_true_bnd = alt.branch_true.bind(global, ctx);
        auto branch_false_bnd = alt.branch_false.bind(global, ctx);
        Xpnode::S_branch alt_bnd = { alt.assign, std::move(branch_true_bnd), std::move(branch_false_bnd) };
        return std::move(alt_bnd);
      }
      case index_function_call: {
        const auto &alt = this->m_stor.as<S_function_call>();
        // Copy it as-is.
        Xpnode::S_function_call alt_bnd = { alt.loc, alt.arg_cnt };
        return std::move(alt_bnd);
      }
      case index_subscript: {
        const auto &alt = this->m_stor.as<S_subscript>();
        // Copy it as-is.
        Xpnode::S_subscript alt_bnd = { alt.name };
        return std::move(alt_bnd);
      }
      case index_operator_rpn: {
        const auto &alt = this->m_stor.as<S_operator_rpn>();
        // Copy it as-is.
        Xpnode::S_operator_rpn alt_bnd = { alt.xop, alt.assign };
        return std::move(alt_bnd);
      }
      case index_unnamed_array: {
        const auto &alt = this->m_stor.as<S_unnamed_array>();
        // Copy it as-is.
        Xpnode::S_unnamed_array alt_bnd = { alt.elem_cnt };
        return std::move(alt_bnd);
      }
      case index_unnamed_object: {
        const auto &alt = this->m_stor.as<S_unnamed_object>();
        // Copy it as-is.
        Xpnode::S_unnamed_object alt_bnd = { alt.keys };
        return std::move(alt_bnd);
      }
      case index_coalescence: {
        const auto &alt = this->m_stor.as<S_coalescence>();
        // Bind the null branch recursively.
        auto branch_null_bnd = alt.branch_null.bind(global, ctx);
        Xpnode::S_coalescence alt_bnd = { alt.assign, std::move(branch_null_bnd) };
        return std::move(alt_bnd);
      }
      default: {
        ASTERIA_TERMINATE("An unknown expression node type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

    namespace {

    bool do_logical_not(bool rhs)
      {
        return !rhs;
      }

    bool do_logical_and(bool lhs, bool rhs)
      {
        return lhs & rhs;
      }

    bool do_logical_or(bool lhs, bool rhs)
      {
        return lhs | rhs;
      }

    bool do_logical_xor(bool lhs, bool rhs)
      {
        return lhs ^ rhs;
      }

    std::int64_t do_negate(std::int64_t rhs, bool wrap)
      {
        if(!wrap && (rhs == INT64_MIN)) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral negation of `", rhs, "` would result in overflow.");
        }
        auto res = static_cast<std::uint64_t>(rhs);
        res = -res;
        return static_cast<std::int64_t>(res);
      }

    std::int64_t do_add(std::int64_t lhs, std::int64_t rhs)
      {
        if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs)) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral addition of `", lhs, "` and `", rhs, "` would result in overflow.");
        }
        return lhs + rhs;
      }

    std::int64_t do_subtract(std::int64_t lhs, std::int64_t rhs)
      {
        if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs)) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral subtraction of `", lhs, "` and `", rhs, "` would result in overflow.");
        }
        return lhs - rhs;
      }

    std::int64_t do_multiply(std::int64_t lhs, std::int64_t rhs)
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
        const auto slhs = (rhs >= 0) ? lhs : -lhs;
        const auto arhs = std::abs(rhs);
        if((slhs >= 0) ? (slhs > INT64_MAX / arhs) : (slhs < INT64_MIN / arhs)) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication of `", lhs, "` and `", rhs, "` would result in overflow.");
        }
        return slhs * arhs;
      }

    std::int64_t do_divide(std::int64_t lhs, std::int64_t rhs)
      {
        if(rhs == 0) {
          ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
        }
        if((lhs == INT64_MIN) && (rhs == -1)) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral division of `", lhs, "` and `", rhs, "` would result in overflow.");
        }
        return lhs / rhs;
      }

    std::int64_t do_modulo(std::int64_t lhs, std::int64_t rhs)
      {
        if(rhs == 0) {
          ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
        }
        if((lhs == INT64_MIN) && (rhs == -1)) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral division of `", lhs, "` and `", rhs, "` would result in overflow.");
        }
        return lhs % rhs;
      }

    std::int64_t do_shift_left_logical(std::int64_t lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return 0;
        }
        auto res = static_cast<std::uint64_t>(lhs);
        res <<= rhs;
        return static_cast<std::int64_t>(res);
      }

    std::int64_t do_shift_right_logical(std::int64_t lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return 0;
        }
        auto res = static_cast<std::uint64_t>(lhs);
        res >>= rhs;
        return static_cast<std::int64_t>(res);
      }

    std::int64_t do_shift_left_arithmetic(std::int64_t lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        auto res = static_cast<std::uint64_t>(lhs);
        if(res == 0) {
          return 0;
        }
        if(rhs >= 64) {
          ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
        }
        const auto bits_rem = static_cast<unsigned char>(63 - rhs);
        const auto mask_out = (res >> bits_rem) << bits_rem;
        const auto mask_sign = -(res >> 63) << bits_rem;
        if(mask_out != mask_sign) {
          ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
        }
        res <<= rhs;
        return static_cast<std::int64_t>(res);
      }

    std::int64_t do_shift_right_arithmetic(std::int64_t lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        auto res = static_cast<std::uint64_t>(lhs);
        if(res == 0) {
          return 0;
        }
        if(rhs >= 64) {
          return 0;
        }
        const auto bits_rem = static_cast<unsigned char>(63 - rhs);
        const auto mask_in = -(res >> 63) << bits_rem;
        res >>= rhs;
        res |= mask_in;
        return static_cast<std::int64_t>(res);
      }

    std::int64_t do_bitwise_not(std::int64_t rhs)
      {
        return ~rhs;
      }

    std::int64_t do_bitwise_and(std::int64_t lhs, std::int64_t rhs)
      {
        return lhs & rhs;
      }

    std::int64_t do_bitwise_or(std::int64_t lhs, std::int64_t rhs)
      {
        return lhs | rhs;
      }

    std::int64_t do_bitwise_xor(std::int64_t lhs, std::int64_t rhs)
      {
        return lhs ^ rhs;
      }

    double do_negate(double rhs)
      {
        return -rhs;
      }

    double do_add(double lhs, double rhs)
      {
        return lhs + rhs;
      }

    double do_subtract(double lhs, double rhs)
      {
        return lhs - rhs;
      }

    double do_multiply(double lhs, double rhs)
      {
        return lhs * rhs;
      }

    double do_divide(double lhs, double rhs)
      {
        return lhs / rhs;
      }

    double do_modulo(double lhs, double rhs)
      {
        return std::fmod(lhs, rhs);
      }

    rocket::cow_string do_concatenate(const rocket::cow_string &lhs, const rocket::cow_string &rhs)
      {
        rocket::cow_string res;
        res.reserve(lhs.size() + rhs.size());
        res.append(lhs);
        res.append(rhs);
        return res;
      }

    rocket::cow_string do_duplicate(const rocket::cow_string &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String duplication count `", rhs, "` for `", lhs, "` is negative.");
        }
        rocket::cow_string res;
        const auto count = static_cast<std::uint64_t>(rhs);
        if(count != 0) {
          if(lhs.size() > res.max_size() / count) {
            ASTERIA_THROW_RUNTIME_ERROR("Duplication of `", lhs, "` up to `", rhs, "` times would result in an overlong string that cannot be allocated.");
          }
          res.reserve(lhs.size() * static_cast<std::size_t>(count));
          auto mask = std::numeric_limits<std::size_t>::max() / 2 + 1;
          do {
            if(count & mask) {
              res.append(lhs);
            }
            if((mask >>= 1) == 0) {
              break;
            }
            res.append(res);
          } while(true);
        }
        return res;
      }

    rocket::cow_string do_move_left(const rocket::cow_string &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        rocket::cow_string res;
        res.assign(lhs.size(), ' ');
        const auto count = static_cast<std::uint64_t>(rhs);
        if(count >= lhs.size()) {
          return res;
        }
        const auto bytes_rem = lhs.size() - static_cast<std::size_t>(count);
        lhs.copy(res.mut_data(), bytes_rem, count);
        return res;
      }

    rocket::cow_string do_move_right(const rocket::cow_string &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        rocket::cow_string res;
        res.assign(lhs.size(), ' ');
        const auto count = static_cast<std::uint64_t>(rhs);
        if(count >= lhs.size()) {
          return res;
        }
        const auto bytes_rem = lhs.size() - static_cast<std::size_t>(count);
        lhs.copy(res.mut_data() + count, bytes_rem);
        return res;
      }

    rocket::cow_string do_extend(const rocket::cow_string &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        rocket::cow_string res;
        const auto count = static_cast<std::uint64_t>(rhs);
        if(count > res.max_size() - lhs.size()) {
          ASTERIA_THROW_RUNTIME_ERROR("Shifting `", lhs, "` to the left by `", rhs, "` bytes would result in an overlong string that cannot be allocated.");
        }
        res.reserve(lhs.size() + static_cast<std::size_t>(count));
        res.append(lhs);
        res.append(static_cast<std::size_t>(count), ' ');
        return res;
      }

    rocket::cow_string do_truncate(const rocket::cow_string &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        rocket::cow_string res;
        const auto count = static_cast<std::uint64_t>(rhs);
        if(count >= lhs.size()) {
          return res;
        }
        res.append(lhs.data(), lhs.size() - static_cast<std::size_t>(count));
        return res;
      }

    }

void Xpnode::evaluate(Reference_stack &stack_io, Global_context &global, const Executive_context &ctx) const
  {
    switch(Index(this->m_stor.index())) {
      case index_literal: {
        const auto &alt = this->m_stor.as<S_literal>();
        // Push the constant.
        Reference_root::S_constant ref_c = { alt.value };
        stack_io.push(std::move(ref_c));
        return;
      }
      case index_named_reference: {
        const auto &alt = this->m_stor.as<S_named_reference>();
        // Look for the reference in the current context.
        auto pair = do_name_lookup(global, ctx, alt.name);
        if(pair.first.get().is_analytic()) {
          ASTERIA_THROW_RUNTIME_ERROR("Expressions cannot be evaluated in analytic contexts.");
        }
        // Push the reference found.
        stack_io.push(pair.second);
        return;
      }
      case index_bound_reference: {
        const auto &alt = this->m_stor.as<S_bound_reference>();
        // Push the reference stored.
        stack_io.push(alt.ref);
        return;
      }
      case index_closure_function: {
        const auto &alt = this->m_stor.as<S_closure_function>();
        // Instantiate the closure function.
        auto func = alt.body.instantiate_function(global, ctx, alt.head);
        Reference_root::S_temporary ref_c = { D_function(std::move(func)) };
        stack_io.push(std::move(ref_c));
        return;
      }
      case index_branch: {
        const auto &alt = this->m_stor.as<S_branch>();
        // Pick a branch basing on the condition.
        const auto branch = stack_io.top().read().test() ? std::ref(alt.branch_true) : std::ref(alt.branch_false);
        const auto has_result = branch.get().evaluate_partial(stack_io, global, ctx);
        if(!has_result) {
          // If the branch is empty, leave the condition on the stack.
          return;
        }
        // The result will have been pushed onto `stack_io`.
        ASTERIA_DEBUG_LOG("Setting branch result: ", stack_io.top().read());
        if(alt.assign) {
          auto value = stack_io.top().read();
          stack_io.pop();
          stack_io.top().write(std::move(value));
        } else {
          auto result = std::move(stack_io.top());
          stack_io.pop();
          stack_io.top() = std::move(result);
        }
        return;
      }
      case index_function_call: {
        const auto &alt = this->m_stor.as<S_function_call>();
        // Allocate the argument vector.
        rocket::cow_vector<Reference> args;
        args.resize(alt.arg_cnt);
        for(auto i = alt.arg_cnt - 1; i + 1 != 0; --i) {
          args.mut(i) = std::move(stack_io.top());
          stack_io.pop();
        }
        // Pop the target off the stack.
        auto self = std::move(stack_io.top());
        const auto tgt_value = self.read();
        self.zoom_out();
        // Make sure it is really a function.
        if(tgt_value.type() != Value::type_function) {
          ASTERIA_THROW_RUNTIME_ERROR("`", tgt_value, "` is not a function and cannot be called.");
        }
        const auto &func = tgt_value.check<D_function>();
        ASTERIA_DEBUG_LOG("Initiating function call at \'", alt.loc, "\':\n", func->describe());
        try {
          func->invoke(stack_io.top(), global, std::move(self), std::move(args));
          ASTERIA_DEBUG_LOG("Returned from function call at \'", alt.loc, "\'.");
        } catch(Exception &except) {
          ASTERIA_DEBUG_LOG("Caught `Asteria::Exception` thrown inside function call at \'", alt.loc, "\': value = ", except.get_value());
          // Append backtrace information and rethrow the exception.
          except.append_backtrace(alt.loc);
          throw;
        } catch(std::exception &stdex) {
          ASTERIA_DEBUG_LOG("Caught `std::exception` thrown inside function call at \'", alt.loc, "\': what = ", stdex.what());
          // Here we behave as if a `string` had been thrown.
          Exception except(stdex);
          except.append_backtrace(alt.loc);
          throw except;
        }
        return;
      }
      case index_subscript: {
        const auto &alt = this->m_stor.as<S_subscript>();
        // Get the subscript.
        Value subscript;
        if(alt.name.empty()) {
          subscript = stack_io.top().read();
          stack_io.pop();
        } else {
          subscript = D_string(alt.name);
        }
        // The subscript shall have type `integer` or `string`.
        switch(rocket::weaken_enum(subscript.type())) {
          case Value::type_integer: {
            Reference_modifier::S_array_index mod_c = { subscript.check<D_integer>() };
            stack_io.top().zoom_in(std::move(mod_c));
            break;
          }
          case Value::type_string: {
            Reference_modifier::S_object_key mod_c = { rocket::prehashed_string(subscript.check<D_string>()) };
            stack_io.top().zoom_in(std::move(mod_c));
            break;
          }
          default: {
            ASTERIA_THROW_RUNTIME_ERROR("The value `", subscript, "` cannot be used as a subscript.");
          }
        }
        return;
      }
      case index_operator_rpn: {
        const auto &alt = this->m_stor.as<S_operator_rpn>();
        const auto do_set_temporary =
          [&](Reference_root::S_temporary &&ref_c)
            {
              if(alt.assign) {
                stack_io.top().write(std::move(ref_c.value));
              } else {
                stack_io.top() = std::move(ref_c);
              }
            };
        switch(alt.xop) {
          case xop_postfix_inc: {
            // Increment the operand and return the old value.
            // `assign` is ignored.
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if(ref_c.value.type() == Value::type_integer) {
              stack_io.top().write(do_add(ref_c.value.check<D_integer>(), D_integer(1)));
              stack_io.top() = std::move(ref_c);
              break;
            }
            if(ref_c.value.type() == Value::type_real) {
              stack_io.top().write(do_add(ref_c.value.check<D_real>(), D_real(1)));
              stack_io.top() = std::move(ref_c);
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
          }
          case xop_postfix_dec: {
            // Decrement the operand and return the old value.
            // `assign` is ignored.
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if(ref_c.value.type() == Value::type_integer) {
              stack_io.top().write(do_subtract(ref_c.value.check<D_integer>(), D_integer(1)));
              stack_io.top() = std::move(ref_c);
              break;
            }
            if(ref_c.value.type() == Value::type_real) {
              stack_io.top().write(do_subtract(ref_c.value.check<D_real>(), D_real(1)));
              stack_io.top() = std::move(ref_c);
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
          }
          case xop_prefix_pos: {
            // Copy the operand to create a temporary value, then return it.
            // N.B. This is one of the few operators that work on all types.
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            do_set_temporary(std::move(ref_c));
            break;
          }
          case xop_prefix_neg: {
            // Negate the operand to create a temporary value, then return it.
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if(ref_c.value.type() == Value::type_integer) {
              ref_c.value = do_negate(ref_c.value.check<D_integer>(), stack_io.top().is_constant());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if(ref_c.value.type() == Value::type_real) {
              ref_c.value = do_negate(ref_c.value.check<D_real>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
          }
          case xop_prefix_notb: {
            // Perform bitwise NOT operation on the operand to create a temporary value, then return it.
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if(ref_c.value.type() == Value::type_boolean) {
              ref_c.value = do_logical_not(ref_c.value.check<D_boolean>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if(ref_c.value.type() == Value::type_integer) {
              ref_c.value = do_bitwise_not(ref_c.value.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
          }
          case xop_prefix_notl: {
            // Perform logical NOT operation on the operand to create a temporary value, then return it.
            // N.B. This is one of the few operators that work on all types.
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            ref_c.value = do_logical_not(ref_c.value.test());
            do_set_temporary(std::move(ref_c));
            break;
          }
          case xop_prefix_inc: {
            // Increment the operand and return it.
            // `assign` is ignored.
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if(ref_c.value.type() == Value::type_integer) {
              stack_io.top().write(do_add(ref_c.value.check<D_integer>(), D_integer(1)));
              break;
            }
            if(ref_c.value.type() == Value::type_real) {
              stack_io.top().write(do_add(ref_c.value.check<D_real>(), D_real(1)));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
          }
          case xop_prefix_dec: {
            // Decrement the operand and return it.
            // `assign` is ignored.
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if(ref_c.value.type() == Value::type_integer) {
              stack_io.top().write(do_subtract(ref_c.value.check<D_integer>(), D_integer(1)));
              stack_io.top() = std::move(ref_c);
              break;
            }
            if(ref_c.value.type() == Value::type_real) {
              stack_io.top().write(do_subtract(ref_c.value.check<D_real>(), D_real(1)));
              stack_io.top() = std::move(ref_c);
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
          }
          case xop_prefix_unset: {
            // Unset the reference and return the value removed.
            Reference_root::S_temporary ref_c = { stack_io.top().unset() };
            do_set_temporary(std::move(ref_c));
            break;
          }
          case xop_prefix_lengthof: {
            // Return the number of elements in `rhs`.
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if(ref_c.value.type() == Value::type_array) {
              ref_c.value = D_integer(ref_c.value.check<D_array>().size());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if(ref_c.value.type() == Value::type_object) {
              ref_c.value = D_integer(ref_c.value.check<D_object>().size());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
          }
          case xop_infix_cmp_eq: {
            // Report unordered operands as being unequal.
            // N.B. This is one of the few operators that work on all types.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            const auto comp = ref_c.value.compare(rhs);
            ref_c.value = comp == Value::compare_equal;
            do_set_temporary(std::move(ref_c));
            break;
          }
          case xop_infix_cmp_ne: {
            // Report unordered operands as being unequal.
            // N.B. This is one of the few operators that work on all types.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            const auto comp = ref_c.value.compare(rhs);
            ref_c.value = comp != Value::compare_equal;
            do_set_temporary(std::move(ref_c));
            break;
          }
          case xop_infix_cmp_lt: {
            // Throw an exception in case of unordered operands.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            const auto comp = ref_c.value.compare(rhs);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", ref_c.value, "` and `", rhs, "` are uncomparable.");
            }
            ref_c.value = comp == Value::compare_less;
            do_set_temporary(std::move(ref_c));
            break;
          }
          case xop_infix_cmp_gt: {
            // Throw an exception in case of unordered operands.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            const auto comp = ref_c.value.compare(rhs);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", ref_c.value, "` and `", rhs, "` are uncomparable.");
            }
            ref_c.value = comp == Value::compare_greater;
            do_set_temporary(std::move(ref_c));
            break;
          }
          case xop_infix_cmp_lte: {
            // Throw an exception in case of unordered operands.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            const auto comp = ref_c.value.compare(rhs);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", ref_c.value, "` and `", rhs, "` are uncomparable.");
            }
            ref_c.value = comp != Value::compare_greater;
            do_set_temporary(std::move(ref_c));
            break;
          }
          case xop_infix_cmp_gte: {
            // Throw an exception in case of unordered operands.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            const auto comp = ref_c.value.compare(rhs);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", ref_c.value, "` and `", rhs, "` are uncomparable.");
            }
            ref_c.value = comp != Value::compare_less;
            do_set_temporary(std::move(ref_c));
            break;
          }
          case xop_infix_cmp_3way: {
            // N.B. This is one of the few operators that work on all types.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            const auto comp = ref_c.value.compare(rhs);
            switch(comp) {
              case Value::compare_unordered: {
                ref_c.value = D_string(D_string::shallow("unordered"));
                break;
              }
              case Value::compare_less: {
                ref_c.value = D_integer(-1);
                break;
              }
              case Value::compare_equal: {
                ref_c.value = D_integer(0);
                break;
              }
              case Value::compare_greater: {
                ref_c.value = D_integer(+1);
                break;
              }
              default: {
                ASTERIA_TERMINATE("An unknown comparison result `", comp, "` has been encountered.");
              }
            }
            do_set_temporary(std::move(ref_c));
            break;
          }
          case xop_infix_add: {
            // For the `boolean` type, return the logical OR'd result of both operands.
            // For the `integer` and `real` types, return the sum of both operands.
            // For the `string` type, concatenate the operands in lexical order to create a new string, then return it.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if((ref_c.value.type() == Value::type_boolean) && (rhs.type() == Value::type_boolean)) {
              ref_c.value = do_logical_or(ref_c.value.check<D_boolean>(), rhs.check<D_boolean>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_add(ref_c.value.check<D_integer>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_real) && (rhs.type() == Value::type_real)) {
              ref_c.value = do_add(ref_c.value.check<D_real>(), rhs.check<D_real>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_string) && (rhs.type() == Value::type_string)) {
              ref_c.value = do_concatenate(ref_c.value.check<D_string>(), rhs.check<D_string>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "` and `", rhs, "`.");
          }
          case xop_infix_sub: {
            // For the `boolean` type, return the logical XOR'd result of both operands.
            // For the `integer` and `real` types, return the difference of both operands.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if((ref_c.value.type() == Value::type_boolean) && (rhs.type() == Value::type_boolean)) {
              ref_c.value = do_logical_xor(ref_c.value.check<D_boolean>(), rhs.check<D_boolean>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_subtract(ref_c.value.check<D_integer>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_real) && (rhs.type() == Value::type_real)) {
              ref_c.value = do_subtract(ref_c.value.check<D_real>(), rhs.check<D_real>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "` and `", rhs, "`.");
          }
          case xop_infix_mul: {
            // For type `boolean`, return the logical AND'd result of both operands.
            // For types `integer` and `real`, return the product of both operands.
            // If either operand has type `string` and the other has type `integer`, duplicate the string up to the specified number of times.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if((ref_c.value.type() == Value::type_boolean) && (rhs.type() == Value::type_boolean)) {
              ref_c.value = do_logical_and(ref_c.value.check<D_boolean>(), rhs.check<D_boolean>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_multiply(ref_c.value.check<D_integer>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_real) && (rhs.type() == Value::type_real)) {
              ref_c.value = do_multiply(ref_c.value.check<D_real>(), rhs.check<D_real>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_string) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_duplicate(ref_c.value.check<D_string>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_string)) {
              ref_c.value = do_duplicate(rhs.check<D_string>(), ref_c.value.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "` and `", rhs, "`.");
          }
          case xop_infix_div: {
            // For types `integer` and `real`, return the quotient of both operands.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_divide(ref_c.value.check<D_integer>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_real) && (rhs.type() == Value::type_real)) {
              ref_c.value = do_divide(ref_c.value.check<D_real>(), rhs.check<D_real>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "` and `", rhs, "`.");
          }
          case xop_infix_mod: {
            // For types `integer` and `real`, return the reminder of both operands.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_modulo(ref_c.value.check<D_integer>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_real) && (rhs.type() == Value::type_real)) {
              ref_c.value = do_modulo(ref_c.value.check<D_real>(), rhs.check<D_real>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "` and `", rhs, "`.");
          }
          case xop_infix_sll: {
            // The second operand shall be an integer.
            // If the first operand is of type `integer`, shift the first operand to the left by the number of bits specified by the second operand. Bits shifted out
            // are discarded. Bits shifted in are filled with zeroes.
            // If the first operand is of type `string`, fill space bytes in the right and discard charactrs from the left. The number of bytes in the first operand
            // will not be changed.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_shift_left_logical(ref_c.value.check<D_integer>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_string) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_move_left(ref_c.value.check<D_string>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "` and `", rhs, "`.");
          }
          case xop_infix_srl: {
            // The second operand shall be an integer.
            // If the first operand is of type `integer`, shift the first operand to the right by the number of bits specified by the second operand. Bits shifted out
            // are discarded. Bits shifted in are filled with zeroes.
            // If the first operand is of type `string`, fill space bytes in the left and discard charactrs from the right. The number of bytes in the first operand
            // will not be changed.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_shift_right_logical(ref_c.value.check<D_integer>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_string) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_move_right(ref_c.value.check<D_string>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "` and `", rhs, "`.");
          }
          case xop_infix_sla: {
            // The second operand shall be an integer.
            // If the first operand is of type `integer`, shift the first operand to the left by the number of bits specified by the second operand. Bits shifted out
            // that are equal to the sign bit are discarded. Bits shifted out that are unequal to the sign bit lead to an exception being thrown. Bits shifted in are
            // filled with zeroes.
            // If the first operand is of type `string`, fill space bytes in the right.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_shift_left_arithmetic(ref_c.value.check<D_integer>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_string) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_extend(ref_c.value.check<D_string>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "` and `", rhs, "`.");
          }
          case xop_infix_sra: {
            // The second operand shall be an integer.
            // If the first operand is of type `integer`, shift the first operand to the right by the number of bits specified by the second operand. Bits shifted out
            // are discarded. Bits shifted in are filled with the sign bit.
            // If the first operand is of type `string`, discard bytes from the right.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_shift_right_arithmetic(ref_c.value.check<D_integer>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_string) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_truncate(ref_c.value.check<D_string>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "` and `", rhs, "`.");
          }
          case xop_infix_andb: {
            // For the `boolean` type, return the logical AND'd result of both operands.
            // For the `integer` type, return the bitwise AND'd result of both operands.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if((ref_c.value.type() == Value::type_boolean) && (rhs.type() == Value::type_boolean)) {
              ref_c.value = do_logical_and(ref_c.value.check<D_boolean>(), rhs.check<D_boolean>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_bitwise_and(ref_c.value.check<D_integer>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "` and `", rhs, "`.");
          }
          case xop_infix_orb: {
            // For the `boolean` type, return the logical OR'd result of both operands.
            // For the `integer` type, return the bitwise OR'd result of both operands.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if((ref_c.value.type() == Value::type_boolean) && (rhs.type() == Value::type_boolean)) {
              ref_c.value = do_logical_or(ref_c.value.check<D_boolean>(), rhs.check<D_boolean>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_bitwise_or(ref_c.value.check<D_integer>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "` and `", rhs, "`.");
          }
          case xop_infix_xorb: {
            // For the `boolean` type, return the logical XOR'd result of both operands.
            // For the `integer` type, return the bitwise XOR'd result of both operands.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            Reference_root::S_temporary ref_c = { stack_io.top().read() };
            if((ref_c.value.type() == Value::type_boolean) && (rhs.type() == Value::type_boolean)) {
              ref_c.value = do_logical_xor(ref_c.value.check<D_boolean>(), rhs.check<D_boolean>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            if((ref_c.value.type() == Value::type_integer) && (rhs.type() == Value::type_integer)) {
              ref_c.value = do_bitwise_xor(ref_c.value.check<D_integer>(), rhs.check<D_integer>());
              do_set_temporary(std::move(ref_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "` and `", rhs, "`.");
          }
          case xop_infix_assign: {
            // Copy the operand referenced by `rhs` to `lhs`.
            // `assign` is ignored.
            auto rhs = stack_io.top().read();
            stack_io.pop();
            stack_io.top().write(std::move(rhs));
            break;
          }
          default: {
            ASTERIA_TERMINATE("An unknown operator type enumeration `", alt.xop, "` has been encountered.");
          }
        }
        return;
      }
      case index_unnamed_array: {
        const auto &alt = this->m_stor.as<S_unnamed_array>();
        // Pop references to create an array.
        D_array array;
        array.resize(alt.elem_cnt);
        for(auto i = alt.elem_cnt - 1; i + 1 != 0; --i) {
          array.mut(i) = stack_io.top().read();
          stack_io.pop();
        }
        Reference_root::S_temporary ref_c = { std::move(array) };
        stack_io.push(std::move(ref_c));
        return;
      }
      case index_unnamed_object: {
        const auto &alt = this->m_stor.as<S_unnamed_object>();
        // Pop references to create an object.
        D_object object;
        object.reserve(alt.keys.size());
        for(auto it = alt.keys.rbegin(); it != alt.keys.rend(); ++it) {
          object.try_emplace(*it, stack_io.top().read());
          stack_io.pop();
        }
        Reference_root::S_temporary ref_c = { std::move(object) };
        stack_io.push(std::move(ref_c));
        return;
      }
      case index_coalescence: {
        const auto &alt = this->m_stor.as<S_coalescence>();
        // Pick a branch basing on the condition.
        if(stack_io.top().read().type() != Value::type_null) {
          // If the result is non-null, leave the condition on the stack.
          return;
        }
        const auto has_result = alt.branch_null.evaluate_partial(stack_io, global, ctx);
        if(!has_result) {
          // If the branch is empty, leave the condition on the stack.
          return;
        }
        // The result will have been pushed onto `stack_io`.
        ASTERIA_DEBUG_LOG("Setting coalescence result: ", stack_io.top().read());
        if(alt.assign) {
          auto value = stack_io.top().read();
          stack_io.pop();
          stack_io.top().write(std::move(value));
        } else {
          auto result = std::move(stack_io.top());
          stack_io.pop();
          stack_io.top() = std::move(result);
        }
        return;
      }
      default: {
        ASTERIA_TERMINATE("An unknown expression node type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

void Xpnode::enumerate_variables(const Abstract_variable_callback &callback) const
  {
    switch(Index(this->m_stor.index())) {
      case index_literal: {
        const auto &alt = this->m_stor.as<S_literal>();
        alt.value.enumerate_variables(callback);
        return;
      }
      case index_named_reference: {
        return;
      }
      case index_bound_reference: {
        const auto &alt = this->m_stor.as<S_bound_reference>();
        alt.ref.enumerate_variables(callback);
        return;
      }
      case index_closure_function: {
        const auto &alt = this->m_stor.as<S_closure_function>();
        alt.body.enumerate_variables(callback);
        return;
      }
      case index_branch: {
        const auto &alt = this->m_stor.as<S_branch>();
        alt.branch_true.enumerate_variables(callback);
        alt.branch_false.enumerate_variables(callback);
        return;
      }
      case index_function_call:
      case index_subscript:
      case index_operator_rpn:
      case index_unnamed_array:
      case index_unnamed_object: {
        return;
      }
      case index_coalescence: {
        const auto &alt = this->m_stor.as<S_coalescence>();
        alt.branch_null.enumerate_variables(callback);
        return;
      }
      default: {
        ASTERIA_TERMINATE("An unknown expression node type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

}
