// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "../runtime/reference_stack.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/executive_context.hpp"
#include "../runtime/function_analytic_context.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/abstract_function.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../runtime/traceable_exception.hpp"
#include "../utilities.hpp"

namespace Asteria {

const char * Xpnode::get_operator_name(Xpnode::Xop xop) noexcept
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

    std::pair<std::reference_wrapper<const Abstract_Context>,
              std::reference_wrapper<const Reference>> do_name_lookup(const Global_Context &global, const Abstract_Context &ctx, const PreHashed_String &name)
      {
        // Search for the name in `ctx` recursively.
        auto qref = ctx.get_named_reference_opt(name);
        auto qctx = std::addressof(ctx);
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
        qref = global.Global_Context::get_named_reference_opt(name);
        if(ROCKET_EXPECT(qref)) {
          return std::make_pair(std::ref(*qctx), std::ref(*qref));
        }
        // Not found...
        ASTERIA_THROW_RUNTIME_ERROR("The identifier `", name, "` has not been declared yet.");
      }

    }

void Xpnode::bind(CoW_Vector<Xpnode> &nodes_out, const Global_Context &global, const Analytic_Context &ctx) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_literal:
      {
        const auto &alt = this->m_stor.as<S_literal>();
        // Copy it as-is.
        Xpnode::S_literal alt_bnd = { alt.value };
        nodes_out.emplace_back(std::move(alt_bnd));
        return;
      }
    case index_named_reference:
      {
        const auto &alt = this->m_stor.as<S_named_reference>();
        // Only references with non-reserved names can be bound.
        if(alt.name.rdstr().starts_with("__")) {
          // Copy it as-is.
          Xpnode::S_named_reference alt_bnd = { alt.name };
          nodes_out.emplace_back(std::move(alt_bnd));
          return;
        }
        // Look for the reference in the current context.
        auto pair = do_name_lookup(global, ctx, alt.name);
        if(pair.first.get().is_analytic()) {
          // Don't bind it onto something in a analytic context which will soon get destroyed.
          Xpnode::S_named_reference alt_bnd = { alt.name };
          nodes_out.emplace_back(std::move(alt_bnd));
          return;
        }
        Xpnode::S_bound_reference alt_bnd = { pair.second };
        nodes_out.emplace_back(std::move(alt_bnd));
        return;
      }
    case index_bound_reference:
      {
        const auto &alt = this->m_stor.as<S_bound_reference>();
        // Copy it as-is.
        Xpnode::S_bound_reference alt_bnd = { alt.ref };
        nodes_out.emplace_back(std::move(alt_bnd));
        return;
      }
    case index_closure_function:
      {
        const auto &alt = this->m_stor.as<S_closure_function>();
        // Bind the body recursively.
        Function_Analytic_Context ctx_next(&ctx, alt.params);
        auto body_bnd = alt.body.bind_in_place(ctx_next, global);
        Xpnode::S_closure_function alt_bnd = { alt.sloc, alt.params, std::move(body_bnd) };
        nodes_out.emplace_back(std::move(alt_bnd));
        return;
      }
    case index_branch:
      {
        const auto &alt = this->m_stor.as<S_branch>();
        // Bind both branches recursively.
        auto branch_true_bnd = alt.branch_true.bind(global, ctx);
        auto branch_false_bnd = alt.branch_false.bind(global, ctx);
        Xpnode::S_branch alt_bnd = { std::move(branch_true_bnd), std::move(branch_false_bnd), alt.assign };
        nodes_out.emplace_back(std::move(alt_bnd));
        return;
      }
    case index_function_call:
      {
        const auto &alt = this->m_stor.as<S_function_call>();
        // Copy it as-is.
        Xpnode::S_function_call alt_bnd = { alt.sloc, alt.nargs };
        nodes_out.emplace_back(std::move(alt_bnd));
        return;
      }
    case index_subscript:
      {
        const auto &alt = this->m_stor.as<S_subscript>();
        // Copy it as-is.
        Xpnode::S_subscript alt_bnd = { alt.name };
        nodes_out.emplace_back(std::move(alt_bnd));
        return;
      }
    case index_operator_rpn:
      {
        const auto &alt = this->m_stor.as<S_operator_rpn>();
        // Copy it as-is.
        Xpnode::S_operator_rpn alt_bnd = { alt.xop, alt.assign };
        nodes_out.emplace_back(std::move(alt_bnd));
        return;
      }
    case index_unnamed_array:
      {
        const auto &alt = this->m_stor.as<S_unnamed_array>();
        // Copy it as-is.
        Xpnode::S_unnamed_array alt_bnd = { alt.nelems };
        nodes_out.emplace_back(std::move(alt_bnd));
        return;
      }
    case index_unnamed_object:
      {
        const auto &alt = this->m_stor.as<S_unnamed_object>();
        // Copy it as-is.
        Xpnode::S_unnamed_object alt_bnd = { alt.keys };
        nodes_out.emplace_back(std::move(alt_bnd));
        return;
      }
    case index_coalescence:
      {
        const auto &alt = this->m_stor.as<S_coalescence>();
        // Bind the null branch recursively.
        auto branch_null_bnd = alt.branch_null.bind(global, ctx);
        Xpnode::S_coalescence alt_bnd = { std::move(branch_null_bnd), alt.assign };
        nodes_out.emplace_back(std::move(alt_bnd));
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown expression node type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

    namespace {

    void do_evaluate_literal(const Xpnode::S_literal &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        // Push the constant.
        Reference_Root::S_constant ref_c = { alt.value };
        stack_io.push(std::move(ref_c));
      }

    void do_evaluate_named_reference(const Xpnode::S_named_reference &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context &global, const Executive_Context &ctx)
      {
        // Look for the reference in the current context.
        auto pair = do_name_lookup(global, ctx, alt.name);
#ifdef ROCKET_DEBUG
        ROCKET_ASSERT(!pair.first.get().is_analytic());
#endif
        // Push the reference found.
        stack_io.push(pair.second);
      }

    void do_evaluate_bound_reference(const Xpnode::S_bound_reference &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        // Push the reference stored.
        stack_io.push(alt.ref);
      }

    void do_evaluate_closure_function(const Xpnode::S_closure_function &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context &global, const Executive_Context &ctx)
      {
        // Instantiate the closure function.
        auto func = alt.body.instantiate_function(alt.sloc, rocket::sref("<closure function>"), alt.params, global, ctx);
        Reference_Root::S_temporary ref_c = { D_function(std::move(func)) };
        stack_io.push(std::move(ref_c));
      }

    void do_set_temporary(Reference_Stack &stack_io, bool assign, Reference_Root::S_temporary &&ref_c)
      {
        if(assign) {
          stack_io.top().open() = std::move(ref_c.value);
        } else {
          stack_io.mut_top() = std::move(ref_c);
        }
      }

    void do_forward(Reference_Stack &stack_io, bool assign)
      {
        if(assign) {
          Reference_Root::S_temporary ref_c = { stack_io.top().read() };
          stack_io.pop();
          do_set_temporary(stack_io, true, std::move(ref_c));
        } else {
          stack_io.pop_next();
        }
      }

    void do_evaluate_branch(const Xpnode::S_branch &alt, Reference_Stack &stack_io, const CoW_String &func, const Global_Context &global, const Executive_Context &ctx)
      {
        // Pick a branch basing on the condition.
        const auto result = stack_io.top().read().test() ? alt.branch_true.evaluate_partial(stack_io, func, global, ctx)
                                                         : alt.branch_false.evaluate_partial(stack_io, func, global, ctx);
        if(!result) {
          // If the branch is empty, leave the condition reference on the stack.
          return;
        }
        do_forward(stack_io, alt.assign);
      }

    void do_evaluate_function_call(const Xpnode::S_function_call &alt, Reference_Stack &stack_io, const CoW_String &func, const Global_Context &global, const Executive_Context & /*ctx*/)
      {
        // Allocate the argument vector.
        CoW_Vector<Reference> args;
        args.resize(alt.nargs);
        for(auto it = args.mut_rbegin(); it != args.rend(); ++it) {
          *it = std::move(stack_io.mut_top());
          stack_io.pop();
        }
        // Get the target reference.
        const auto target_value = stack_io.top().read();
        // Make sure it is really a function.
        if(target_value.type() != type_function) {
          ASTERIA_THROW_RUNTIME_ERROR("`", target_value, "` is not a function and cannot be called.");
        }
        const auto &target = target_value.check<D_function>().get();
        // Make the `this` reference. On the function's return it is reused to store the result of the function.
        auto &self_result = stack_io.mut_top().zoom_out();
        // Call the function now.
        ASTERIA_DEBUG_LOG("Initiating function call at \'", alt.sloc, "\' inside `", func, "`: target = ", target, ", this = ", self_result.read());
        try {
          target.invoke(self_result, global, std::move(args));
          // The result will have been written to `self_result`.
        } catch(const std::exception &stdex) {
          ASTERIA_DEBUG_LOG("Caught `std::exception` thrown inside function call at \'", alt.sloc, "\' inside `", func, "`: what = ", stdex.what());
          // Translate the exception.
          auto traceable = trace_exception(stdex);
          traceable.append_frame(alt.sloc, func);
          throw traceable;
        }
        ASTERIA_DEBUG_LOG("Returned from function call at \'", alt.sloc, "\' inside `", func, "`: target = ", target, ", result = ", self_result.read());
      }

    void do_evaluate_subscript(const Xpnode::S_subscript &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
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
        case type_integer:
          {
            Reference_Modifier::S_array_index mod_c = { subscript.check<D_integer>() };
            stack_io.mut_top().zoom_in(std::move(mod_c));
            break;
          }
        case type_string:
          {
            Reference_Modifier::S_object_key mod_c = { PreHashed_String(subscript.check<D_string>()) };
            stack_io.mut_top().zoom_in(std::move(mod_c));
            break;
          }
        default:
          ASTERIA_THROW_RUNTIME_ERROR("The value `", subscript, "` cannot be used as a subscript.");
        }
      }

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
        auto res = static_cast<std::uint64_t>(rhs);
        res = -res;
        return static_cast<std::int64_t>(res);
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
        const auto slhs = (rhs >= 0) ? lhs : -lhs;
        const auto arhs = std::abs(rhs);
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
        auto res = static_cast<std::uint64_t>(lhs);
        res <<= rhs;
        return static_cast<std::int64_t>(res);
      }

    ROCKET_PURE_FUNCTION std::int64_t do_operator_srl(std::int64_t lhs, std::int64_t rhs)
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

    ROCKET_PURE_FUNCTION std::int64_t do_operator_sla(std::int64_t lhs, std::int64_t rhs)
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

    ROCKET_PURE_FUNCTION std::int64_t do_operator_sra(std::int64_t lhs, std::int64_t rhs)
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

    ROCKET_PURE_FUNCTION CoW_String do_operator_add(const CoW_String &lhs, const CoW_String &rhs)
      {
        CoW_String res;
        res.reserve(lhs.size() + rhs.size());
        res.append(lhs);
        res.append(rhs);
        return res;
      }

    ROCKET_PURE_FUNCTION CoW_String do_operator_mul(const CoW_String &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String duplication count `", rhs, "` for `", lhs, "` is negative.");
        }
        CoW_String res;
        const auto count = static_cast<std::uint64_t>(rhs);
        if(count == 0) {
          return res;
        }
        if(lhs.size() > res.max_size() / count) {
          ASTERIA_THROW_RUNTIME_ERROR("Duplication of `", lhs, "` up to `", rhs, "` times would result in an overlong string that cannot be allocated.");
        }
        res.reserve(lhs.size() * static_cast<std::size_t>(count));
        auto mask = size_t(-1) / 2 + 1;
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

    ROCKET_PURE_FUNCTION CoW_String do_operator_mul(std::int64_t lhs, const CoW_String &rhs)
      {
        return do_operator_mul(rhs, lhs);
      }

    ROCKET_PURE_FUNCTION CoW_String do_operator_sll(const CoW_String &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        CoW_String res;
        res.assign(lhs.size(), ' ');
        const auto count = static_cast<std::uint64_t>(rhs);
        if(count >= lhs.size()) {
          return res;
        }
        std::char_traits<char>::copy(res.mut_data(), lhs.data() + count, lhs.size() - static_cast<std::size_t>(count));
        return res;
      }

    ROCKET_PURE_FUNCTION CoW_String do_operator_srl(const CoW_String &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        CoW_String res;
        res.assign(lhs.size(), ' ');
        const auto count = static_cast<std::uint64_t>(rhs);
        if(count >= lhs.size()) {
          return res;
        }
        std::char_traits<char>::copy(res.mut_data() + count, lhs.data(), lhs.size() - static_cast<std::size_t>(count));
        return res;
      }

    ROCKET_PURE_FUNCTION CoW_String do_operator_sla(const CoW_String &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        CoW_String res;
        const auto count = static_cast<std::uint64_t>(rhs);
        if(count > res.max_size() - lhs.size()) {
          ASTERIA_THROW_RUNTIME_ERROR("Shifting `", lhs, "` to the left by `", rhs, "` bytes would result in an overlong string that cannot be allocated.");
        }
        res.assign(lhs.size() + static_cast<std::size_t>(count), ' ');
        std::char_traits<char>::copy(res.mut_data(), lhs.data(), lhs.size());
        return res;
      }

    ROCKET_PURE_FUNCTION CoW_String do_operator_sra(const CoW_String &lhs, std::int64_t rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        CoW_String res;
        const auto count = static_cast<std::uint64_t>(rhs);
        if(count >= lhs.size()) {
          return res;
        }
        res.append(lhs.data(), lhs.size() - static_cast<std::size_t>(count));
        return res;
      }

    void do_evaluate_operator_postfix_inc(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_postfix_inc);
        // Increment the operand and return the old value.
        // `alt.assign` is ignored.
        auto &value = stack_io.top().open();
        if(value.type() == type_integer) {
          auto &lhs = value.check<D_integer>();
          Reference_Root::S_temporary ref_c = { lhs };
          lhs = do_operator_add(lhs, D_integer(1));
          do_set_temporary(stack_io, false, std::move(ref_c));
          return;
        }
        if(value.type() == type_real) {
          auto &lhs = value.check<D_real>();
          Reference_Root::S_temporary ref_c = { lhs };
          lhs = do_operator_add(lhs, D_real(1));
          do_set_temporary(stack_io, false, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", value, "`.");
      }

    void do_evaluate_operator_postfix_dec(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_postfix_dec);
        // Decrement the operand and return the old value.
        // `alt.assign` is ignored.
        auto &value = stack_io.top().open();
        if(value.type() == type_integer) {
          auto &lhs = value.check<D_integer>();
          Reference_Root::S_temporary ref_c = { lhs };
          lhs = do_operator_sub(lhs, D_integer(1));
          do_set_temporary(stack_io, false, std::move(ref_c));
          return;
        }
        if(value.type() == type_real) {
          auto &lhs = value.check<D_real>();
          Reference_Root::S_temporary ref_c = { lhs };
          lhs = do_operator_sub(lhs, D_real(1));
          do_set_temporary(stack_io, false, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", value, "`.");
      }

    void do_evaluate_operator_prefix_pos(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_pos);
        // Copy the operand to create a temporary value, then return it.
        // N.B. This is one of the few operators that work on all types.
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        do_set_temporary(stack_io, alt.assign, std::move(ref_c));
      }

    void do_evaluate_operator_prefix_neg(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_neg);
        // Negate the operand to create a temporary value, then return it.
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        if(ref_c.value.type() == type_integer) {
          auto &rhs = ref_c.value.check<D_integer>();
          rhs = do_operator_neg(rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        if(ref_c.value.type() == type_real) {
          auto &rhs = ref_c.value.check<D_real>();
          rhs = do_operator_neg(rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
      }

    void do_evaluate_operator_prefix_notb(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_notb);
        // Perform bitwise NOT operation on the operand to create a temporary value, then return it.
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        if(ref_c.value.type() == type_boolean) {
          auto &rhs = ref_c.value.check<D_boolean>();
          rhs = do_operator_not(rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        if(ref_c.value.type() == type_integer) {
          auto &rhs = ref_c.value.check<D_integer>();
          rhs = do_operator_not(rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
      }

    void do_evaluate_operator_prefix_notl(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_notl);
        // Perform logical NOT operation on the operand to create a temporary value, then return it.
        // N.B. This is one of the few operators that work on all types.
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        ref_c.value = do_operator_not(ref_c.value.test());
        do_set_temporary(stack_io, alt.assign, std::move(ref_c));
      }

    void do_evaluate_operator_prefix_inc(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_inc);
        // Increment the operand and return it.
        // `alt.assign` is ignored.
        auto &value = stack_io.top().open();
        if(value.type() == type_integer) {
          auto &rhs = value.check<D_integer>();
          rhs = do_operator_add(rhs, D_integer(1));
          return;
        }
        if(value.type() == type_real) {
          auto &rhs = value.check<D_real>();
          rhs = do_operator_add(rhs, D_real(1));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", value, "`.");
      }

    void do_evaluate_operator_prefix_dec(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_dec);
        // Decrement the operand and return it.
        // `alt.assign` is ignored.
        auto &value = stack_io.top().open();
        if(value.type() == type_integer) {
          auto &rhs = value.check<D_integer>();
          rhs = do_operator_sub(rhs, D_integer(1));
          return;
        }
        if(value.type() == type_real) {
          auto &rhs = value.check<D_real>();
          rhs = do_operator_sub(rhs, D_real(1));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", value, "`.");
      }

    void do_evaluate_operator_prefix_unset(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_unset);
        // Unset the reference and return the old value.
        Reference_Root::S_temporary ref_c = { stack_io.top().unset() };
        do_set_temporary(stack_io, alt.assign, std::move(ref_c));
      }

    void do_evaluate_operator_prefix_lengthof(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_lengthof);
        // Return the number of elements in the operand.
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        if(ref_c.value.type() == type_null) {
          ref_c.value = D_integer(0);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        if(ref_c.value.type() == type_string) {
          ref_c.value = D_integer(ref_c.value.check<D_string>().size());
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        if(ref_c.value.type() == type_array) {
          ref_c.value = D_integer(ref_c.value.check<D_array>().size());
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        if(ref_c.value.type() == type_object) {
          ref_c.value = D_integer(ref_c.value.check<D_object>().size());
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", ref_c.value, "`.");
      }

    void do_evaluate_operator_prefix_typeof(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_prefix_typeof);
        // Return the type name of the operand.
        // N.B. This is one of the few operators that work on all types.
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        ref_c.value = D_string(rocket::sref(Value::get_type_name(ref_c.value.type())));
        do_set_temporary(stack_io, alt.assign, std::move(ref_c));
      }

    void do_evaluate_operator_infix_cmp_eq(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_eq);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        const auto comp = lhs.compare(ref_c.value);
        ref_c.value = D_boolean(comp == Value::compare_equal);
        do_set_temporary(stack_io, alt.assign, std::move(ref_c));
      }

    void do_evaluate_operator_infix_cmp_ne(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_ne);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // Report unordered operands as being unequal.
        // N.B. This is one of the few operators that work on all types.
        const auto comp = lhs.compare(ref_c.value);
        ref_c.value = D_boolean(comp != Value::compare_equal);
        do_set_temporary(stack_io, alt.assign, std::move(ref_c));
      }

    void do_evaluate_operator_infix_cmp_lt(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_lt);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // Throw an exception in case of unordered operands.
        const auto comp = lhs.compare(ref_c.value);
        if(comp == Value::compare_unordered) {
          ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs, "` and `", ref_c.value, "` are unordered.");
        }
        ref_c.value = D_boolean(comp == Value::compare_less);
        do_set_temporary(stack_io, alt.assign, std::move(ref_c));
      }

    void do_evaluate_operator_infix_cmp_gt(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_gt);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // Throw an exception in case of unordered operands.
        const auto comp = lhs.compare(ref_c.value);
        if(comp == Value::compare_unordered) {
          ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs, "` and `", ref_c.value, "` are unordered.");
        }
        ref_c.value = D_boolean(comp == Value::compare_greater);
        do_set_temporary(stack_io, alt.assign, std::move(ref_c));
      }

    void do_evaluate_operator_infix_cmp_lte(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_lte);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // Throw an exception in case of unordered operands.
        const auto comp = lhs.compare(ref_c.value);
        if(comp == Value::compare_unordered) {
          ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs, "` and `", ref_c.value, "` are unordered.");
        }
        ref_c.value = D_boolean(comp != Value::compare_greater);
        do_set_temporary(stack_io, alt.assign, std::move(ref_c));
      }

    void do_evaluate_operator_infix_cmp_gte(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_gte);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // Throw an exception in case of unordered operands.
        const auto comp = lhs.compare(ref_c.value);
        if(comp == Value::compare_unordered) {
          ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs, "` and `", ref_c.value, "` are unordered.");
        }
        ref_c.value = D_boolean(comp != Value::compare_less);
        do_set_temporary(stack_io, alt.assign, std::move(ref_c));
      }

    void do_evaluate_operator_infix_cmp_3way(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_cmp_3way);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // N.B. This is one of the few operators that work on all types.
        const auto comp = lhs.compare(ref_c.value);
        switch(rocket::weaken_enum(comp)) {
        case Value::compare_less:
          {
            ref_c.value = D_integer(-1);
            break;
          }
        case Value::compare_equal:
          {
            ref_c.value = D_integer(0);
            break;
          }
        case Value::compare_greater:
          {
            ref_c.value = D_integer(+1);
            break;
          }
        default:
          {
            ref_c.value = D_string(rocket::sref("<unordered>"));
            break;
          }
        }
        do_set_temporary(stack_io, alt.assign, std::move(ref_c));
      }

    void do_evaluate_operator_infix_add(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_add);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // For the `boolean` type, return the logical OR'd result of both operands.
        if((lhs.type() == type_boolean) && (ref_c.value.type() == type_boolean)) {
          auto &rhs = ref_c.value.check<D_boolean>();
          rhs = do_operator_or(lhs.check<D_boolean>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        // For the `integer` and `real` types, return the sum of both operands.
        if((lhs.type() == type_integer) && (ref_c.value.type() == type_integer)) {
          auto &rhs = ref_c.value.check<D_integer>();
          rhs = do_operator_add(lhs.check<D_integer>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        if((lhs.type() == type_real) && (ref_c.value.type() == type_real)) {
          auto &rhs = ref_c.value.check<D_real>();
          rhs = do_operator_add(lhs.check<D_real>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        // For the `string` type, concatenate the operands in lexical order to create a new string, then return it.
        if((lhs.type() == type_string) && (ref_c.value.type() == type_string)) {
          auto &rhs = ref_c.value.check<D_string>();
          rhs = do_operator_add(lhs.check<D_string>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_operator_infix_sub(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_sub);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // For the `boolean` type, return the logical XOR'd result of both operands.
        if((lhs.type() == type_boolean) && (ref_c.value.type() == type_boolean)) {
          auto &rhs = ref_c.value.check<D_boolean>();
          rhs = do_operator_xor(lhs.check<D_boolean>(), rhs);
          return;
        }
        // For the `integer` and `real` types, return the difference of both operands.
        if((lhs.type() == type_integer) && (ref_c.value.type() == type_integer)) {
          auto &rhs = ref_c.value.check<D_integer>();
          rhs = do_operator_sub(lhs.check<D_integer>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        if((lhs.type() == type_real) && (ref_c.value.type() == type_real)) {
          auto &rhs = ref_c.value.check<D_real>();
          rhs = do_operator_sub(lhs.check<D_real>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_operator_infix_mul(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_mul);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // For type `boolean`, return the logical AND'd result of both operands.
        if((lhs.type() == type_boolean) && (ref_c.value.type() == type_boolean)) {
          auto &rhs = ref_c.value.check<D_boolean>();
          rhs = do_operator_and(lhs.check<D_boolean>(), rhs);
          return;
        }
        // For the `integer` and `real` types, return the product of both operands.
        if((lhs.type() == type_integer) && (ref_c.value.type() == type_integer)) {
          auto &rhs = ref_c.value.check<D_integer>();
          rhs = do_operator_mul(lhs.check<D_integer>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        if((lhs.type() == type_real) && (ref_c.value.type() == type_real)) {
          auto &rhs = ref_c.value.check<D_real>();
          rhs = do_operator_mul(lhs.check<D_real>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        // If either operand has type `string` and the other has type `integer`, duplicate the string up to the specified number of times and return the result.
        if((lhs.type() == type_string) && (ref_c.value.type() == type_integer)) {
          // Note that `ref_c.value` does not have type `D_string`, thus this branch can't be optimized.
          ref_c.value = do_operator_mul(lhs.check<D_string>(), ref_c.value.check<D_integer>());
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        if((lhs.type() == type_integer) && (ref_c.value.type() == type_string)) {
          auto &rhs = ref_c.value.check<D_string>();
          rhs = do_operator_mul(lhs.check<D_integer>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_operator_infix_div(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_div);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // For the `integer` and `real` types, return the quotient of both operands.
        if((lhs.type() == type_integer) && (ref_c.value.type() == type_integer)) {
          auto &rhs = ref_c.value.check<D_integer>();
          rhs = do_operator_div(lhs.check<D_integer>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        if((lhs.type() == type_real) && (ref_c.value.type() == type_real)) {
          auto &rhs = ref_c.value.check<D_real>();
          rhs = do_operator_div(lhs.check<D_real>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_operator_infix_mod(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_mod);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // For types `integer` and `real`, return the reminder of both operands.
        if((lhs.type() == type_integer) && (ref_c.value.type() == type_integer)) {
          auto &rhs = ref_c.value.check<D_integer>();
          rhs = do_operator_mod(lhs.check<D_integer>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        if((lhs.type() == type_real) && (ref_c.value.type() == type_real)) {
          auto &rhs = ref_c.value.check<D_real>();
          rhs = do_operator_mod(lhs.check<D_real>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_operator_infix_sll(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_sll);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // The second operand shall have type `integer`.
        if(ref_c.value.type() == type_integer) {
          // If the first operand has type `integer`, shift the first operand to the left by the number of bits specified by the second operand.
          // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
          if(lhs.type() == type_integer) {
            auto &rhs = ref_c.value.check<D_integer>();
            rhs = do_operator_sll(lhs.check<D_integer>(), rhs);
            do_set_temporary(stack_io, alt.assign, std::move(ref_c));
            return;
          }
          // If the first operand has type `string`, fill space characters in the right and discard characters from the left.
          // The number of bytes in the first operand will be preserved.
          if(lhs.type() == type_string) {
            // Note that `ref_c.value` does not have type `D_string`, thus this branch can't be optimized.
            ref_c.value = do_operator_sll(lhs.check<D_string>(), ref_c.value.check<D_integer>());
            do_set_temporary(stack_io, alt.assign, std::move(ref_c));
            return;
          }
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_operator_infix_srl(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_srl);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // The second operand shall have type `integer`.
        if(ref_c.value.type() == type_integer) {
          // If the first operand has type `integer`, shift the first operand to the right by the number of bits specified by the second operand.
          // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
          if(lhs.type() == type_integer) {
            auto &rhs = ref_c.value.check<D_integer>();
            rhs = do_operator_srl(lhs.check<D_integer>(), rhs);
            do_set_temporary(stack_io, alt.assign, std::move(ref_c));
            return;
          }
          // If the first operand has type `string`, fill space characters in the left and discard characters from the right.
          // The number of bytes in the first operand will be preserved.
          if(lhs.type() == type_string) {
            // Note that `ref_c.value` does not have type `D_string`, thus this branch can't be optimized.
            ref_c.value = do_operator_srl(lhs.check<D_string>(), ref_c.value.check<D_integer>());
            do_set_temporary(stack_io, alt.assign, std::move(ref_c));
            return;
          }
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_operator_infix_sla(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_sla);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // The second operand shall have type `integer`.
        if(ref_c.value.type() == type_integer) {
          // If the first operand is of type `integer`, shift the first operand to the left by the number of bits specified by the second operand.
          // Bits shifted out that are equal to the sign bit are discarded. Bits shifted in are filled with zeroes.
          // If any bits that are different from the sign bit would be shifted out, an exception is thrown.
          if(lhs.type() == type_integer) {
            auto &rhs = ref_c.value.check<D_integer>();
            rhs = do_operator_sla(lhs.check<D_integer>(), rhs);
            do_set_temporary(stack_io, alt.assign, std::move(ref_c));
            return;
          }
          // If the first operand has type `string`, fill space characters in the right.
          if(lhs.type() == type_string) {
            // Note that `ref_c.value` does not have type `D_string`, thus this branch can't be optimized.
            ref_c.value = do_operator_sla(lhs.check<D_string>(), ref_c.value.check<D_integer>());
            do_set_temporary(stack_io, alt.assign, std::move(ref_c));
            return;
          }
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_operator_infix_sra(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_sra);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // The second operand shall have type `integer`.
        if(ref_c.value.type() == type_integer) {
          // If the first operand is of type `integer`, shift the first operand to the right by the number of bits specified by the second operand.
          // Bits shifted out are discarded. Bits shifted in are filled with the sign bit.
          if(lhs.type() == type_integer) {
            auto &rhs = ref_c.value.check<D_integer>();
            rhs = do_operator_sra(lhs.check<D_integer>(), rhs);
            do_set_temporary(stack_io, alt.assign, std::move(ref_c));
            return;
          }
          // If the first operand has type `string`, discard characters from the right.
          if(lhs.type() == type_string) {
            // Note that `ref_c.value` does not have type `D_string`, thus this branch can't be optimized.
            ref_c.value = do_operator_sra(lhs.check<D_string>(), ref_c.value.check<D_integer>());
            do_set_temporary(stack_io, alt.assign, std::move(ref_c));
            return;
          }
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_operator_infix_andb(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_andb);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // For the `boolean` type, return the logical AND'd result of both operands.
        if((lhs.type() == type_boolean) && (ref_c.value.type() == type_boolean)) {
          auto &rhs = ref_c.value.check<D_boolean>();
          rhs = do_operator_and(lhs.check<D_boolean>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        // For the `integer` type, return the bitwise AND'd result of both operands.
        if((lhs.type() == type_integer) && (ref_c.value.type() == type_integer)) {
          auto &rhs = ref_c.value.check<D_integer>();
          rhs = do_operator_and(lhs.check<D_integer>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_operator_infix_orb(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_orb);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // For the `boolean` type, return the logical OR'd result of both operands.
        if((lhs.type() == type_boolean) && (ref_c.value.type() == type_boolean)) {
          auto &rhs = ref_c.value.check<D_boolean>();
          rhs = do_operator_or(lhs.check<D_boolean>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        // For the `integer` type, return the bitwise OR'd result of both operands.
        if((lhs.type() == type_integer) && (ref_c.value.type() == type_integer)) {
          auto &rhs = ref_c.value.check<D_integer>();
          rhs = do_operator_or(lhs.check<D_integer>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_operator_infix_xorb(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_xorb);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        const auto &lhs = stack_io.top().read();
        // For the `boolean` type, return the logical XOR'd result of both operands.
        if((lhs.type() == type_boolean) && (ref_c.value.type() == type_boolean)) {
          auto &rhs = ref_c.value.check<D_boolean>();
          rhs = do_operator_xor(lhs.check<D_boolean>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        // For the `integer` type, return the bitwise XOR'd result of both operands.
        if((lhs.type() == type_integer) && (ref_c.value.type() == type_integer)) {
          auto &rhs = ref_c.value.check<D_integer>();
          rhs = do_operator_xor(lhs.check<D_integer>(), rhs);
          do_set_temporary(stack_io, alt.assign, std::move(ref_c));
          return;
        }
        ASTERIA_THROW_RUNTIME_ERROR("The ", Xpnode::get_operator_name(alt.xop), " operation is not defined for `", lhs, "` and `", ref_c.value, "`.");
      }

    void do_evaluate_operator_infix_assign(const Xpnode::S_operator_rpn &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        ROCKET_ASSERT(alt.xop == Xpnode::xop_infix_assign);
        Reference_Root::S_temporary ref_c = { stack_io.top().read() };
        stack_io.pop();
        // Copy the operand.
        // `alt.assign` is ignored.
        do_set_temporary(stack_io, true, std::move(ref_c));
      }

    void do_evaluate_unnamed_array(const Xpnode::S_unnamed_array &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        // Pop references to create an array.
        D_array array;
        array.resize(alt.nelems);
        for(auto it = array.mut_rbegin(); it != array.rend(); ++it) {
          *it = stack_io.top().read();
          stack_io.pop();
        }
        Reference_Root::S_temporary ref_c = { std::move(array) };
        stack_io.push(std::move(ref_c));
      }

    void do_evaluate_unnamed_object(const Xpnode::S_unnamed_object &alt, Reference_Stack &stack_io, const CoW_String & /*func*/, const Global_Context & /*global*/, const Executive_Context & /*ctx*/)
      {
        // Pop references to create an object.
        D_object object;
        object.reserve(alt.keys.size());
        for(auto it = alt.keys.rbegin(); it != alt.keys.rend(); ++it) {
          object.try_emplace(*it, stack_io.top().read());
          stack_io.pop();
        }
        Reference_Root::S_temporary ref_c = { std::move(object) };
        stack_io.push(std::move(ref_c));
      }

    void do_evaluate_coalescence(const Xpnode::S_coalescence &alt, Reference_Stack &stack_io, const CoW_String &func, const Global_Context &global, const Executive_Context &ctx)
      {
        // Pick a branch basing on the condition.
        if(stack_io.top().read().type() != type_null) {
          // If the value is non-null, leave the condition reference on the stack.
          return;
        }
        const auto result = alt.branch_null.evaluate_partial(stack_io, func, global, ctx);
        if(!result) {
          // If the branch is empty, leave the condition reference on the stack.
          return;
        }
        do_forward(stack_io, alt.assign);
      }

    // Why do we have to duplicate these parameters so many times?
    // BECAUSE C++ IS STUPID, PERIOD.
    template<typename AltT,
             void (&funcT)(const AltT &,
                           Reference_Stack &, const CoW_String &, const Global_Context &, const Executive_Context &)
             > Expression::Compiled_Instruction do_bind(const AltT &alt)
      {
        return rocket::bind_front(
          [](const void *qalt,
             const std::tuple<Reference_Stack &, const CoW_String &, const Global_Context &, const Executive_Context &> &params)
            {
              return funcT(*static_cast<const AltT *>(qalt),
                           std::get<0>(params), std::get<1>(params), std::get<2>(params), std::get<3>(params));
            },
          std::addressof(alt));
      }

    }

void Xpnode::compile(CoW_Vector<Expression::Compiled_Instruction> &cinsts_out) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_literal:
      {
        const auto &alt = this->m_stor.as<S_literal>();
        cinsts_out.emplace_back(do_bind<S_literal, do_evaluate_literal>(alt));
        return;
      }
    case index_named_reference:
      {
        const auto &alt = this->m_stor.as<S_named_reference>();
        cinsts_out.emplace_back(do_bind<S_named_reference, do_evaluate_named_reference>(alt));
        return;
      }
    case index_bound_reference:
      {
        const auto &alt = this->m_stor.as<S_bound_reference>();
        cinsts_out.emplace_back(do_bind<S_bound_reference, do_evaluate_bound_reference>(alt));
        return;
      }
    case index_closure_function:
      {
        const auto &alt = this->m_stor.as<S_closure_function>();
        cinsts_out.emplace_back(do_bind<S_closure_function, do_evaluate_closure_function>(alt));
        return;
      }
    case index_branch:
      {
        const auto &alt = this->m_stor.as<S_branch>();
        cinsts_out.emplace_back(do_bind<S_branch, do_evaluate_branch>(alt));
        return;
      }
    case index_function_call:
      {
        const auto &alt = this->m_stor.as<S_function_call>();
        cinsts_out.emplace_back(do_bind<S_function_call, do_evaluate_function_call>(alt));
        return;
      }
    case index_subscript:
      {
        const auto &alt = this->m_stor.as<S_subscript>();
        cinsts_out.emplace_back(do_bind<S_subscript, do_evaluate_subscript>(alt));
        return;
      }
    case index_operator_rpn:
      {
        const auto &alt = this->m_stor.as<S_operator_rpn>();
        switch(alt.xop) {
        case xop_postfix_inc:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_postfix_inc>(alt));
            return;
          }
        case xop_postfix_dec:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_postfix_dec>(alt));
            return;
          }
        case xop_prefix_pos:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_prefix_pos>(alt));
            return;
          }
        case xop_prefix_neg:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_prefix_neg>(alt));
            return;
          }
        case xop_prefix_notb:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_prefix_notb>(alt));
            return;
          }
        case xop_prefix_notl:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_prefix_notl>(alt));
            return;
          }
        case xop_prefix_inc:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_prefix_inc>(alt));
            return;
          }
        case xop_prefix_dec:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_prefix_dec>(alt));
            return;
          }
        case xop_prefix_unset:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_prefix_unset>(alt));
            return;
          }
        case xop_prefix_lengthof:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_prefix_lengthof>(alt));
            return;
          }
        case xop_prefix_typeof:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_prefix_typeof>(alt));
            return;
          }
        case xop_infix_cmp_eq:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_cmp_eq>(alt));
            return;
          }
        case xop_infix_cmp_ne:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_cmp_ne>(alt));
            return;
          }
        case xop_infix_cmp_lt:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_cmp_lt>(alt));
            return;
          }
        case xop_infix_cmp_gt:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_cmp_gt>(alt));
            return;
          }
        case xop_infix_cmp_lte:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_cmp_lte>(alt));
            return;
          }
        case xop_infix_cmp_gte:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_cmp_gte>(alt));
            return;
          }
        case xop_infix_cmp_3way:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_cmp_3way>(alt));
            return;
          }
        case xop_infix_add:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_add>(alt));
            return;
          }
        case xop_infix_sub:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_sub>(alt));
            return;
          }
        case xop_infix_mul:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_mul>(alt));
            return;
          }
        case xop_infix_div:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_div>(alt));
            return;
          }
        case xop_infix_mod:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_mod>(alt));
            return;
          }
        case xop_infix_sll:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_sll>(alt));
            return;
          }
        case xop_infix_srl:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_srl>(alt));
            return;
          }
        case xop_infix_sla:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_sla>(alt));
            return;
          }
        case xop_infix_sra:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_sra>(alt));
            return;
          }
        case xop_infix_andb:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_andb>(alt));
            return;
          }
        case xop_infix_orb:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_orb>(alt));
            return;
          }
        case xop_infix_xorb:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_xorb>(alt));
            return;
          }
        case xop_infix_assign:
          {
            cinsts_out.emplace_back(do_bind<S_operator_rpn, do_evaluate_operator_infix_assign>(alt));
            return;
          }
        default:
          ASTERIA_TERMINATE("An unknown operator type enumeration `", alt.xop, "` has been encountered.");
        }
      }
    case index_unnamed_array:
      {
        const auto &alt = this->m_stor.as<S_unnamed_array>();
        cinsts_out.emplace_back(do_bind<S_unnamed_array, do_evaluate_unnamed_array>(alt));
        return;
      }
    case index_unnamed_object:
      {
        const auto &alt = this->m_stor.as<S_unnamed_object>();
        cinsts_out.emplace_back(do_bind<S_unnamed_object, do_evaluate_unnamed_object>(alt));
        return;
      }
    case index_coalescence:
      {
        const auto &alt = this->m_stor.as<S_coalescence>();
        cinsts_out.emplace_back(do_bind<S_coalescence, do_evaluate_coalescence>(alt));
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown expression node type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

void Xpnode::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case index_literal:
      {
        const auto &alt = this->m_stor.as<S_literal>();
        alt.value.enumerate_variables(callback);
        return;
      }
    case index_named_reference:
      {
        return;
      }
    case index_bound_reference:
      {
        const auto &alt = this->m_stor.as<S_bound_reference>();
        alt.ref.enumerate_variables(callback);
        return;
      }
    case index_closure_function:
      {
        const auto &alt = this->m_stor.as<S_closure_function>();
        alt.body.enumerate_variables(callback);
        return;
      }
    case index_branch:
      {
        const auto &alt = this->m_stor.as<S_branch>();
        alt.branch_true.enumerate_variables(callback);
        alt.branch_false.enumerate_variables(callback);
        return;
      }
    case index_function_call:
    case index_subscript:
    case index_operator_rpn:
    case index_unnamed_array:
    case index_unnamed_object:
      {
        return;
      }
    case index_coalescence:
      {
        const auto &alt = this->m_stor.as<S_coalescence>();
        alt.branch_null.enumerate_variables(callback);
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown expression node type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

}
