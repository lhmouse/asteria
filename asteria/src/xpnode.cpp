// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "analytic_context.hpp"
#include "executive_context.hpp"
#include "abstract_function.hpp"
#include "instantiated_function.hpp"
#include "backtracer.hpp"
#include "utilities.hpp"

namespace Asteria {

const char * Xpnode::get_operator_name(Xpnode::Xop xop) noexcept
  {
    switch(xop) {
    case Xpnode::xop_postfix_inc:
      return "postfix increment";
    case Xpnode::xop_postfix_dec:
      return "postfix decrement";
    case Xpnode::xop_postfix_at:
      return "subscripting";
    case Xpnode::xop_prefix_pos:
      return "unary plus";
    case Xpnode::xop_prefix_neg:
      return "unary negation";
    case Xpnode::xop_prefix_notb:
      return "bitwise not";
    case Xpnode::xop_prefix_notl:
      return "logical not";
    case Xpnode::xop_prefix_inc:
      return "prefix increment";
    case Xpnode::xop_prefix_dec:
      return "prefix decrement";
    case Xpnode::xop_prefix_unset:
      return "prefix `unset`";
    case Xpnode::xop_infix_cmp_eq:
      return "equality comparison";
    case Xpnode::xop_infix_cmp_ne:
      return "inequality comparison";
    case Xpnode::xop_infix_cmp_lt:
      return "less-than comparison";
    case Xpnode::xop_infix_cmp_gt:
      return "greater-than comparison";
    case Xpnode::xop_infix_cmp_lte:
      return "less-than-or-equal comparison";
    case Xpnode::xop_infix_cmp_gte:
      return "greater-than-or-equal comparison";
    case Xpnode::xop_infix_add:
      return "addition";
    case Xpnode::xop_infix_sub:
      return "subtraction";
    case Xpnode::xop_infix_mul:
      return "multiplication";
    case Xpnode::xop_infix_div:
      return "division";
    case Xpnode::xop_infix_mod:
      return "modulo";
    case Xpnode::xop_infix_sll:
      return "logical left shift";
    case Xpnode::xop_infix_srl:
      return "arithmetic left shift";
    case Xpnode::xop_infix_sla:
      return "logical right shift";
    case Xpnode::xop_infix_sra:
      return "arithmetic right shift";
    case Xpnode::xop_infix_andb:
      return "bitwise and";
    case Xpnode::xop_infix_orb:
      return "bitwise or";
    case Xpnode::xop_infix_xorb:
      return "bitwise xor";
    case Xpnode::xop_infix_assign:
      return "assginment";
    default:
      ASTERIA_TERMINATE("An unknown operator type enumeration `", xop, "` has been encountered.");
    }
  }

Xpnode::~Xpnode()
  {
  }

Xpnode::Xpnode(Xpnode &&) noexcept
  = default;
Xpnode & Xpnode::operator=(Xpnode &&) noexcept
  = default;

namespace {

