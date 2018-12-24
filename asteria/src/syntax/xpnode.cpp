// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "../runtime/reference_stack.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/executive_context.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/abstract_function.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../runtime/exception.hpp"
#include "../utilities.hpp"

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
      case xop_prefix_typeof: {
        return "prefix `typeof`";
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

    template<typename ContextT>
      std::pair<std::reference_wrapper<const Abstract_context>,
        std::reference_wrapper<const Reference>> do_name_lookup(const Global_context &global, const ContextT &ctx, const rocket::prehashed_string &name)
      {
        auto qctx = static_cast<const Abstract_context *>(&ctx);
        // De-virtualize the first call by hand.
        auto qref = ctx.ContextT::get_named_reference_opt(name);
        for(;;) {
          if(ROCKET_EXPECT(qref)) {
            return std::make_pair(std::ref(*qctx), std::ref(*qref));
          }
          // Search for the name in deeper contexts.
          qctx = qctx->get_parent_opt();
          if(!qctx) {
            break;
          }
          qref = qctx->get_named_reference_opt(name);
        }
        // Search for the name in the global context.
        qref = global.Global_context::get_named_reference_opt(name);
        if(ROCKET_EXPECT(qref)) {
          return std::make_pair(std::ref(*qctx), std::ref(*qref));
        }
        // Not found...
        ASTERIA_THROW_RUNTIME_ERROR("The identifier `", name, "` has not been declared yet.");
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
        ctx_next.initialize_for_function(alt.params);
        auto body_bnd = alt.body.bind_in_place(ctx_next, global);
        Xpnode::S_closure_function alt_bnd = { alt.loc, alt.params, std::move(body_bnd) };
        return std::move(alt_bnd);
      }
      case index_branch: {
        const auto &alt = this->m_stor.as<S_branch>();
        // Bind both branches recursively.
        auto branch_true_bnd = alt.branch_true.bind(global, ctx);
        auto branch_false_bnd = alt.branch_false.bind(global, ctx);
        Xpnode::S_branch alt_bnd = { std::move(branch_true_bnd), std::move(branch_false_bnd), alt.assign };
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
        Xpnode::S_coalescence alt_bnd = { std::move(branch_null_bnd), alt.assign };
        return std::move(alt_bnd);
      }
      default: {
        ASTERIA_TERMINATE("An unknown expression node type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

    namespace {

    void do_evaluate_literal(const Xpnode::S_literal &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        // Push the constant.
        Reference_root::S_constant ref_c = { alt.value };
        stack_io.push(std::move(ref_c));
      }

    void do_evaluate_named_reference(const Xpnode::S_named_reference &alt, Reference_stack &stack_io, Global_context &global, const Executive_context &ctx)
      {
        // Look for the reference in the current context.
        auto pair = do_name_lookup(global, ctx, alt.name);
#ifdef ROCKET_DEBUG
        ROCKET_ASSERT(!pair.first.get().is_analytic());
#endif
        // Push the reference found.
        stack_io.push(pair.second);
      }

    void do_evaluate_bound_reference(const Xpnode::S_bound_reference &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        // Push the reference stored.
        stack_io.push(alt.ref);
      }

    void do_evaluate_closure_function(const Xpnode::S_closure_function &alt, Reference_stack &stack_io, Global_context &global, const Executive_context &ctx)
      {
        // Instantiate the closure function.
        auto func = alt.body.instantiate_function(global, ctx, alt.loc, std::ref("<closure function>"), alt.params);
        Reference_root::S_temporary ref_c = { D_function(std::move(func)) };
        stack_io.push(std::move(ref_c));
      }

    void do_evaluate_branch(const Xpnode::S_branch &alt, Reference_stack &stack_io, Global_context &global, const Executive_context &ctx)
      {
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
          stack_io.top().open() = std::move(value);
        } else {
          auto result = std::move(stack_io.mut_top());
          stack_io.pop();
          stack_io.mut_top() = std::move(result);
        }
      }

    void do_evaluate_function_call(const Xpnode::S_function_call &alt, Reference_stack &stack_io, Global_context &global, const Executive_context & /*ctx*/)
      {
        // Allocate the argument vector.
        rocket::cow_vector<Reference> args(alt.arg_cnt);
        for(auto i = alt.arg_cnt - 1; i + 1 != 0; --i) {
          args.mut(i) = std::move(stack_io.mut_top());
          stack_io.pop();
        }
        // Pop the target off the stack.
        const auto tgt_value = stack_io.top().read();
        // Make sure it is really a function.
        if(tgt_value.type() != Value::type_function) {
          ASTERIA_THROW_RUNTIME_ERROR("`", tgt_value, "` is not a function and cannot be called.");
        }
        const auto &func = tgt_value.check<D_function>();
        ASTERIA_DEBUG_LOG("Initiating function call at \'", alt.loc, "\':\n", func->describe());
        try {
          // Call the function now.
          func->invoke(stack_io.mut_top(), global, std::move(args));
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
      }

    void do_evaluate_subscript(const Xpnode::S_subscript &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
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
            stack_io.mut_top().zoom_in(std::move(mod_c));
            break;
          }
          case Value::type_string: {
            Reference_modifier::S_object_key mod_c = { rocket::prehashed_string(subscript.check<D_string>()) };
            stack_io.mut_top().zoom_in(std::move(mod_c));
            break;
          }
          default: {
            ASTERIA_THROW_RUNTIME_ERROR("The value `", subscript, "` cannot be used as a subscript.");
          }
        }
      }

    void do_set_temporary(Reference_stack &stack_io, const Xpnode::S_operator_rpn &alt, Reference_root::S_temporary &&ref_c)
      {
        if(alt.assign) {
          stack_io.top().open() = std::move(ref_c.value);
        } else {
          stack_io.mut_top() = std::move(ref_c);
        }
      }

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

    std::int64_t do_negate(std::int64_t rhs)
      {
        if(rhs == INT64_MIN) {
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
          return static_cast<std::int64_t>(-(res >> 63));
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
        if(count == 0) {
          return res;
        }
        if(lhs.size() > res.max_size() / count) {
          ASTERIA_THROW_RUNTIME_ERROR("Duplication of `", lhs, "` up to `", rhs, "` times would result in an overlong string that cannot be allocated.");
        }
        res.reserve(lhs.size() * static_cast<std::size_t>(count));
        auto mask = std::numeric_limits<std::size_t>::max() / 2 + 1;
        for(;;) {
          if(count & mask) {
            res.append(lhs);
          }
          if((mask >>= 1) == 0) {
            break;
          }
          res.append(res);
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
        std::char_traits<char>::copy(res.mut_data(), lhs.data() + count, lhs.size() - static_cast<std::size_t>(count));
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
        std::char_traits<char>::copy(res.mut_data() + count, lhs.data(), lhs.size() - static_cast<std::size_t>(count));
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
        res.assign(lhs.size() + static_cast<std::size_t>(count), ' ');
        std::char_traits<char>::copy(res.mut_data(), lhs.data(), lhs.size());
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

    void do_evaluate_postfix_inc(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_postfix_inc);
        // Increment the operand and return the old value.
        // `alt.assign` is ignored.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        if(ref_c.value.type() == Value::type_integer) {
          stack_io.top().open() = do_add(ref_c.value.check<D_integer>(), D_integer(1));
          stack_io.mut_top() = std::move(ref_c);
          return;
        }
        if(ref_c.value.type() == Value::type_real) {
          stack_io.top().open() = do_add(ref_c.value.check<D_real>(), D_real(1));
          stack_io.mut_top() = std::move(ref_c);
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
      }

    void do_evaluate_postfix_dec(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_postfix_dec);
        // Decrement the operand and return the old value.
        // `alt.assign` is ignored.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        if(ref_c.value.type() == Value::type_integer) {
          stack_io.top().open() = do_subtract(ref_c.value.check<D_integer>(), D_integer(1));
          stack_io.mut_top() = std::move(ref_c);
          return;
        }
        if(ref_c.value.type() == Value::type_real) {
          stack_io.top().open() = do_subtract(ref_c.value.check<D_real>(), D_real(1));
          stack_io.mut_top() = std::move(ref_c);
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
      }

    void do_evaluate_prefix_pos(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_pos);
        // Copy the operand to create a temporary value, then return it.
        // N.B. This is one of the few operators that work on all types.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        do_set_temporary(stack_io, alt, std::move(ref_c));
      }

    void do_evaluate_prefix_neg(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_neg);
        // Negate the operand to create a temporary value, then return it.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        if(ref_c.value.type() == Value::type_integer) {
          ref_c.value = do_negate(ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if(ref_c.value.type() == Value::type_real) {
          ref_c.value = do_negate(ref_c.value.check<D_real>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
      }

    void do_evaluate_prefix_notb(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_notb);
        // Perform bitwise NOT operation on the operand to create a temporary value, then return it.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        if(ref_c.value.type() == Value::type_boolean) {
          ref_c.value = do_logical_not(ref_c.value.check<D_boolean>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if(ref_c.value.type() == Value::type_integer) {
          ref_c.value = do_bitwise_not(ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
      }

    void do_evaluate_prefix_notl(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_notl);
        // Perform logical NOT operation on the operand to create a temporary value, then return it.
        // N.B. This is one of the few operators that work on all types.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        ref_c.value = do_logical_not(ref_c.value.test());
        do_set_temporary(stack_io, alt, std::move(ref_c));
      }

    void do_evaluate_prefix_inc(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_inc);
        // Increment the operand and return it.
        // `alt.assign` is ignored.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        if(ref_c.value.type() == Value::type_integer) {
          stack_io.top().open() = do_add(ref_c.value.check<D_integer>(), D_integer(1));
          return;
        }
        if(ref_c.value.type() == Value::type_real) {
          stack_io.top().open() = do_add(ref_c.value.check<D_real>(), D_real(1));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
      }

    void do_evaluate_prefix_dec(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_dec);
        // Decrement the operand and return it.
        // `alt.assign` is ignored.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        if(ref_c.value.type() == Value::type_integer) {
          stack_io.top().open() = do_subtract(ref_c.value.check<D_integer>(), D_integer(1));
          stack_io.mut_top() = std::move(ref_c);
          return;
        }
        if(ref_c.value.type() == Value::type_real) {
          stack_io.top().open() = do_subtract(ref_c.value.check<D_real>(), D_real(1));
          stack_io.mut_top() = std::move(ref_c);
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
      }

    void do_evaluate_prefix_unset(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_unset);
        // Unset the reference and return the value removed.
        Reference_root::S_temporary ref_c = { stack_io.top().unset() };
        do_set_temporary(stack_io, alt, std::move(ref_c));
      }

    void do_evaluate_prefix_lengthof(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_lengthof);
        // Return the number of elements in the operand.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        if(ref_c.value.type() == Value::type_null) {
          ref_c.value = D_integer(0);
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if(ref_c.value.type() == Value::type_array) {
          ref_c.value = D_integer(ref_c.value.check<D_array>().size());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if(ref_c.value.type() == Value::type_object) {
          ref_c.value = D_integer(ref_c.value.check<D_object>().size());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
      }

    void do_evaluate_prefix_typeof(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_typeof);
        // Return the type name of the operand.
        // N.B. This is one of the few operators that work on all types.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        ref_c.value = D_string(std::ref(reinterpret_cast<const char (&)[]>(*(Value::get_type_name(ref_c.value.type())))));
        do_set_temporary(stack_io, alt, std::move(ref_c));
      }

    void do_evaluate_infix_cmp_eq(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_eq);
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        const auto comp = lhs.compare(ref_c.value);
        ref_c.value = comp == Value::compare_equal;
        do_set_temporary(stack_io, alt, std::move(ref_c));
      }

    void do_evaluate_infix_cmp_ne(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_ne);
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        const auto comp = lhs.compare(ref_c.value);
        ref_c.value = comp != Value::compare_equal;
        do_set_temporary(stack_io, alt, std::move(ref_c));
      }

    void do_evaluate_infix_cmp_lt(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_lt);
        // Throw an exception in case of unordered operands.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        const auto comp = lhs.compare(ref_c.value);
        if(comp == Value::compare_unordered) {
          ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs, "` and `", ref_c.value, "` are unordered.");
        }
        ref_c.value = comp == Value::compare_less;
        do_set_temporary(stack_io, alt, std::move(ref_c));
      }

    void do_evaluate_infix_cmp_gt(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_gt);
        // Throw an exception in case of unordered operands.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        const auto comp = lhs.compare(ref_c.value);
        if(comp == Value::compare_unordered) {
          ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs, "` and `", ref_c.value, "` are unordered.");
        }
        ref_c.value = comp == Value::compare_greater;
        do_set_temporary(stack_io, alt, std::move(ref_c));
      }

    void do_evaluate_infix_cmp_lte(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_lte);
        // Throw an exception in case of unordered operands.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        const auto comp = lhs.compare(ref_c.value);
        if(comp == Value::compare_unordered) {
          ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs, "` and `", ref_c.value, "` are unordered.");
        }
        ref_c.value = comp != Value::compare_greater;
        do_set_temporary(stack_io, alt, std::move(ref_c));
      }

    void do_evaluate_infix_cmp_gte(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_gte);
        // Throw an exception in case of unordered operands.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        const auto comp = lhs.compare(ref_c.value);
        if(comp == Value::compare_unordered) {
          ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs, "` and `", ref_c.value, "` are unordered.");
        }
        ref_c.value = comp != Value::compare_less;
        do_set_temporary(stack_io, alt, std::move(ref_c));
      }

    void do_evaluate_infix_cmp_3way(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_3way);
        // N.B. This is one of the few operators that work on all types.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        const auto comp = lhs.compare(ref_c.value);
        switch(rocket::weaken_enum(comp)) {
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
            ref_c.value = D_string(std::ref("unordered"));
            break;
          }
        }
        do_set_temporary(stack_io, alt, std::move(ref_c));
      }

    void do_evaluate_infix_add(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_add);
        // For the `boolean` type, return the logical OR'd result of both operands.
        // For the `integer` and `real` types, return the sum of both operands.
        // For the `string` type, concatenate the operands in lexical order to create a new string, then return it.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        if((lhs.type() == Value::type_boolean) && (ref_c.value.type() == Value::type_boolean)) {
          ref_c.value = do_logical_or(lhs.check<D_boolean>(), ref_c.value.check<D_boolean>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_add(lhs.check<D_integer>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_real) && (ref_c.value.type() == Value::type_real)) {
          ref_c.value = do_add(lhs.check<D_real>(), ref_c.value.check<D_real>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_string) && (ref_c.value.type() == Value::type_string)) {
          ref_c.value = do_concatenate(lhs.check<D_string>(), ref_c.value.check<D_string>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_infix_sub(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_sub);
        // For the `boolean` type, return the logical XOR'd result of both operands.
        // For the `integer` and `real` types, return the difference of both operands.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        if((lhs.type() == Value::type_boolean) && (ref_c.value.type() == Value::type_boolean)) {
          ref_c.value = do_logical_xor(lhs.check<D_boolean>(), ref_c.value.check<D_boolean>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_subtract(lhs.check<D_integer>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_real) && (ref_c.value.type() == Value::type_real)) {
          ref_c.value = do_subtract(lhs.check<D_real>(), ref_c.value.check<D_real>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_infix_mul(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_mul);
        // For type `boolean`, return the logical AND'd result of both operands.
        // For types `integer` and `real`, return the product of both operands.
        // If either operand has type `string` and the other has type `integer`, duplicate the string up to the specified number of times.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        if((lhs.type() == Value::type_boolean) && (ref_c.value.type() == Value::type_boolean)) {
          ref_c.value = do_logical_and(lhs.check<D_boolean>(), ref_c.value.check<D_boolean>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_multiply(lhs.check<D_integer>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_real) && (ref_c.value.type() == Value::type_real)) {
          ref_c.value = do_multiply(lhs.check<D_real>(), ref_c.value.check<D_real>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_string) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_duplicate(lhs.check<D_string>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_string)) {
          ref_c.value = do_duplicate(ref_c.value.check<D_string>(), lhs.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_infix_div(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_div);
        // For types `integer` and `real`, return the quotient of both operands.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_divide(lhs.check<D_integer>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_real) && (ref_c.value.type() == Value::type_real)) {
          ref_c.value = do_divide(lhs.check<D_real>(), ref_c.value.check<D_real>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_infix_mod(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_mod);
        // For types `integer` and `real`, return the reminder of both operands.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_modulo(lhs.check<D_integer>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_real) && (ref_c.value.type() == Value::type_real)) {
          ref_c.value = do_modulo(lhs.check<D_real>(), ref_c.value.check<D_real>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_infix_sll(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_sll);
        // The second operand shall be an integer.
        // If the first operand is of type `integer`, shift the first operand to the left by the number of bits specified by the second operand. Bits shifted out
        // are discarded. Bits shifted in are filled with zeroes.
        // If the first operand is of type `string`, fill space bytes in the right and discard charactrs from the left. The number of bytes in the first operand
        // will not be changed.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_shift_left_logical(lhs.check<D_integer>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_string) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_move_left(lhs.check<D_string>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_infix_srl(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_srl);
        // The second operand shall be an integer.
        // If the first operand is of type `integer`, shift the first operand to the right by the number of bits specified by the second operand. Bits shifted out
        // are discarded. Bits shifted in are filled with zeroes.
        // If the first operand is of type `string`, fill space bytes in the left and discard charactrs from the right. The number of bytes in the first operand
        // will not be changed.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_shift_right_logical(lhs.check<D_integer>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_string) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_move_right(lhs.check<D_string>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_infix_sla(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_sla);
        // The second operand shall be an integer.
        // If the first operand is of type `integer`, shift the first operand to the left by the number of bits specified by the second operand. Bits shifted out
        // that are equal to the sign bit are discarded. Bits shifted out that are unequal to the sign bit lead to an exception being thrown. Bits shifted in are
        // filled with zeroes.
        // If the first operand is of type `string`, fill space bytes in the right.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_shift_left_arithmetic(lhs.check<D_integer>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_string) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_extend(lhs.check<D_string>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_infix_sra(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_sra);
        // The second operand shall be an integer.
        // If the first operand is of type `integer`, shift the first operand to the right by the number of bits specified by the second operand. Bits shifted out
        // are discarded. Bits shifted in are filled with the sign bit.
        // If the first operand is of type `string`, discard bytes from the right.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_shift_right_arithmetic(lhs.check<D_integer>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_string) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_truncate(lhs.check<D_string>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_infix_andb(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_andb);
        // For the `boolean` type, return the logical AND'd result of both operands.
        // For the `integer` type, return the bitwise AND'd result of both operands.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        if((lhs.type() == Value::type_boolean) && (ref_c.value.type() == Value::type_boolean)) {
          ref_c.value = do_logical_and(lhs.check<D_boolean>(), ref_c.value.check<D_boolean>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_bitwise_and(lhs.check<D_integer>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_infix_orb(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_orb);
        // For the `boolean` type, return the logical OR'd result of both operands.
        // For the `integer` type, return the bitwise OR'd result of both operands.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        if((lhs.type() == Value::type_boolean) && (ref_c.value.type() == Value::type_boolean)) {
          ref_c.value = do_logical_or(lhs.check<D_boolean>(), ref_c.value.check<D_boolean>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_bitwise_or(lhs.check<D_integer>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_infix_xorb(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_xorb);
        // For the `boolean` type, return the logical XOR'd result of both operands.
        // For the `integer` type, return the bitwise XOR'd result of both operands.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        if((lhs.type() == Value::type_boolean) && (ref_c.value.type() == Value::type_boolean)) {
          ref_c.value = do_logical_xor(lhs.check<D_boolean>(), ref_c.value.check<D_boolean>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        if((lhs.type() == Value::type_integer) && (ref_c.value.type() == Value::type_integer)) {
          ref_c.value = do_bitwise_xor(lhs.check<D_integer>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_infix_assign(const Xpnode::S_operator_rpn &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_assign);
        // Copy the operand.
        // `alt.assign` is ignored.
        Reference_root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        stack_io.top().open() = std::move(ref_c.value);
      }

    void do_evaluate_unnamed_array(const Xpnode::S_unnamed_array &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        // Pop references to create an array.
        D_array array(alt.elem_cnt);
        for(auto i = alt.elem_cnt - 1; i + 1 != 0; --i) {
          array.mut(i) = stack_io.top().read();
          stack_io.pop();
        }
        Reference_root::S_temporary ref_c = { std::move(array) };
        stack_io.push(std::move(ref_c));
      }

    void do_evaluate_unnamed_object(const Xpnode::S_unnamed_object &alt, Reference_stack &stack_io, Global_context & /*global*/, const Executive_context & /*ctx*/)
      {
        // Pop references to create an object.
        D_object object(alt.keys.size());
        for(auto it = alt.keys.rbegin(); it != alt.keys.rend(); ++it) {
          object.mut(*it) = stack_io.top().read();
          stack_io.pop();
        }
        Reference_root::S_temporary ref_c = { std::move(object) };
        stack_io.push(std::move(ref_c));
      }

    void do_evaluate_coalescence(const Xpnode::S_coalescence &alt, Reference_stack &stack_io, Global_context &global, const Executive_context &ctx)
      {
        // Pick a branch basing on the condition.
        if(stack_io.top().read().type() != Value::type_null) {
          // If the result is non-null, leave the condition on the stack.
          return;
        }
        const auto has_result = alt.branch_null.evaluate_partial(stack_io, global, ctx);
        if(!has_result) {
          // If the branch is empty, push a hard `null` on the stack.
          stack_io.mut_top() = Reference_root::S_null();
          return;
        }
        // The result will have been pushed onto `stack_io`.
        ASTERIA_DEBUG_LOG("Setting coalescence result: ", stack_io.top().read());
        if(alt.assign) {
          auto value = stack_io.top().read();
          stack_io.pop();
          stack_io.top().open() = std::move(value);
        } else {
          auto result = std::move(stack_io.mut_top());
          stack_io.pop();
          stack_io.mut_top() = std::move(result);
        }
      }

    // Why do we have to duplicate these parameters so many times?
    // BECAUSE C++ IS STUPID, PERIOD.
    template<typename AltT, void (&funcT)(const AltT &, Reference_stack &, Global_context &, const Executive_context &)>
      rocket::binder_first<void (*)(const void *, Reference_stack &, Global_context &, const Executive_context &), const void *> do_bind(const AltT &alt)
      {
        return rocket::bind_first(
          [](const void *qalt, Reference_stack &stack_io, Global_context &global, const Executive_context &ctx)
            {
              return funcT(*static_cast<const AltT *>(qalt), stack_io, global, ctx);
            },
          std::addressof(alt));
      }

    }

rocket::binder_first<void (*)(const void *, Reference_stack &, Global_context &, const Executive_context &), const void *> Xpnode::compile() const
  {
    switch(Index(this->m_stor.index())) {
      case index_literal: {
        const auto &alt = this->m_stor.as<S_literal>();
        return do_bind<S_literal, do_evaluate_literal>(alt);
      }
      case index_named_reference: {
        const auto &alt = this->m_stor.as<S_named_reference>();
        return do_bind<S_named_reference, do_evaluate_named_reference>(alt);
      }
      case index_bound_reference: {
        const auto &alt = this->m_stor.as<S_bound_reference>();
        return do_bind<S_bound_reference, do_evaluate_bound_reference>(alt);
      }
      case index_closure_function: {
        const auto &alt = this->m_stor.as<S_closure_function>();
        return do_bind<S_closure_function, do_evaluate_closure_function>(alt);
      }
      case index_branch: {
        const auto &alt = this->m_stor.as<S_branch>();
        return do_bind<S_branch, do_evaluate_branch>(alt);
      }
      case index_function_call: {
        const auto &alt = this->m_stor.as<S_function_call>();
        return do_bind<S_function_call, do_evaluate_function_call>(alt);
      }
      case index_subscript: {
        const auto &alt = this->m_stor.as<S_subscript>();
        return do_bind<S_subscript, do_evaluate_subscript>(alt);
      }
      case index_operator_rpn: {
        const auto &alt = this->m_stor.as<S_operator_rpn>();
        switch(alt.xop) {
          case xop_postfix_inc: {
            return do_bind<S_operator_rpn, do_evaluate_postfix_inc>(alt);
          }
          case xop_postfix_dec: {
            return do_bind<S_operator_rpn, do_evaluate_postfix_dec>(alt);
          }
          case xop_prefix_pos: {
            return do_bind<S_operator_rpn, do_evaluate_prefix_pos>(alt);
          }
          case xop_prefix_neg: {
            return do_bind<S_operator_rpn, do_evaluate_prefix_neg>(alt);
          }
          case xop_prefix_notb: {
            return do_bind<S_operator_rpn, do_evaluate_prefix_notb>(alt);
          }
          case xop_prefix_notl: {
            return do_bind<S_operator_rpn, do_evaluate_prefix_notl>(alt);
          }
          case xop_prefix_inc: {
            return do_bind<S_operator_rpn, do_evaluate_prefix_inc>(alt);
          }
          case xop_prefix_dec: {
            return do_bind<S_operator_rpn, do_evaluate_prefix_dec>(alt);
          }
          case xop_prefix_unset: {
            return do_bind<S_operator_rpn, do_evaluate_prefix_unset>(alt);
          }
          case xop_prefix_lengthof: {
            return do_bind<S_operator_rpn, do_evaluate_prefix_lengthof>(alt);
          }
          case xop_prefix_typeof: {
            return do_bind<S_operator_rpn, do_evaluate_prefix_typeof>(alt);
          }
          case xop_infix_cmp_eq: {
            return do_bind<S_operator_rpn, do_evaluate_infix_cmp_eq>(alt);
          }
          case xop_infix_cmp_ne: {
            return do_bind<S_operator_rpn, do_evaluate_infix_cmp_ne>(alt);
          }
          case xop_infix_cmp_lt: {
            return do_bind<S_operator_rpn, do_evaluate_infix_cmp_lt>(alt);
          }
          case xop_infix_cmp_gt: {
            return do_bind<S_operator_rpn, do_evaluate_infix_cmp_gt>(alt);
          }
          case xop_infix_cmp_lte: {
            return do_bind<S_operator_rpn, do_evaluate_infix_cmp_lte>(alt);
          }
          case xop_infix_cmp_gte: {
            return do_bind<S_operator_rpn, do_evaluate_infix_cmp_gte>(alt);
          }
          case xop_infix_cmp_3way: {
            return do_bind<S_operator_rpn, do_evaluate_infix_cmp_3way>(alt);
          }
          case xop_infix_add: {
            return do_bind<S_operator_rpn, do_evaluate_infix_add>(alt);
          }
          case xop_infix_sub: {
            return do_bind<S_operator_rpn, do_evaluate_infix_sub>(alt);
          }
          case xop_infix_mul: {
            return do_bind<S_operator_rpn, do_evaluate_infix_mul>(alt);
          }
          case xop_infix_div: {
            return do_bind<S_operator_rpn, do_evaluate_infix_div>(alt);
          }
          case xop_infix_mod: {
            return do_bind<S_operator_rpn, do_evaluate_infix_mod>(alt);
          }
          case xop_infix_sll: {
            return do_bind<S_operator_rpn, do_evaluate_infix_sll>(alt);
          }
          case xop_infix_srl: {
            return do_bind<S_operator_rpn, do_evaluate_infix_srl>(alt);
          }
          case xop_infix_sla: {
            return do_bind<S_operator_rpn, do_evaluate_infix_sla>(alt);
          }
          case xop_infix_sra: {
            return do_bind<S_operator_rpn, do_evaluate_infix_sra>(alt);
          }
          case xop_infix_andb: {
            return do_bind<S_operator_rpn, do_evaluate_infix_andb>(alt);
          }
          case xop_infix_orb: {
            return do_bind<S_operator_rpn, do_evaluate_infix_orb>(alt);
          }
          case xop_infix_xorb: {
            return do_bind<S_operator_rpn, do_evaluate_infix_xorb>(alt);
          }
          case xop_infix_assign: {
            return do_bind<S_operator_rpn, do_evaluate_infix_assign>(alt);
          }
          default: {
            ASTERIA_TERMINATE("An unknown operator type enumeration `", alt.xop, "` has been encountered.");
          }
        }
      }
      case index_unnamed_array: {
        const auto &alt = this->m_stor.as<S_unnamed_array>();
        return do_bind<S_unnamed_array, do_evaluate_unnamed_array>(alt);
      }
      case index_unnamed_object: {
        const auto &alt = this->m_stor.as<S_unnamed_object>();
        return do_bind<S_unnamed_object, do_evaluate_unnamed_object>(alt);
      }
      case index_coalescence: {
        const auto &alt = this->m_stor.as<S_coalescence>();
        return do_bind<S_coalescence, do_evaluate_coalescence>(alt);
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
