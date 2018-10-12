// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
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

  std::pair<const Abstract_context *, const Reference *> do_name_lookup(const Global_context *global_opt, const Abstract_context &ctx, const String &name)
    {
      auto next = global_opt;
      auto qctx = &ctx;
      do {
        const auto qref = qctx->get_named_reference_opt(name);
        if(qref) {
          return std::make_pair(qctx, qref);
        }
        qctx = qctx->get_parent_opt();
        if(!qctx) {
          qctx = rocket::exchange(next, nullptr);
          if(!qctx) {
            ASTERIA_THROW_RUNTIME_ERROR("The identifier `", name, "` has not been declared yet.");
          }
        }
      } while(true);
    }

}

Xpnode Xpnode::bind(const Global_context *global_opt, const Analytic_context &ctx) const
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
        if(!ctx.is_name_reserved(alt.name)) {
          // Look for the reference in the current context.
          // Don't bind it onto something in a analytic context which will soon get destroyed.
          const auto pair = do_name_lookup(global_opt, ctx, alt.name);
          if(!pair.first->is_analytic()) {
            Xpnode::S_bound_reference alt_bnd = { *(pair.second) };
            return std::move(alt_bnd);
          }
        }
        // Copy it as-is.
        Xpnode::S_named_reference alt_bnd = { alt.name };
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
        auto body_bnd = alt.body.bind_in_place(ctx_next, global_opt);
        Xpnode::S_closure_function alt_bnd = { alt.file, alt.line, alt.params, std::move(body_bnd) };
        return std::move(alt_bnd);
      }
      case index_branch: {
        const auto &alt = this->m_stor.as<S_branch>();
        // Bind both branches recursively.
        auto branch_true_bnd = alt.branch_true.bind(global_opt, ctx);
        auto branch_false_bnd = alt.branch_false.bind(global_opt, ctx);
        Xpnode::S_branch alt_bnd = { alt.assign, std::move(branch_true_bnd), std::move(branch_false_bnd) };
        return std::move(alt_bnd);
      }
      case index_function_call: {
        const auto &alt = this->m_stor.as<S_function_call>();
        // Copy it as-is.
        Xpnode::S_function_call alt_bnd = { alt.file, alt.line, alt.arg_cnt };
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
        auto branch_null_bnd = alt.branch_null.bind(global_opt, ctx);
        Xpnode::S_coalescence alt_bnd = { alt.assign, std::move(branch_null_bnd) };
        return std::move(alt_bnd);
      }
      default: {
        ASTERIA_TERMINATE("An unknown expression node type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

namespace {

  template<typename XvalueT>
    void do_set_result(Reference &ref_io, bool assign, XvalueT &&value)
      {
        ASTERIA_DEBUG_LOG("Setting result: ", Value(value));
        if(assign) {
          ref_io.write(std::forward<XvalueT>(value));
        } else {
          Reference_root::S_temporary ref_c = { std::forward<XvalueT>(value) };
          ref_io = std::move(ref_c);
        }
      }

  Reference do_pop_reference(Vector<Reference> &stack_io)
    {
      if(stack_io.empty()) {
        ASTERIA_THROW_RUNTIME_ERROR("The evaluation stack is empty, which means the expression is probably invalid.");
      }
      auto ref = std::move(stack_io.mut_back());
      stack_io.pop_back();
      return ref;
    }

  D_boolean do_logical_not(D_boolean rhs)
    {
      return !rhs;
    }

  D_boolean do_logical_and(D_boolean lhs, D_boolean rhs)
    {
      return lhs & rhs;
    }

  D_boolean do_logical_or(D_boolean lhs, D_boolean rhs)
    {
      return lhs | rhs;
    }

  D_boolean do_logical_xor(D_boolean lhs, D_boolean rhs)
    {
      return lhs ^ rhs;
    }

  D_integer do_negate(D_integer rhs, bool wrap)
    {
      using Limits = std::numeric_limits<D_integer>;
      if(!wrap && (rhs == Limits::min())) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral negation of `", rhs, "` would result in overflow.");
      }
      auto reg = static_cast<Uint64>(rhs);
      reg = -reg;
      return static_cast<D_integer>(reg);
    }

  D_integer do_add(D_integer lhs, D_integer rhs)
    {
      using Limits = std::numeric_limits<D_integer>;
      if((rhs >= 0) ? (lhs > Limits::max() - rhs) : (lhs < Limits::min() - rhs)) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral addition of `", lhs, "` and `", rhs, "` would result in overflow.");
      }
      return lhs + rhs;
    }

  D_integer do_subtract(D_integer lhs, D_integer rhs)
    {
      using Limits = std::numeric_limits<D_integer>;
      if((rhs >= 0) ? (lhs < Limits::min() + rhs) : (lhs > Limits::max() + rhs)) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral subtraction of `", lhs, "` and `", rhs, "` would result in overflow.");
      }
      return lhs - rhs;
    }

  D_integer do_multiply(D_integer lhs, D_integer rhs)
    {
      using Limits = std::numeric_limits<D_integer>;
      if((lhs == 0) || (rhs == 0)) {
        return 0;
      }
      if((lhs == 1) || (rhs == 1)) {
        return lhs ^ rhs ^ 1;
      }
      if((lhs == Limits::min()) || (rhs == Limits::min())) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication of `", lhs, "` and `", rhs, "` would result in overflow.");
      }
      if((lhs == -1) || (rhs == -1)) {
        return -(lhs ^ rhs ^ -1);
      }
      const auto slhs = (rhs >= 0) ? lhs : -lhs;
      const auto arhs = std::abs(rhs);
      if((slhs >= 0) ? (slhs > Limits::max() / arhs) : (slhs < Limits::min() / arhs)) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication of `", lhs, "` and `", rhs, "` would result in overflow.");
      }
      return slhs * arhs;
    }

  D_integer do_divide(D_integer lhs, D_integer rhs)
    {
      using Limits = std::numeric_limits<D_integer>;
      if(rhs == 0) {
        ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
      }
      if((lhs == Limits::min()) && (rhs == -1)) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral division of `", lhs, "` and `", rhs, "` would result in overflow.");
      }
      return lhs / rhs;
    }

  D_integer do_modulo(D_integer lhs, D_integer rhs)
    {
      using Limits = std::numeric_limits<D_integer>;
      if(rhs == 0) {
        ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
      }
      if((lhs == Limits::min()) && (rhs == -1)) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral division of `", lhs, "` and `", rhs, "` would result in overflow.");
      }
      return lhs % rhs;
    }

  D_integer do_shift_left_logical(D_integer lhs, D_integer rhs)
    {
      using Limits = std::numeric_limits<D_integer>;
      if(rhs < 0) {
        ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
      }
      if(rhs > Limits::digits) {
        return 0;
      }
      auto reg = static_cast<Uint64>(lhs);
      reg <<= rhs;
      return static_cast<D_integer>(reg);
    }

  D_integer do_shift_right_logical(D_integer lhs, D_integer rhs)
    {
      using Limits = std::numeric_limits<D_integer>;
      if(rhs < 0) {
        ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
      }
      if(rhs > Limits::digits) {
        return 0;
      }
      auto reg = static_cast<Uint64>(lhs);
      reg >>= rhs;
      return static_cast<D_integer>(reg);
    }

  D_integer do_shift_left_arithmetic(D_integer lhs, D_integer rhs)
    {
      using Limits = std::numeric_limits<D_integer>;
      if(rhs < 0) {
        ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
      }
      if(rhs > Limits::digits) {
        ASTERIA_THROW_RUNTIME_ERROR("Arithmetic bit shift count `", rhs, "` for `", lhs, "` is larger than the width of an `integer`.");
      }
      const auto bits_rem = static_cast<unsigned char>(Limits::digits - rhs);
      auto reg = static_cast<Uint64>(lhs);
      const auto mask_out = (reg >> bits_rem) << bits_rem;
      const auto mask_sign = -(reg >> Limits::digits) << bits_rem;
      if(mask_out != mask_sign) {
        ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
      }
      reg <<= rhs;
      return static_cast<D_integer>(reg);
    }

  D_integer do_shift_right_arithmetic(D_integer lhs, D_integer rhs)
    {
      using Limits = std::numeric_limits<D_integer>;
      if(rhs < 0) {
        ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
      }
      if(rhs > Limits::digits) {
        ASTERIA_THROW_RUNTIME_ERROR("Arithmetic bit shift count `", rhs, "` for `", lhs, "` is larger than the width of an `integer`.");
      }
      const auto bits_rem = static_cast<unsigned char>(Limits::digits - rhs);
      auto reg = static_cast<Uint64>(lhs);
      const auto mask_in = -(reg >> Limits::digits) << bits_rem;
      reg >>= rhs;
      reg |= mask_in;
      return static_cast<D_integer>(reg);
    }

  D_integer do_bitwise_not(D_integer rhs)
    {
      return ~rhs;
    }

  D_integer do_bitwise_and(D_integer lhs, D_integer rhs)
    {
      return lhs & rhs;
    }

  D_integer do_bitwise_or(D_integer lhs, D_integer rhs)
    {
      return lhs | rhs;
    }

  D_integer do_bitwise_xor(D_integer lhs, D_integer rhs)
    {
      return lhs ^ rhs;
    }

  D_real do_negate(D_real rhs)
    {
      return -rhs;
    }

  D_real do_add(D_real lhs, D_real rhs)
    {
      return lhs + rhs;
    }

  D_real do_subtract(D_real lhs, D_real rhs)
    {
      return lhs - rhs;
    }

  D_real do_multiply(D_real lhs, D_real rhs)
    {
      return lhs * rhs;
    }

  D_real do_divide(D_real lhs, D_real rhs)
    {
      return lhs / rhs;
    }

  D_real do_modulo(D_real lhs, D_real rhs)
    {
      return std::fmod(lhs, rhs);
    }

  D_string do_concatenate(const D_string &lhs, const D_string &rhs)
    {
      D_string res;
      res.reserve(lhs.size() + rhs.size());
      res.append(lhs);
      res.append(rhs);
      return res;
    }

  D_string do_duplicate(const D_string &lhs, D_integer rhs)
    {
      if(rhs < 0) {
        ASTERIA_THROW_RUNTIME_ERROR("String duplication count `", rhs, "` for `", lhs, "` is negative.");
      }
      D_string res;
      const auto count = static_cast<Uint64>(rhs);
      if(count != 0) {
        if(lhs.size() > res.max_size() / count) {
          ASTERIA_THROW_RUNTIME_ERROR("Duplication of `", lhs, "` up to `", rhs, "` times would result in an overlong string that cannot be allocated.");
        }
        res.reserve(lhs.size() * static_cast<Size>(count));
        auto mask = std::numeric_limits<Size>::max() / 2 + 1;
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

}

void Xpnode::evaluate(Vector<Reference> &stack_io, Global_context *global_opt, const Executive_context &ctx) const
  {
    switch(Index(this->m_stor.index())) {
      case index_literal: {
        const auto &alt = this->m_stor.as<S_literal>();
        // Push the constant.
        Reference_root::S_constant ref_c = { alt.value };
        stack_io.emplace_back(std::move(ref_c));
        return;
      }
      case index_named_reference: {
        const auto &alt = this->m_stor.as<S_named_reference>();
        // Look for the reference in the current context.
        const auto pair = do_name_lookup(global_opt, ctx, alt.name);
        if(pair.first->is_analytic()) {
          ASTERIA_THROW_RUNTIME_ERROR("Expressions cannot be evaluated in analytic contexts.");
        }
        // Push the reference found.
        stack_io.emplace_back(*(pair.second));
        return;
      }
      case index_bound_reference: {
        const auto &alt = this->m_stor.as<S_bound_reference>();
        // Push the reference stored.
        stack_io.emplace_back(alt.ref);
        return;
      }
      case index_closure_function: {
        const auto &alt = this->m_stor.as<S_closure_function>();
        // Instantiate the closure function.
        auto func = alt.body.instantiate_function(global_opt, ctx, alt.file, alt.line, String::shallow("<closure function>"), alt.params);
        Reference_root::S_temporary ref_c = { D_function(std::move(func)) };
        stack_io.emplace_back(std::move(ref_c));
        return;
      }
      case index_branch: {
        const auto &alt = this->m_stor.as<S_branch>();
        // Pop the condition off the stack.
        auto cond = do_pop_reference(stack_io);
        // Read the condition and pick a branch.
        const auto stack_size_old = stack_io.size();
        const auto has_result = (cond.read().test() ? alt.branch_true : alt.branch_false).evaluate_partial(stack_io, global_opt, ctx);
        if(has_result) {
          ROCKET_ASSERT(stack_io.size() == stack_size_old + 1);
          // The result will have been pushed onto `stack_io`.
          ASTERIA_DEBUG_LOG("Setting branch result: ", stack_io.back().read());
          if(alt.assign) {
            auto &result = stack_io.mut_back();
            cond.write(result.read());
            result = std::move(cond);
          }
          return;
        }
        ROCKET_ASSERT(stack_io.size() == stack_size_old);
        // Push the condition if the branch is empty.
        ASTERIA_DEBUG_LOG("Forwarding the condition as-is: ", cond.read());
        stack_io.emplace_back(std::move(cond));
        return;
      }
      case index_function_call: {
        const auto &alt = this->m_stor.as<S_function_call>();
        // Allocate the argument vector.
        Vector<Reference> args;
        args.resize(alt.arg_cnt);
        for(auto i = alt.arg_cnt - 1; i + 1 != 0; --i) {
          auto arg = do_pop_reference(stack_io);
          args.mut(i) = std::move(arg);
        }
        // Pop the target off the stack.
        auto tgt = do_pop_reference(stack_io);
        const auto tgt_value = tgt.read();
        // Make sure it is really a function.
        const auto qfunc = tgt_value.opt<D_function>();
        if(!qfunc) {
          ASTERIA_THROW_RUNTIME_ERROR("`", tgt_value, "` is not a function and cannot be called.");
        }
        // This is the `this` reference.
        auto self = std::move(tgt.zoom_out());
        ASTERIA_DEBUG_LOG("Beginning function call at \'", alt.file, ':', alt.line, "\'...\n",
                          qfunc->get()->describe());
        try {
          tgt = qfunc->get()->invoke(global_opt, std::move(self), std::move(args));
          ASTERIA_DEBUG_LOG("Returned from function call at \'", alt.file, ':', alt.line, "\'...");
        } catch(Exception &except) {
          ASTERIA_DEBUG_LOG("Caught `Asteria::Exception` thrown inside function call at \'", alt.file, ':', alt.line, "\': value = ", except.get_value());
          // Append backtrace information and rethrow the exception.
          except.append_backtrace(alt.file, alt.line);
          throw;
        } catch(std::exception &stdex) {
          ASTERIA_DEBUG_LOG("Caught `std::exception` thrown inside function call at \'", alt.file, ':', alt.line, "\': what = ", stdex.what());
          // Here we behave as if a `string` had been thrown.
          Exception except(stdex);
          except.append_backtrace(alt.file, alt.line);
          throw except;
        }
        stack_io.emplace_back(std::move(tgt));
        return;
      }
      case index_subscript: {
        const auto &alt = this->m_stor.as<S_subscript>();
        // Get the subscript.
        Value sub_value;
        if(!alt.name.empty()) {
          sub_value = D_string(alt.name);
        } else {
          auto sub = do_pop_reference(stack_io);
          sub_value = sub.read();
        }
        auto cursor = do_pop_reference(stack_io);
        // The subscript operand shall have type `integer` or `string`.
        switch(rocket::weaken_enum(sub_value.type())) {
          case Value::type_integer: {
            Reference_modifier::S_array_index mod_c = { sub_value.check<D_integer>() };
            cursor.zoom_in(std::move(mod_c));
            break;
          }
          case Value::type_string: {
            Reference_modifier::S_object_key mod_c = { sub_value.check<D_string>() };
            cursor.zoom_in(std::move(mod_c));
            break;
          }
          default: {
            ASTERIA_THROW_RUNTIME_ERROR("The value `", sub_value, "` cannot be used as a subscript.");
          }
        }
        stack_io.emplace_back(std::move(cursor));
        return;
      }
      case index_operator_rpn: {
        const auto &alt = this->m_stor.as<S_operator_rpn>();
        switch(alt.xop) {
          case xop_postfix_inc: {
            // Increment the operand and return the old value.
            // `assign` is ignored.
            auto lhs = do_pop_reference(stack_io);
            auto lhs_value = lhs.read();
            if(lhs_value.type() == Value::type_integer) {
              auto result = lhs_value.check<D_integer>();
              do_set_result(lhs, true, do_add(result, D_integer(1)));
              do_set_result(lhs, false, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if(lhs_value.type() == Value::type_real) {
              auto result = lhs_value.check<D_real>();
              do_set_result(lhs, true, do_add(result, D_real(1)));
              do_set_result(lhs, false, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "`.");
          }
          case xop_postfix_dec: {
            // Decrement the operand and return the old value.
            // `assign` is ignored.
            auto lhs = do_pop_reference(stack_io);
            auto lhs_value = lhs.read();
            if(lhs_value.type() == Value::type_integer) {
              auto result = lhs_value.check<D_integer>();
              do_set_result(lhs, true, do_subtract(result, D_integer(1)));
              do_set_result(lhs, false, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if(lhs_value.type() == Value::type_real) {
              auto result = lhs_value.check<D_real>();
              do_set_result(lhs, true, do_subtract(result, D_real(1)));
              do_set_result(lhs, false, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "`.");
          }
          case xop_prefix_pos: {
            auto rhs = do_pop_reference(stack_io);
            // Copy the operand to create an rvalue, then return it.
            // N.B. This is one of the few operators that work on all types.
            auto result = rhs.read();
            do_set_result(rhs, alt.assign, std::move(result));
            stack_io.emplace_back(std::move(rhs));
            break;
          }
          case xop_prefix_neg: {
            auto rhs = do_pop_reference(stack_io);
            // Negate the operand to create an rvalue, then return it.
            auto rhs_value = rhs.read();
            if(rhs_value.type() == Value::type_integer) {
              auto result = do_negate(rhs_value.check<D_integer>(), rhs.is_constant());
              do_set_result(rhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(rhs));
              break;
            }
            if(rhs_value.type() == Value::type_string) {
              auto result = do_negate(rhs_value.check<D_real>());
              do_set_result(rhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(rhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", rhs_value, "`.");
          }
          case xop_prefix_notb: {
            auto rhs = do_pop_reference(stack_io);
            // Perform bitwise not operation on the operand to create an rvalue, then return it.
            auto rhs_value = rhs.read();
            if(rhs_value.type() == Value::type_boolean) {
              auto result = do_logical_not(rhs_value.check<D_boolean>());
              do_set_result(rhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(rhs));
              break;
            }
            if(rhs_value.type() == Value::type_integer) {
              auto result = do_bitwise_not(rhs_value.check<D_integer>());
              do_set_result(rhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(rhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", rhs_value, "`.");
          }
          case xop_prefix_notl: {
            auto rhs = do_pop_reference(stack_io);
            // Perform logical NOT operation on the operand to create an rvalue, then return it.
            // N.B. This is one of the few operators that work on all types.
            auto rhs_value = rhs.read();
            auto result = !rhs_value.test();
            do_set_result(rhs, alt.assign, std::move(result));
            stack_io.emplace_back(std::move(rhs));
            break;
          }
          case xop_prefix_inc: {
            auto rhs = do_pop_reference(stack_io);
            // Increment the operand and return it.
            // `assign` is ignored.
            auto rhs_value = rhs.read();
            if(rhs_value.type() == Value::type_integer) {
              auto result = do_add(rhs_value.check<D_integer>(), D_integer(1));
              do_set_result(rhs, true, std::move(result));
              stack_io.emplace_back(std::move(rhs));
              break;
            }
            if(rhs_value.type() == Value::type_real) {
              auto result = do_add(rhs_value.check<D_real>(), D_real(1));
              do_set_result(rhs, true, std::move(result));
              stack_io.emplace_back(std::move(rhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", rhs_value, "`.");
          }
          case xop_prefix_dec: {
            auto rhs = do_pop_reference(stack_io);
            // Decrement the operand and return it.
            // `assign` is ignored.
            auto rhs_value = rhs.read();
            if(rhs_value.type() == Value::type_integer) {
              auto result = do_subtract(rhs_value.check<D_integer>(), D_integer(1));
              do_set_result(rhs, true, std::move(result));
              stack_io.emplace_back(std::move(rhs));
              break;
            }
            if(rhs_value.type() == Value::type_real) {
              auto result = do_subtract(rhs_value.check<D_real>(), D_real(1));
              do_set_result(rhs, true, std::move(result));
              stack_io.emplace_back(std::move(rhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", rhs_value, "`.");
          }
          case xop_prefix_unset: {
            auto rhs = do_pop_reference(stack_io);
            // Unset the reference and return the value unset.
            auto result = rhs.unset();
            do_set_result(rhs, alt.assign, std::move(result));
            stack_io.emplace_back(std::move(rhs));
            break;
          }
          case xop_prefix_lengthof: {
            auto rhs = do_pop_reference(stack_io);
            // Return the number of elements in `rhs`.
            auto rhs_value = rhs.read();
            if(rhs_value.type() == Value::type_null) {
              auto result = D_integer(0);
              do_set_result(rhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(rhs));
              break;
            }
            if(rhs_value.type() == Value::type_string) {
              auto result = D_integer(rhs_value.check<D_string>().size());
              do_set_result(rhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(rhs));
              break;
            }
            if(rhs_value.type() == Value::type_array) {
              auto result = D_integer(rhs_value.check<D_array>().size());
              do_set_result(rhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(rhs));
              break;
            }
            if(rhs_value.type() == Value::type_object) {
              auto result = D_integer(rhs_value.check<D_object>().size());
              do_set_result(rhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(rhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", rhs_value, "`.");
          }
          case xop_infix_cmp_eq: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // Report unordered operands as being unequal.
            // N.B. This is one of the few operators that work on all types.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            auto result = comp == Value::compare_equal;
            do_set_result(lhs, false, result);
            stack_io.emplace_back(std::move(lhs));
            break;
          }
          case xop_infix_cmp_ne: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // Report unordered operands as being unequal.
            // N.B. This is one of the few operators that work on all types.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            auto result = comp != Value::compare_equal;
            do_set_result(lhs, false, result);
            stack_io.emplace_back(std::move(lhs));
            break;
          }
          case xop_infix_cmp_lt: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // Throw an exception in case of unordered operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_value, "` and `", rhs_value, "` are uncomparable.");
            }
            auto result = comp == Value::compare_less;
            do_set_result(lhs, false, result);
            stack_io.emplace_back(std::move(lhs));
            break;
          }
          case xop_infix_cmp_gt: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // Throw an exception in case of unordered operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_value, "` and `", rhs_value, "` are uncomparable.");
            }
            auto result = comp == Value::compare_greater;
            do_set_result(lhs, false, result);
            stack_io.emplace_back(std::move(lhs));
            break;
          }
          case xop_infix_cmp_lte: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // Throw an exception in case of unordered operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_value, "` and `", rhs_value, "` are uncomparable.");
            }
            auto result = comp != Value::compare_greater;
            do_set_result(lhs, false, result);
            stack_io.emplace_back(std::move(lhs));
            break;
          }
          case xop_infix_cmp_gte: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // Throw an exception in case of unordered operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_value, "` and `", rhs_value, "` are uncomparable.");
            }
            auto result = comp != Value::compare_less;
            do_set_result(lhs, false, result);
            stack_io.emplace_back(std::move(lhs));
            break;
          }
          case xop_infix_cmp_3way: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // N.B. This is one of the few operators that work on all types.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            switch(comp) {
              case Value::compare_less: {
                do_set_result(lhs, false, D_integer(-1));
                break;
              }
              case Value::compare_equal: {
                do_set_result(lhs, false, D_integer(0));
                break;
              }
              case Value::compare_greater: {
                do_set_result(lhs, false, D_integer(+1));
                break;
              }
              case Value::compare_unordered: {
                do_set_result(lhs, false, D_string(String::shallow("unordered")));
                break;
              }
              default: {
                ASTERIA_TERMINATE("An unknown comparison result `", comp, "` has been encountered.");
              }
            }
            stack_io.emplace_back(std::move(lhs));
            break;
          }
          case xop_infix_add: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // For the `boolean` type, return the logical OR'd result of both operands.
            // For the `integer` and `real` types, return the sum of both operands.
            // For the `string` type, concatenate the operands in lexical order to create a new string, then return it.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_boolean) && (rhs_value.type() == Value::type_boolean)) {
              auto result = do_logical_or(lhs_value.check<D_boolean>(), rhs_value.check<D_boolean>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_add(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_real) && (rhs_value.type() == Value::type_real)) {
              auto result = do_add(lhs_value.check<D_real>(), rhs_value.check<D_real>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_string) && (rhs_value.type() == Value::type_string)) {
              auto result = do_concatenate(lhs_value.check<D_string>(), rhs_value.check<D_string>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
          case xop_infix_sub: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // For the `boolean` type, return the logical XOR'd result of both operands.
            // For the `integer` and `real` types, return the difference of both operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_boolean) && (rhs_value.type() == Value::type_boolean)) {
              auto result = do_logical_xor(lhs_value.check<D_boolean>(), rhs_value.check<D_boolean>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_subtract(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_real) && (rhs_value.type() == Value::type_real)) {
              auto result = do_subtract(lhs_value.check<D_real>(), rhs_value.check<D_real>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
          case xop_infix_mul: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // For the boolean type, return the logical AND'd result of both operands.
            // For the integer and real types, return the product of both operands.
            // If either operand has the integer type and the other has the string type, duplicate the string up to the specified number of times.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_boolean) && (rhs_value.type() == Value::type_boolean)) {
              auto result = do_logical_and(lhs_value.check<D_boolean>(), rhs_value.check<D_boolean>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_multiply(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_real) && (rhs_value.type() == Value::type_real)) {
              auto result = do_multiply(lhs_value.check<D_real>(), rhs_value.check<D_real>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_string) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_duplicate(lhs_value.check<D_string>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_string)) {
              auto result = do_duplicate(rhs_value.check<D_string>(), lhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
          case xop_infix_div: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // For the integer and real types, return the quotient of both operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_divide(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_real) && (rhs_value.type() == Value::type_real)) {
              auto result = do_divide(lhs_value.check<D_real>(), rhs_value.check<D_real>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
          case xop_infix_mod: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // For the integer and real types, return the reminder of both operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_modulo(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_real) && (rhs_value.type() == Value::type_real)) {
              auto result = do_modulo(lhs_value.check<D_real>(), rhs_value.check<D_real>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
          case xop_infix_sll: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // Shift the first operand to the left by the number of bits specified by the second operand
            // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
            // Both operands have to be integers.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_shift_left_logical(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
          case xop_infix_srl: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // Shift the first operand to the right by the number of bits specified by the second operand
            // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
            // Both operands have to be integers.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_shift_right_logical(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
          case xop_infix_sla: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // Shift the first operand to the left by the number of bits specified by the second operand
            // Bits shifted out that equal the sign bit are dicarded. Bits shifted in are filled with zeroes.
            // If a bit unequal to the sign bit would be shifted into or across the sign bit, an exception is thrown.
            // Both operands have to be integers.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_shift_left_arithmetic(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
          case xop_infix_sra: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // Shift the first operand to the right by the number of bits specified by the second operand
            // Bits shifted out are discarded. Bits shifted in are filled with the sign bit.
            // Both operands have to be integers.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_shift_right_arithmetic(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
          case xop_infix_andb: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // For the `boolean` type, return the logical AND'd result of both operands.
            // For the `integer` type, return the bitwise AND'd result of both operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_boolean) && (rhs_value.type() == Value::type_boolean)) {
              auto result = do_logical_and(lhs_value.check<D_boolean>(), rhs_value.check<D_boolean>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_bitwise_and(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
          case xop_infix_orb: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // For the `boolean` type, return the logical OR'd result of both operands.
            // For the `integer` type, return the bitwise OR'd result of both operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_boolean) && (rhs_value.type() == Value::type_boolean)) {
              auto result = do_logical_or(lhs_value.check<D_boolean>(), rhs_value.check<D_boolean>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_bitwise_or(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
          case xop_infix_xorb: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // For the `boolean` type, return the logical XOR'd result of both operands.
            // For the `integer` type, return the bitwise XOR'd result of both operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_boolean) && (rhs_value.type() == Value::type_boolean)) {
              auto result = do_logical_xor(lhs_value.check<D_boolean>(), rhs_value.check<D_boolean>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_bitwise_xor(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.assign, std::move(result));
              stack_io.emplace_back(std::move(lhs));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
          case xop_infix_assign: {
            auto rhs = do_pop_reference(stack_io);
            auto lhs = do_pop_reference(stack_io);
            // Copy the operand referenced by `rhs` to `lhs`.
            // `assign` is ignored.
            // N.B. This is one of the few operators that work on all types.
            auto result = rhs.read();
            do_set_result(lhs, true, std::move(result));
            stack_io.emplace_back(std::move(lhs));
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
          auto ref = do_pop_reference(stack_io);
          array.mut(i) = ref.read();
        }
        Reference_root::S_temporary ref_c = { std::move(array) };
        stack_io.emplace_back(std::move(ref_c));
        return;
      }
      case index_unnamed_object: {
        const auto &alt = this->m_stor.as<S_unnamed_object>();
        // Pop references to create an object.
        D_object object;
        object.reserve(alt.keys.size());
        for(auto it = alt.keys.rbegin(); it != alt.keys.rend(); ++it) {
          auto ref = do_pop_reference(stack_io);
          object.insert_or_assign(*it, ref.read());
        }
        Reference_root::S_temporary ref_c = { std::move(object) };
        stack_io.emplace_back(std::move(ref_c));
        return;
      }
      case index_coalescence: {
        const auto &alt = this->m_stor.as<S_coalescence>();
        // Pop the condition off the stack.
        auto cond = do_pop_reference(stack_io);
        // Read the condition. If it is null, evaluate the branch.
        if(cond.read().type() == Value::type_null) {
          const auto stack_size_old = stack_io.size();
          const auto has_result = alt.branch_null.evaluate_partial(stack_io, global_opt, ctx);
          if(has_result) {
            ROCKET_ASSERT(stack_io.size() == stack_size_old + 1);
            // The result will have been pushed onto `stack_io`.
            ASTERIA_DEBUG_LOG("Setting branch result: ", stack_io.back().read());
            if(alt.assign) {
              auto &result = stack_io.mut_back();
              cond.write(result.read());
              result = std::move(cond);
            }
            return;
          }
          ROCKET_ASSERT(stack_io.size() == stack_size_old);
        }
        // Push the condition back.
        ASTERIA_DEBUG_LOG("Forwarding the condition as-is: ", cond.read());
        stack_io.emplace_back(std::move(cond));
        return;
      }
      default: {
        ASTERIA_TERMINATE("An unknown expression node type enumeration `", this->m_stor.index(), "` has been encountered.");
      }
    }
  }

}
