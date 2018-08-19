// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "xpnode.hpp"
#include "context.hpp"
#include "function_base.hpp"
#include "utilities.hpp"

namespace Asteria
{

Xpnode::Xpnode(Xpnode &&) noexcept = default;
Xpnode & Xpnode::operator=(Xpnode &&) noexcept = default;
Xpnode::~Xpnode() = default;

const char * get_operator_name(Xpnode::Xop xop) noexcept
  {
    switch(xop)
      {
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

namespace
  {
    std::pair<Sptr<Context>, Reference *> do_name_lookup(Spref<Context> ctx, const String &name)
      {
        auto qctx = ctx;
      loop:
        auto qref = qctx->get_named_reference_opt(name);
        if(!qref) {
          qctx = qctx->get_parent_opt();
          if(!qctx) {
            ASTERIA_THROW_RUNTIME_ERROR("The identifier `", name, "` has not been declared.");
          }
          goto loop;
        }
        return std::make_pair(std::move(qctx), qref);
      }
  }

Xpnode bind_xpnode_partial(const Xpnode &node, Spref<Context> ctx)
  {
    switch(node.which()) {
    case Xpnode::index_literal:
      {
        const auto &cand = node.as<Xpnode::S_literal>();
        // Copy it as-is.
        return cand;
      }
    case Xpnode::index_named_reference:
      {
        const auto &cand = node.as<Xpnode::S_named_reference>();
        // Look for the reference in the current context.
        const auto pair = do_name_lookup(ctx, cand.name);
        if(pair.first->is_feigned()) {
          // Don't bind it onto something in a feigned context.
          return cand;
        }
        // Bind it.
        const auto qref = pair.second;
        Xpnode::S_bound_reference cand_bnd = { *qref };
        return std::move(cand_bnd);
      }
    case Xpnode::index_bound_reference:
      {
        const auto &cand = node.as<Xpnode::S_bound_reference>();
        // Copy it as-is.
        return cand;
      }
    case Xpnode::index_subexpression:
      {
        const auto &cand = node.as<Xpnode::S_subexpression>();
        // Bind the subexpression recursively.
        auto expr_bnd = bind_expression(cand.expr, ctx);
        Xpnode::S_subexpression cand_bnd = { std::move(expr_bnd) };
        return std::move(cand_bnd);
      }
    case Xpnode::index_closure_function:
      {
        const auto &cand = node.as<Xpnode::S_closure_function>();
        // TODO
        std::terminate();
        (void)cand;
      }
    case Xpnode::index_branch:
      {
        const auto &cand = node.as<Xpnode::S_branch>();
        // Bind both branches recursively.
        auto branch_true_bnd = bind_expression(cand.branch_true, ctx);
        auto branch_false_bnd = bind_expression(cand.branch_false, ctx);
        Xpnode::S_branch cand_bnd = { std::move(branch_true_bnd), std::move(branch_false_bnd) };
        return std::move(cand_bnd);
      }
    case Xpnode::index_function_call:
      {
        const auto &cand = node.as<Xpnode::S_function_call>();
        // Copy it as-is.
        return cand;
      }
    case Xpnode::index_operator_rpn:
      {
        const auto &cand = node.as<Xpnode::S_operator_rpn>();
        // Copy it as-is.
        return cand;
      }
    case Xpnode::index_unnamed_array:
      {
        const auto &cand = node.as<Xpnode::S_unnamed_array>();
        // Bind everything recursively.
        Vector<Vector<Xpnode>> elems_bnd;
        elems_bnd.reserve(cand.elems.size());
        for(const auto &elem : cand.elems) {
          auto elem_bnd = bind_expression(elem, ctx);
          elems_bnd.emplace_back(std::move(elem_bnd));
        }
        Xpnode::S_unnamed_array cand_bnd = { std::move(elems_bnd) };
        return std::move(cand_bnd);
      }
    case Xpnode::index_unnamed_object:
      {
        const auto &cand = node.as<Xpnode::S_unnamed_object>();
        // Bind everything recursively.
        Dictionary<Vector<Xpnode>> pairs_bnd;
        pairs_bnd.reserve(cand.pairs.size());
        for(const auto &pair : cand.pairs) {
          auto second_bnd = bind_expression(pair.second, ctx);
          pairs_bnd.insert_or_assign(pair.first, std::move(second_bnd));
        }
        Xpnode::S_unnamed_object cand_bnd = { std::move(pairs_bnd) };
        return std::move(cand_bnd);
      }
    default:
      ASTERIA_TERMINATE("An unknown expression node type enumeration `", node.which(), "` has been encountered.");
    }
  }

namespace
  {
    Reference do_pop_reference(Vector<Reference> &stack_inout)
      {
        if(stack_inout.empty()) {
          ASTERIA_THROW_RUNTIME_ERROR("The evaluation stack is empty, which means the expression is probably invalid.");
        }
        auto ref = std::move(stack_inout.mut_back());
        stack_inout.pop_back();
        return ref;
      }
    void do_set_result(Reference &ref_inout, bool compound_assign, Value &&value)
      {
        if(compound_assign) {
          // Write the value through `ref_inout`, which is unchanged.
          write_reference(ref_inout, std::move(value));
          return;
        }
        // Create an rvalue reference and assign it to `ref_inout`.
        Reference_root::S_temporary_value ref_c = { std::move(value) };
        ref_inout.set_root(std::move(ref_c));
      }

    // `boolean` operations
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

    // `integer` operations
    D_integer do_negate(D_integer rhs)
      {
        using limits = std::numeric_limits<D_integer>;
        if(rhs == limits::min()) {
            ASTERIA_THROW_RUNTIME_ERROR("Integral negation of `", rhs, "` would result in overflow.");
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
        auto reg = static_cast<Unsigned>(lhs);
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
        auto reg = static_cast<Unsigned>(lhs);
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
        auto reg = static_cast<Unsigned>(lhs);
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
        auto reg = static_cast<Unsigned>(lhs);
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

    // `double` operations
    D_double do_negate(D_double rhs)
      {
        return -rhs;
      }
    D_double do_add(D_double lhs, D_double rhs)
      {
        return lhs + rhs;
      }
    D_double do_subtract(D_double lhs, D_double rhs)
      {
        return lhs - rhs;
      }
    D_double do_multiply(D_double lhs, D_double rhs)
      {
        return lhs * rhs;
      }
    D_double do_divide(D_double lhs, D_double rhs)
      {
        return lhs / rhs;
      }
    D_double do_modulo(D_double lhs, D_double rhs)
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
        if(rhs == 0) {
            return res;
        }
        const auto count = static_cast<Unsigned>(rhs);
        if(lhs.size() > res.max_size() / count) {
            ASTERIA_THROW_RUNTIME_ERROR("Duplication of `", lhs, "` up to `", rhs, "` times would result in an overlong string that cannot be allocated.");
        }
        res.reserve(lhs.size() * static_cast<std::size_t>(count));
        auto mask = static_cast<Unsigned>(1) << 63;
        for(;;) {
            if(count & mask) {
                res.append(lhs);
            }
            mask >>= 1;
            if(mask == 0) {
                break;
            }
            res.append(res);
        }
        return res;
      }
  }

void evaluate_xpnode_partial(Vector<Reference> &stack_inout, const Xpnode &node, Spref<Context> ctx)
  {
    switch(node.which()) {
    case Xpnode::index_literal:
      {
        const auto &cand = node.as<Xpnode::S_literal>();
        // Push the constant.
        Reference_root::S_constant ref_c = { cand.value };
        stack_inout.emplace_back(std::move(ref_c));
        return;
      }
    case Xpnode::index_named_reference:
      {
        const auto &cand = node.as<Xpnode::S_named_reference>();
        // Look for the reference in the current context.
        const auto pair = do_name_lookup(ctx, cand.name);
        if(pair.first->is_feigned()) {
          ASTERIA_THROW_RUNTIME_ERROR("Expressions cannot be evaluated in feigned contexts.");
        }
        // Push the reference found.
        const auto qref = pair.second;
        stack_inout.emplace_back(*qref);
        return;
      }
    case Xpnode::index_bound_reference:
      {
        const auto &cand = node.as<Xpnode::S_bound_reference>();
        // Push the reference stored.
        stack_inout.emplace_back(cand.ref);
        return;
      }
    case Xpnode::index_subexpression:
      {
        const auto &cand = node.as<Xpnode::S_subexpression>();
        // Evaluate the subexpression recursively.
        auto ref = evaluate_expression(cand.expr, ctx);
        stack_inout.emplace_back(std::move(ref));
        return;
      }
    case Xpnode::index_closure_function:
      {
        const auto &cand = node.as<Xpnode::S_closure_function>();
        // TODO
        std::terminate();
        (void)cand;
      }
    case Xpnode::index_branch:
      {
        const auto &cand = node.as<Xpnode::S_branch>();
        // Pop the condition off the stack.
        auto cond = do_pop_reference(stack_inout);
        auto cond_value = read_reference(cond);
        const auto branch = test_value(cond_value) ? &(cand.branch_true) : &(cand.branch_false);
        if(branch->empty()) {
          // If the branch taken is empty, push the condition instead.
          stack_inout.emplace_back(std::move(cond));
          return;
        }
        // Evaluate the branch and push the result;
        auto result = evaluate_expression(*branch, ctx);
        stack_inout.emplace_back(std::move(result));
        return;
      }
    case Xpnode::index_function_call:
      {
        const auto &cand = node.as<Xpnode::S_function_call>();
        // Pop the callee off the stack.
        auto callee = do_pop_reference(stack_inout);
        auto callee_value = read_reference(callee);
        // Make sure it is really a function.
        const auto func = callee_value.opt<D_function>();
        if(!func) {
          ASTERIA_THROW_RUNTIME_ERROR("`", callee_value, "` is not a function and cannot be called.");
        }
        // Allocate the argument vector.
        Vector<Reference> args;
        args.reserve(cand.arg_cnt);
        for(auto i = cand.arg_cnt; i != 0; --i) {
          auto arg = do_pop_reference(stack_inout);
          args.emplace_back(std::move(arg));
        }
        // Get the `this` reference.
        auto self = callee;
        if(self.has_modifiers()) {
          // This is a member function.
          self.pop_modifier();
        } else {
          // This is a non-member function.
          Reference_root::S_constant ref_c = { D_null() };
          self.set_root(std::move(ref_c));
        }
        // Call the function and push the result as is.
        auto result = (*func)->invoke(std::move(self), std::move(args));
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
        const auto &cand = node.as<Xpnode::S_operator_rpn>();
        switch(cand.xop) {
        case Xpnode::xop_postfix_inc:
          {
            // Increment the operand and return the old value.
            // `compound_assign` is ignored.
            auto lhs_value = read_reference(lhs);
            if(lhs_value.which() == Value::type_integer)
              {
                auto result = lhs_value.as<D_integer>();
                do_set_result(lhs, true, do_add(result, D_integer(1)));
                do_set_result(lhs, false, result);
              }
            else if(lhs_value.which() == Value::type_double)
              {
                auto result = lhs_value.as<D_double>();
                do_set_result(lhs, true, do_add(result, D_double(1)));
                do_set_result(lhs, false, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_postfix_dec:
          {
            // Decrement the operand and return the old value.
            // `compound_assign` is ignored.
            auto lhs_value = read_reference(lhs);
            if(lhs_value.which() == Value::type_integer)
              {
                auto result = lhs_value.as<D_integer>();
                do_set_result(lhs, true, do_subtract(result, D_integer(1)));
                do_set_result(lhs, false, result);
              }
            else if(lhs_value.which() == Value::type_double)
              {
                auto result = lhs_value.as<D_double>();
                do_set_result(lhs, true, do_subtract(result, D_double(1)));
                do_set_result(lhs, false, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_postfix_at:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // The subscript operand shall have type `integer` or `string`.
            // `compound_assign` is ignored.
            auto rhs_value = read_reference(rhs);
            if(rhs_value.which() == Value::type_integer)
              {
                auto index = rhs_value.as<D_integer>();
                Reference_modifier::S_array_index mod_c = { index };
                lhs.push_modifier(std::move(mod_c));
              }
            else if(rhs_value.which() == Value::type_string)
              {
                auto key = rhs_value.as<D_string>();
                Reference_modifier::S_object_key mod_c = { std::move(key) };
                lhs.push_modifier(std::move(mod_c));
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("`", rhs_value, "` is not a valid member designator.");
            }
            break;
          }
        case Xpnode::xop_prefix_pos:
          {
            // Copy the operand to create an rvalue, then return it.
            auto lhs_value = read_reference(lhs);
            if(lhs_value.which() == Value::type_integer)
              {
                auto result = lhs_value.as<D_integer>();
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if(lhs_value.which() == Value::type_string)
              {
                auto result = lhs_value.as<D_double>();
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_prefix_neg:
          {
            // Negate the operand to create an rvalue, then return it.
            auto lhs_value = read_reference(lhs);
            if(lhs_value.which() == Value::type_integer)
              {
                auto result = do_negate(lhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if(lhs_value.which() == Value::type_string)
              {
                auto result = do_negate(lhs_value.as<D_double>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_prefix_notb:
          {
            // Perform bitwise not operation on the operand to create an rvalue, then return it.
            auto lhs_value = read_reference(lhs);
            if(lhs_value.which() == Value::type_boolean)
              {
                auto result = do_logical_not(lhs_value.as<D_boolean>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if(lhs_value.which() == Value::type_integer)
              {
                auto result = do_bitwise_not(lhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_prefix_notl:
          {
            // Perform logical NOT operation on the operand to create an rvalue, then return it.
            // N.B. This is one of the few operators that work on all types.
            auto lhs_value = read_reference(lhs);
              {
                auto result = do_logical_not(test_value(lhs_value));
                do_set_result(lhs, cand.compound_assign, result);
              }
            break;
          }
        case Xpnode::xop_prefix_inc:
          {
            // Increment the operand and return it.
            // `compound_assign` is ignored.
            auto lhs_value = read_reference(lhs);
            if(lhs_value.which() == Value::type_integer)
              {
                auto result = do_add(lhs_value.as<D_integer>(), D_integer(1));
                do_set_result(lhs, true, result);
              }
            else if(lhs_value.which() == Value::type_double)
              {
                auto result = do_add(lhs_value.as<D_double>(), D_double(1));
                do_set_result(lhs, true, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_prefix_dec:
          {
            // Decrement the operand and return it.
            // `compound_assign` is ignored.
            auto lhs_value = read_reference(lhs);
            if(lhs_value.which() == Value::type_integer)
              {
                auto result = do_subtract(lhs_value.as<D_integer>(), D_integer(1));
                do_set_result(lhs, true, result);
              }
            else if(lhs_value.which() == Value::type_double)
              {
                auto result = do_subtract(lhs_value.as<D_double>(), D_double(1));
                do_set_result(lhs, true, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_cmp_eq:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Report unordered operands as being unequal.
            // N.B. This is one of the few operators that work on all types.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
              {
                auto result = compare_values(lhs_value, rhs_value);
                do_set_result(lhs, false, result == Value::comparison_equal);
              }
            break;
          }
        case Xpnode::xop_infix_cmp_ne:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Report unordered operands as being unequal.
            // N.B. This is one of the few operators that work on all types.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
              {
                auto result = compare_values(lhs_value, rhs_value);
                do_set_result(lhs, false, result != Value::comparison_equal);
              }
            break;
          }
        case Xpnode::xop_infix_cmp_lt:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Throw an exception in case of unordered operands.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
              {
                auto result = compare_values(lhs_value, rhs_value);
                if(result == Value::comparison_unordered) {
                  ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_value, "` and `", rhs_value, "` are uncomparable.");
                }
                do_set_result(lhs, false, result == Value::comparison_less);
              }
            break;
          }
        case Xpnode::xop_infix_cmp_gt:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Throw an exception in case of unordered operands.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
              {
                auto result = compare_values(lhs_value, rhs_value);
                if(result == Value::comparison_unordered) {
                  ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_value, "` and `", rhs_value, "` are uncomparable.");
                }
                do_set_result(lhs, false, result == Value::comparison_greater);
              }
            break;
          }
        case Xpnode::xop_infix_cmp_lte:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Throw an exception in case of unordered operands.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
              {
                auto result = compare_values(lhs_value, rhs_value);
                if(result == Value::comparison_unordered) {
                  ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_value, "` and `", rhs_value, "` are uncomparable.");
                }
                do_set_result(lhs, false, result != Value::comparison_greater);
              }
            break;
          }
        case Xpnode::xop_infix_cmp_gte:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Throw an exception in case of unordered operands.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
              {
                auto result = compare_values(lhs_value, rhs_value);
                if(result == Value::comparison_unordered) {
                  ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs_value, "` and `", rhs_value, "` are uncomparable.");
                }
                do_set_result(lhs, false, result != Value::comparison_less);
              }
            break;
          }
        case Xpnode::xop_infix_add:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the `boolean` type, return the logical OR'd result of both operands.
            // For the `integer` and `double` types, return the sum of both operands.
            // For the `string` type, concatenate the operands in lexical order to create a new string, then return it.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
            if((lhs_value.which() == Value::type_boolean) && (rhs_value.which() == Value::type_boolean))
              {
                auto result = do_logical_or(lhs_value.as<D_boolean>(), rhs_value.as<D_boolean>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_add(lhs_value.as<D_integer>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_double) && (rhs_value.which() == Value::type_double))
              {
                auto result = do_add(lhs_value.as<D_double>(), rhs_value.as<D_double>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_string) && (rhs_value.which() == Value::type_string))
              {
                auto result = do_concatenate(lhs_value.as<D_string>(), rhs_value.as<D_string>());
                do_set_result(lhs, cand.compound_assign, std::move(result));
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_sub:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the `boolean` type, return the logical XOR'd result of both operands.
            // For the `integer` and `double` types, return the difference of both operands.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
            if((lhs_value.which() == Value::type_boolean) && (rhs_value.which() == Value::type_boolean))
              {
                auto result = do_logical_xor(lhs_value.as<D_boolean>(), rhs_value.as<D_boolean>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_subtract(lhs_value.as<D_integer>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_double) && (rhs_value.which() == Value::type_double))
              {
                auto result = do_subtract(lhs_value.as<D_double>(), rhs_value.as<D_double>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_mul:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the boolean type, return the logical AND'd result of both operands.
            // For the integer and double types, return the product of both operands.
            // If either operand has the integer type and the other has the string type, duplicate the string up to the specified number of times.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
            if((lhs_value.which() == Value::type_boolean) && (rhs_value.which() == Value::type_boolean))
              {
                auto result = do_logical_and(lhs_value.as<D_boolean>(), rhs_value.as<D_boolean>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_multiply(lhs_value.as<D_integer>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_double) && (rhs_value.which() == Value::type_double))
              {
                auto result = do_multiply(lhs_value.as<D_double>(), rhs_value.as<D_double>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_string) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_duplicate(lhs_value.as<D_string>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, std::move(result));
              }
            else if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_string))
              {
                auto result = do_duplicate(rhs_value.as<D_string>(), lhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, std::move(result));
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_div:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the integer and double types, return the quotient of both operands.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
            if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_divide(lhs_value.as<D_integer>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_double) && (rhs_value.which() == Value::type_double))
              {
                auto result = do_divide(lhs_value.as<D_double>(), rhs_value.as<D_double>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_mod:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the integer and double types, return the reminder of both operands.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
            if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_modulo(lhs_value.as<D_integer>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_double) && (rhs_value.which() == Value::type_double))
              {
                auto result = do_modulo(lhs_value.as<D_double>(), rhs_value.as<D_double>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_sll:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Shift the first operand to the left by the number of bits specified by the second operand
            // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
            // Both operands have to be integers.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
            if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_shift_left_logical(lhs_value.as<D_integer>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_srl:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Shift the first operand to the right by the number of bits specified by the second operand
            // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
            // Both operands have to be integers.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
            if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_shift_right_logical(lhs_value.as<D_integer>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_sla:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Shift the first operand to the left by the number of bits specified by the second operand
            // Bits shifted out that equal the sign bit are dicarded. Bits shifted in are filled with zeroes.
            // If a bit unequal to the sign bit would be shifted into or across the sign bit, an exception is thrown.
            // Both operands have to be integers.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
            if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_shift_left_arithmetic(lhs_value.as<D_integer>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_sra:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Shift the first operand to the right by the number of bits specified by the second operand
            // Bits shifted out are discarded. Bits shifted in are filled with the sign bit.
            // Both operands have to be integers.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
            if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_shift_right_arithmetic(lhs_value.as<D_integer>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_andb:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the `boolean` type, return the logical AND'd result of both operands.
            // For the `integer` type, return the bitwise AND'd result of both operands.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
            if((lhs_value.which() == Value::type_boolean) && (rhs_value.which() == Value::type_boolean))
              {
                auto result = do_logical_and(lhs_value.as<D_boolean>(), rhs_value.as<D_boolean>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_bitwise_and(lhs_value.as<D_integer>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_orb:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the `boolean` type, return the logical OR'd result of both operands.
            // For the `integer` type, return the bitwise OR'd result of both operands.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
            if((lhs_value.which() == Value::type_boolean) && (rhs_value.which() == Value::type_boolean))
              {
                auto result = do_logical_or(lhs_value.as<D_boolean>(), rhs_value.as<D_boolean>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_bitwise_or(lhs_value.as<D_integer>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_xorb:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // For the `boolean` type, return the logical XOR'd result of both operands.
            // For the `integer` type, return the bitwise XOR'd result of both operands.
            auto lhs_value = read_reference(lhs);
            auto rhs_value = read_reference(rhs);
            if((lhs_value.which() == Value::type_boolean) && (rhs_value.which() == Value::type_boolean))
              {
                auto result = do_logical_xor(lhs_value.as<D_boolean>(), rhs_value.as<D_boolean>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else if((lhs_value.which() == Value::type_integer) && (rhs_value.which() == Value::type_integer))
              {
                auto result = do_bitwise_xor(lhs_value.as<D_integer>(), rhs_value.as<D_integer>());
                do_set_result(lhs, cand.compound_assign, result);
              }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The ", get_operator_name(cand.xop), " operation is not defined for `", lhs_value, "` and `", rhs_value, "`.");
            }
            break;
          }
        case Xpnode::xop_infix_assign:
          {
            // Pop the second operand off the stack.
            auto rhs = do_pop_reference(stack_inout);
            // Copy the operand referenced by `rhs` to `lhs`.
            // `compound_assign` is ignored.
            // N.B. This is one of the few operators that work on all types.
            auto rhs_value = read_reference(rhs);
            do_set_result(lhs, true, std::move(rhs_value));
            break;
          }
        default:
          ASTERIA_TERMINATE("An unknown operator type enumeration `", cand.xop, "` has been encountered.");
        }
        stack_inout.emplace_back(std::move(lhs));
        return;
      }
    case Xpnode::index_unnamed_array:
      {
        const auto &cand = node.as<Xpnode::S_unnamed_array>();
        // Create an array by evaluating elements recursively.
        D_array array;
        array.reserve(cand.elems.size());
        for(const auto &elem : cand.elems) {
          const auto result = evaluate_expression(elem, ctx);
          auto value = read_reference(result);
          array.emplace_back(std::move(value));
        }
        // The result is a temporary value.
        Reference_root::S_temporary_value ref_c = { std::move(array) };
        stack_inout.emplace_back(std::move(ref_c));
        return;
      }
    case Xpnode::index_unnamed_object:
      {
        const auto &cand = node.as<Xpnode::S_unnamed_object>();
        // Create an object by evaluating elements recursively.
        D_object object;
        object.reserve(cand.pairs.size());
        for(const auto &pair : cand.pairs) {
          const auto result = evaluate_expression(pair.second, ctx);
          auto value = read_reference(result);
          object.insert_or_assign(pair.first, std::move(value));
        }
        // The result is a temporary value.
        Reference_root::S_temporary_value ref_c = { std::move(object) };
        stack_inout.emplace_back(std::move(ref_c));
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown expression node type enumeration `", node.which(), "` has been encountered.");
    }
  }

Vector<Xpnode> bind_expression(const Vector<Xpnode> &expr, Spref<Context> ctx)
  {
    Vector<Xpnode> expr_bnd;
    expr_bnd.reserve(expr.size());
    for(const auto &node : expr) {
      auto node_bnd = bind_xpnode_partial(node, ctx);
      expr_bnd.emplace_back(std::move(node_bnd));
    }
    return expr_bnd;
  }
Reference evaluate_expression(const Vector<Xpnode> &expr, Spref<Context> ctx)
  {
    if(expr.empty()) {
      return { };
    }
    Vector<Reference> stack;
    for(const auto &node : expr) {
      evaluate_xpnode_partial(stack, node, ctx);
    }
    if(stack.size() != 1) {
      ASTERIA_THROW_RUNTIME_ERROR("The expression is unbalanced.");
    }
    return std::move(stack.mut_front());
  }

}