  std::pair<const Abstract_context *, const Reference *> do_name_lookup(const Abstract_context &ctx, const String &name)
    {
      auto qctx = &ctx;
      do {
        const auto qref = qctx->get_named_reference_opt(name);
        if(qref) {
          return std::make_pair(qctx, qref);
        }
        qctx = qctx->get_parent_opt();
        if(!qctx) {
          ASTERIA_THROW_RUNTIME_ERROR("The identifier `", name, "` has not been declared yet.");
        }
      } while(true);
    }

}

Xpnode Xpnode::bind(const Analytic_context &ctx) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case Xpnode::index_literal:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_literal>();
        // Copy it as-is.
        Xpnode::S_literal alt_bnd = { alt.value };
        return std::move(alt_bnd);
      }
    case Xpnode::index_named_reference:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_named_reference>();
        // Only references with non-reserved names can be bound.
        if(ctx.is_name_reserved(alt.name) == false) {
          // Look for the reference in the current context.
          // Don't bind it onto something in a analytic context which will soon get destroyed.
          const auto pair = do_name_lookup(ctx, alt.name);
          if(pair.first->is_analytic() == false) {
            Xpnode::S_bound_reference alt_bnd = { *(pair.second) };
            return std::move(alt_bnd);
          }
        }
        // Copy it as-is.
        Xpnode::S_named_reference alt_bnd = { alt.name };
        return std::move(alt_bnd);
      }
    case Xpnode::index_bound_reference:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_bound_reference>();
        // Copy it as-is.
        Xpnode::S_bound_reference alt_bnd = { alt.ref };
        return std::move(alt_bnd);
      }
    case Xpnode::index_subexpression:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_subexpression>();
        // Bind the subexpression recursively.
        auto expr_bnd = alt.expr.bind(ctx);
        Xpnode::S_subexpression alt_bnd = { std::move(expr_bnd) };
        return std::move(alt_bnd);
      }
    case Xpnode::index_closure_function:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_closure_function>();
        // Bind the body recursively.
        Analytic_context ctx_next(&ctx);
        ctx_next.initialize_for_function(alt.params);
        auto body_bnd = alt.body.bind_in_place(ctx_next);
        Xpnode::S_closure_function alt_bnd = { alt.params, alt.file, alt.line, std::move(body_bnd) };
        return std::move(alt_bnd);
      }
    case Xpnode::index_branch:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_branch>();
        // Bind both branches recursively.
        auto branch_true_bnd = alt.branch_true.bind(ctx);
        auto branch_false_bnd = alt.branch_false.bind(ctx);
        Xpnode::S_branch alt_bnd = { std::move(branch_true_bnd), std::move(branch_false_bnd) };
        return std::move(alt_bnd);
      }
    case Xpnode::index_function_call:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_function_call>();
        // Copy it as-is.
        Xpnode::S_function_call alt_bnd = { alt.file, alt.line, alt.arg_cnt };
        return std::move(alt_bnd);
      }
    case Xpnode::index_operator_rpn:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_operator_rpn>();
        // Copy it as-is.
        Xpnode::S_operator_rpn alt_bnd = { alt.xop, alt.compound_assign };
        return std::move(alt_bnd);
      }
    case Xpnode::index_unnamed_array:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_unnamed_array>();
        // Bind everything recursively.
        Vector<Expression> elems_bnd;
        elems_bnd.reserve(alt.elems.size());
        for(const auto &elem : alt.elems) {
          auto elem_bnd = elem.bind(ctx);
          elems_bnd.emplace_back(std::move(elem_bnd));
        }
        Xpnode::S_unnamed_array alt_bnd = { std::move(elems_bnd) };
        return std::move(alt_bnd);
      }
    case Xpnode::index_unnamed_object:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_unnamed_object>();
        // Bind everything recursively.
        Dictionary<Expression> pairs_bnd;
        pairs_bnd.reserve(alt.pairs.size());
        for(const auto &pair : alt.pairs) {
          auto second_bnd = pair.second.bind(ctx);
          pairs_bnd.insert_or_assign(pair.first, std::move(second_bnd));
        }
        Xpnode::S_unnamed_object alt_bnd = { std::move(pairs_bnd) };
        return std::move(alt_bnd);
      }
    default:
      ASTERIA_TERMINATE("An unknown expression node type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

namespace {

  Reference do_pop_reference(Vector<Reference> &stack_inout)
    {
      if(stack_inout.empty()) {
        ASTERIA_THROW_RUNTIME_ERROR("The evaluation stack is empty, which means the expression is probably invalid.");
      }
      auto ref = std::move(stack_inout.mut_back());
      stack_inout.pop_back();
      return ref;
    }

  void do_set_result(Reference &ref_inout, bool compound_assign, Value value)
    {
      if(compound_assign) {
        // Write the value through `ref_inout`, which is unchanged.
        ref_inout.write(std::move(value));
        return;
      }
      // Create an rvalue reference and assign it to `ref_inout`.
      Reference_root::S_temporary ref_c = { std::move(value) };
      ref_inout = std::move(ref_c);
    }

  Reference do_traced_call(const String &file, Uint64 line, const D_function &func, Reference &&self, Vector<Reference> &&args)
    try {
      return func->invoke(std::move(self), std::move(args));
    } catch(...) {
      ASTERIA_DEBUG_LOG("  Forwarding exception thrown insode \'", file, ':', line, "\'...");
      throw Backtracer(file, line);
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
      using limits = std::numeric_limits<D_integer>;
      if(rhs == limits::min()) {
        if(!wrap) {
          ASTERIA_THROW_RUNTIME_ERROR("Integral negation of `", rhs, "` would result in overflow.");
        }
        return rhs;
      }
      return -rhs;
    }

  D_integer do_add(D_integer lhs, D_integer rhs)
    {
      using limits = std::numeric_limits<D_integer>;
      if((rhs >= 0) ? (lhs > limits::max() - rhs) : (lhs < limits::min() - rhs)) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral addition of `", lhs, "` and `", rhs, "` would result in overflow.");
      }
      return lhs + rhs;
    }

  D_integer do_subtract(D_integer lhs, D_integer rhs)
    {
      using limits = std::numeric_limits<D_integer>;
      if((rhs >= 0) ? (lhs < limits::min() + rhs) : (lhs > limits::max() + rhs)) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral subtraction of `", lhs, "` and `", rhs, "` would result in overflow.");
      }
      return lhs - rhs;
    }

  D_integer do_multiply(D_integer lhs, D_integer rhs)
    {
      using limits = std::numeric_limits<D_integer>;
      if((lhs == 0) || (rhs == 0)) {
        return 0;
      }
      if((lhs == 1) || (rhs == 1)) {
        return lhs ^ rhs ^ 1;
      }
      if((lhs == limits::min()) || (rhs == limits::min())) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication of `", lhs, "` and `", rhs, "` would result in overflow.");
      }
      if((lhs == -1) || (rhs == -1)) {
        return -(lhs ^ rhs ^ -1);
      }
      const auto slhs = (rhs >= 0) ? lhs : -lhs;
      const auto arhs = std::abs(rhs);
      if((slhs >= 0) ? (slhs > limits::max() / arhs) : (slhs < limits::min() / arhs)) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral multiplication of `", lhs, "` and `", rhs, "` would result in overflow.");
      }
      return slhs * arhs;
    }

  D_integer do_divide(D_integer lhs, D_integer rhs)
    {
      using limits = std::numeric_limits<D_integer>;
      if(rhs == 0) {
        ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
      }
      if((lhs == limits::min()) && (rhs == -1)) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral division of `", lhs, "` and `", rhs, "` would result in overflow.");
      }
      return lhs / rhs;
    }

  D_integer do_modulo(D_integer lhs, D_integer rhs)
    {
      using limits = std::numeric_limits<D_integer>;
      if(rhs == 0) {
        ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
      }
      if((lhs == limits::min()) && (rhs == -1)) {
        ASTERIA_THROW_RUNTIME_ERROR("Integral division of `", lhs, "` and `", rhs, "` would result in overflow.");
      }
      return lhs % rhs;
    }

  D_integer do_shift_left_logical(D_integer lhs, D_integer rhs)
    {
      using limits = std::numeric_limits<D_integer>;
      if(rhs < 0) {
        ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` was negative.");
      }
      if(rhs > limits::digits) {
        return 0;
      }
      auto reg = static_cast<Uint64>(lhs);
      reg <<= rhs;
      return static_cast<D_integer>(reg);
    }

  D_integer do_shift_right_logical(D_integer lhs, D_integer rhs)
    {
      using limits = std::numeric_limits<D_integer>;
      if(rhs < 0) {
        ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` was negative.");
      }
      if(rhs > limits::digits) {
        return 0;
      }
      auto reg = static_cast<Uint64>(lhs);
      reg >>= rhs;
      return static_cast<D_integer>(reg);
    }

  D_integer do_shift_left_arithmetic(D_integer lhs, D_integer rhs)
    {
      using limits = std::numeric_limits<D_integer>;
      if(rhs < 0) {
        ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` was negative.");
      }
      if(rhs > limits::digits) {
        ASTERIA_THROW_RUNTIME_ERROR("Arithmetic bit shift count `", rhs, "` for `", lhs, "` is larger than the width of an `integer`.");
      }
      const auto bits_rem = static_cast<unsigned char>(limits::digits - rhs);
      auto reg = static_cast<Uint64>(lhs);
      const auto mask_out = (reg >> bits_rem) << bits_rem;
      const auto mask_sign = -(reg >> limits::digits) << bits_rem;
      if(mask_out != mask_sign) {
        ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
      }
      reg <<= rhs;
      return static_cast<D_integer>(reg);
    }

  D_integer do_shift_right_arithmetic(D_integer lhs, D_integer rhs)
    {
      using limits = std::numeric_limits<D_integer>;
      if(rhs < 0) {
        ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` was negative.");
      }
      if(rhs > limits::digits) {
        ASTERIA_THROW_RUNTIME_ERROR("Arithmetic bit shift count `", rhs, "` for `", lhs, "` is larger than the width of an `integer`.");
      }
      const auto bits_rem = static_cast<unsigned char>(limits::digits - rhs);
      auto reg = static_cast<Uint64>(lhs);
      const auto mask_in = -(reg >> limits::digits) << bits_rem;
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
        ASTERIA_THROW_RUNTIME_ERROR("String duplication count `", rhs, "` for `", lhs, "` was negative.");
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

void Xpnode::evaluate(Vector<Reference> &stack_inout, const Executive_context &ctx) const
  {
    switch(static_cast<Index>(this->m_stor.index())) {
    case Xpnode::index_literal:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_literal>();
        // Push the constant.
        Reference_root::S_constant ref_c = { alt.value };
        stack_inout.emplace_back(std::move(ref_c));
        return;
      }
    case Xpnode::index_named_reference:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_named_reference>();
        // Look for the reference in the current context.
        const auto pair = do_name_lookup(ctx, alt.name);
        if(pair.first->is_analytic()) {
          ASTERIA_THROW_RUNTIME_ERROR("Expressions cannot be evaluated in analytic contexts.");
        }
        // Push the reference found.
        stack_inout.emplace_back(*(pair.second));
        return;
      }
    case Xpnode::index_bound_reference:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_bound_reference>();
        // Push the reference stored.
        stack_inout.emplace_back(alt.ref);
        return;
      }
    case Xpnode::index_subexpression:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_subexpression>();
        // Evaluate the subexpression recursively.
        auto ref = alt.expr.evaluate(ctx);
        stack_inout.emplace_back(std::move(ref));
        return;
      }
    case Xpnode::index_closure_function:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_closure_function>();
        // Bind the function body recursively.
        Analytic_context ctx_next(&ctx);
        ctx_next.initialize_for_function(alt.params);
        auto body_bnd = alt.body.bind_in_place(ctx_next);
        auto func = rocket::make_refcounted<Instantiated_function>(alt.params, alt.file, alt.line, std::move(body_bnd));
        Reference_root::S_temporary ref_c = { D_function(std::move(func)) };
        stack_inout.emplace_back(std::move(ref_c));
        return;
      }
    case Xpnode::index_branch:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_branch>();
        // Pop the condition off the stack.
        auto cond = do_pop_reference(stack_inout);
        // Pick a branch. If it is not empty, evaluate it and write the result to `cond`.
        // This means that if the branch taken is empty then `cond` is pushed.
        const auto branch_taken = cond.read().test() ? &(alt.branch_true) : &(alt.branch_false);
        if(branch_taken->empty() == false) {
          cond = branch_taken->evaluate(ctx);
        }
        stack_inout.emplace_back(std::move(cond));
        return;
      }
    case Xpnode::index_function_call:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_function_call>();
        // Pop the callee off the stack.
        auto callee = do_pop_reference(stack_inout);
        auto callee_value = callee.read();
        // Make sure it is really a function.
        const auto qfunc = callee_value.opt<D_function>();
        if(!qfunc) {
          ASTERIA_THROW_RUNTIME_ERROR("`", callee_value, "` is not a function and cannot be called.");
        }
        if(!*qfunc) {
          ASTERIA_THROW_RUNTIME_ERROR("An attempt to call a null function pointer has been made.");
        }
        // Allocate the argument vector.
        Vector<Reference> args;
        args.reserve(alt.arg_cnt);
        for(auto i = alt.arg_cnt; i != 0; --i) {
          auto arg = do_pop_reference(stack_inout);
          args.emplace_back(std::move(arg));
        }
        // Call the function and de-materialize the result.
        auto result = do_traced_call(alt.file, alt.line, *qfunc, std::move(callee.zoom_out()), std::move(args));
        result.dematerialize();
        stack_inout.emplace_back(std::move(result));
        return;
      }
    case Xpnode::index_operator_rpn:
      {
        // Pop the first operand off the stack.
        // For prefix operators, this is actually the RHS operand anyway.
        // This is also the object where the result will be stored.
        auto lhs = do_pop_reference(stack_inout);
        // Deal with individual operators.
        const auto &alt = this->m_stor.as<Xpnode::S_operator_rpn>();
        switch(alt.xop) {
        case Xpnode::xop_postfix_inc:
          {
            // Increment the operand and return the old value.
            // `compound_assign` is ignored.
            auto lhs_value = lhs.read();
            if(lhs_value.type() == Value::type_integer) {
              auto result = lhs_value.check<D_integer>();
              do_set_result(lhs, true, do_add(result, D_integer(1)));
              do_set_result(lhs, false, std::move(result));
              break;
            }
            if(lhs_value.type() == Value::type_real) {
              auto result = lhs_value.check<D_real>();
              do_set_result(lhs, true, do_add(result, D_real(1)));
              do_set_result(lhs, false, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "`.");
          }
        case Xpnode::xop_postfix_dec:
          {
            // Decrement the operand and return the old value.
            // `compound_assign` is ignored.
            auto lhs_value = lhs.read();
            if(lhs_value.type() == Value::type_integer) {
              auto result = lhs_value.check<D_integer>();
              do_set_result(lhs, true, do_subtract(result, D_integer(1)));
              do_set_result(lhs, false, std::move(result));
              break;
            }
            if(lhs_value.type() == Value::type_real) {
              auto result = lhs_value.check<D_real>();
              do_set_result(lhs, true, do_subtract(result, D_real(1)));
              do_set_result(lhs, false, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "`.");
          }
        case Xpnode::xop_postfix_at:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // The subscript operand shall have type `integer` or `string`.
            // `compound_assign` is ignored.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if(rhs_value.type() == Value::type_integer) {
              auto index = rhs_value.check<D_integer>();
              Reference_modifier::S_array_index mod_c = { index };
              lhs.zoom_in(std::move(mod_c));
              break;
            }
            if(rhs_value.type() == Value::type_string) {
              auto key = rhs_value.check<D_string>();
              Reference_modifier::S_object_key mod_c = { std::move(key) };
              lhs.zoom_in(std::move(mod_c));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("`", rhs_value, "` is not a valid member designator.");
          }
        case Xpnode::xop_prefix_pos:
          {
            // Copy the operand to create an rvalue, then return it.
            // N.B. This is one of the few operators that work on all types.
            auto result = lhs.read();
            do_set_result(lhs, alt.compound_assign, std::move(result));
            break;
          }
        case Xpnode::xop_prefix_neg:
          {
            // Negate the operand to create an rvalue, then return it.
            auto lhs_value = lhs.read();
            if(lhs_value.type() == Value::type_integer) {
              auto result = do_negate(lhs_value.check<D_integer>(), lhs.is_constant());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if(lhs_value.type() == Value::type_string) {
              auto result = do_negate(lhs_value.check<D_real>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "`.");
          }
        case Xpnode::xop_prefix_notb:
          {
            // Perform bitwise not operation on the operand to create an rvalue, then return it.
            auto lhs_value = lhs.read();
            if(lhs_value.type() == Value::type_boolean) {
              auto result = do_logical_not(lhs_value.check<D_boolean>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if(lhs_value.type() == Value::type_integer) {
              auto result = do_bitwise_not(lhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "`.");
          }
        case Xpnode::xop_prefix_notl:
          {
            // Perform logical NOT operation on the operand to create an rvalue, then return it.
            // N.B. This is one of the few operators that work on all types.
            auto lhs_value = lhs.read();
            auto result = lhs_value.test() == false;
            do_set_result(lhs, alt.compound_assign, std::move(result));
            break;
          }
        case Xpnode::xop_prefix_inc:
          {
            // Increment the operand and return it.
            // `compound_assign` is ignored.
            auto lhs_value = lhs.read();
            if(lhs_value.type() == Value::type_integer) {
              auto result = do_add(lhs_value.check<D_integer>(), D_integer(1));
              do_set_result(lhs, true, std::move(result));
              break;
            }
            if(lhs_value.type() == Value::type_real) {
              auto result = do_add(lhs_value.check<D_real>(), D_real(1));
              do_set_result(lhs, true, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "`.");
          }
        case Xpnode::xop_prefix_dec:
          {
            // Decrement the operand and return it.
            // `compound_assign` is ignored.
            auto lhs_value = lhs.read();
            if(lhs_value.type() == Value::type_integer) {
              auto result = do_subtract(lhs_value.check<D_integer>(), D_integer(1));
              do_set_result(lhs, true, std::move(result));
              break;
            }
            if(lhs_value.type() == Value::type_real) {
              auto result = do_subtract(lhs_value.check<D_real>(), D_real(1));
              do_set_result(lhs, true, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "`.");
          }
        case Xpnode::xop_prefix_unset:
          {
            // Unset the reference and return the value unset.
            auto result = lhs.unset();
            do_set_result(lhs, alt.compound_assign, std::move(result));
            break;
          }
        case Xpnode::xop_infix_cmp_eq:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Report unordered operands as being unequal.
            // N.B. This is one of the few operators that work on all types.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            auto result = comp == Value::compare_equal;
            do_set_result(lhs, false, result);
            break;
          }
        case Xpnode::xop_infix_cmp_ne:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Report unordered operands as being unequal.
            // N.B. This is one of the few operators that work on all types.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            auto result = comp != Value::compare_equal;
            do_set_result(lhs, false, result);
            break;
          }
        case Xpnode::xop_infix_cmp_lt:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Throw an exception in case of unordered operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_value, "` and `", rhs_value, "` are uncomparable.");
            }
            auto result = comp == Value::compare_less;
            do_set_result(lhs, false, result);
            break;
          }
        case Xpnode::xop_infix_cmp_gt:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Throw an exception in case of unordered operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_value, "` and `", rhs_value, "` are uncomparable.");
            }
            auto result = comp == Value::compare_greater;
            do_set_result(lhs, false, result);
            break;
          }
        case Xpnode::xop_infix_cmp_lte:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Throw an exception in case of unordered operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_value, "` and `", rhs_value, "` are uncomparable.");
            }
            auto result = comp != Value::compare_greater;
            do_set_result(lhs, false, result);
            break;
          }
        case Xpnode::xop_infix_cmp_gte:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Throw an exception in case of unordered operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            auto comp = lhs_value.compare(rhs_value);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_value, "` and `", rhs_value, "` are uncomparable.");
            }
            auto result = comp != Value::compare_less;
            do_set_result(lhs, false, result);
            break;
          }
        case Xpnode::xop_infix_add:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the `boolean` type, return the logical OR'd result of both operands.
            // For the `integer` and `real` types, return the sum of both operands.
            // For the `string` type, concatenate the operands in lexical order to create a new string, then return it.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_boolean) && (rhs_value.type() == Value::type_boolean)) {
              auto result = do_logical_or(lhs_value.check<D_boolean>(), rhs_value.check<D_boolean>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_add(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_real) && (rhs_value.type() == Value::type_real)) {
              auto result = do_add(lhs_value.check<D_real>(), rhs_value.check<D_real>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_string) && (rhs_value.type() == Value::type_string)) {
              auto result = do_concatenate(lhs_value.check<D_string>(), rhs_value.check<D_string>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
        case Xpnode::xop_infix_sub:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the `boolean` type, return the logical XOR'd result of both operands.
            // For the `integer` and `real` types, return the difference of both operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_boolean) && (rhs_value.type() == Value::type_boolean)) {
              auto result = do_logical_xor(lhs_value.check<D_boolean>(), rhs_value.check<D_boolean>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_subtract(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_real) && (rhs_value.type() == Value::type_real)) {
              auto result = do_subtract(lhs_value.check<D_real>(), rhs_value.check<D_real>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
        case Xpnode::xop_infix_mul:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the boolean type, return the logical AND'd result of both operands.
            // For the integer and real types, return the product of both operands.
            // If either operand has the integer type and the other has the string type, duplicate the string up to the specified number of times.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_boolean) && (rhs_value.type() == Value::type_boolean)) {
              auto result = do_logical_and(lhs_value.check<D_boolean>(), rhs_value.check<D_boolean>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_multiply(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_real) && (rhs_value.type() == Value::type_real)) {
              auto result = do_multiply(lhs_value.check<D_real>(), rhs_value.check<D_real>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_string) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_duplicate(lhs_value.check<D_string>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_string)) {
              auto result = do_duplicate(rhs_value.check<D_string>(), lhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
        case Xpnode::xop_infix_div:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the integer and real types, return the quotient of both operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_divide(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_real) && (rhs_value.type() == Value::type_real)) {
              auto result = do_divide(lhs_value.check<D_real>(), rhs_value.check<D_real>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
        case Xpnode::xop_infix_mod:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the integer and real types, return the reminder of both operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_modulo(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_real) && (rhs_value.type() == Value::type_real)) {
              auto result = do_modulo(lhs_value.check<D_real>(), rhs_value.check<D_real>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
        case Xpnode::xop_infix_sll:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Shift the first operand to the left by the number of bits specified by the second operand
            // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
            // Both operands have to be integers.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_shift_left_logical(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
        case Xpnode::xop_infix_srl:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Shift the first operand to the right by the number of bits specified by the second operand
            // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
            // Both operands have to be integers.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_shift_right_logical(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
        case Xpnode::xop_infix_sla:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Shift the first operand to the left by the number of bits specified by the second operand
            // Bits shifted out that equal the sign bit are dicarded. Bits shifted in are filled with zeroes.
            // If a bit unequal to the sign bit would be shifted into or across the sign bit, an exception is thrown.
            // Both operands have to be integers.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_shift_left_arithmetic(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
        case Xpnode::xop_infix_sra:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Shift the first operand to the right by the number of bits specified by the second operand
            // Bits shifted out are discarded. Bits shifted in are filled with the sign bit.
            // Both operands have to be integers.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_shift_right_arithmetic(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
        case Xpnode::xop_infix_andb:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the `boolean` type, return the logical AND'd result of both operands.
            // For the `integer` type, return the bitwise AND'd result of both operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_boolean) && (rhs_value.type() == Value::type_boolean)) {
              auto result = do_logical_and(lhs_value.check<D_boolean>(), rhs_value.check<D_boolean>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_bitwise_and(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
        case Xpnode::xop_infix_orb:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the `boolean` type, return the logical OR'd result of both operands.
            // For the `integer` type, return the bitwise OR'd result of both operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_boolean) && (rhs_value.type() == Value::type_boolean)) {
              auto result = do_logical_or(lhs_value.check<D_boolean>(), rhs_value.check<D_boolean>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_bitwise_or(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
        case Xpnode::xop_infix_xorb:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the `boolean` type, return the logical XOR'd result of both operands.
            // For the `integer` type, return the bitwise XOR'd result of both operands.
            auto lhs_value = lhs.read();
            auto rhs_value = rhs.read();
            if((lhs_value.type() == Value::type_boolean) && (rhs_value.type() == Value::type_boolean)) {
              auto result = do_logical_xor(lhs_value.check<D_boolean>(), rhs_value.check<D_boolean>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            if((lhs_value.type() == Value::type_integer) && (rhs_value.type() == Value::type_integer)) {
              auto result = do_bitwise_xor(lhs_value.check<D_integer>(), rhs_value.check<D_integer>());
              do_set_result(lhs, alt.compound_assign, std::move(result));
              break;
            }
            ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(alt.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
          }
        case Xpnode::xop_infix_assign:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Copy the operand referenced by `rhs` to `lhs`.
            // `compound_assign` is ignored.
            // N.B. This is one of the few operators that work on all types.
            auto result = rhs.read();
            do_set_result(lhs, true, std::move(result));
            break;
          }
        default:
          ASTERIA_TERMINATE("An unknown operator type enumeration `", alt.xop, "` has been encountered.");
        }
        stack_inout.emplace_back(std::move(lhs));
        return;
      }
    case Xpnode::index_unnamed_array:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_unnamed_array>();
        // Create an array by evaluating elements recursively.
        D_array array;
        array.reserve(alt.elems.size());
        for(const auto &elem : alt.elems) {
          const auto result = elem.evaluate(ctx);
          auto value = result.read();
          array.emplace_back(std::move(value));
        }
        Reference_root::S_temporary ref_c = { std::move(array) };
        stack_inout.emplace_back(std::move(ref_c));
        return;
      }
    case Xpnode::index_unnamed_object:
      {
        const auto &alt = this->m_stor.as<Xpnode::S_unnamed_object>();
        // Create an object by evaluating elements recursively.
        D_object object;
        object.reserve(alt.pairs.size());
        for(const auto &pair : alt.pairs) {
          const auto result = pair.second.evaluate(ctx);
          auto value = result.read();
          object.insert_or_assign(pair.first, std::move(value));
        }
        Reference_root::S_temporary ref_c = { std::move(object) };
        stack_inout.emplace_back(std::move(ref_c));
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown expression node type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

}
