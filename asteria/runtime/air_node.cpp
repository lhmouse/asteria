// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "air_node.hpp"
#include "enums.hpp"
#include "executive_context.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "analytic_context.hpp"
#include "garbage_collector.hpp"
#include "random_engine.hpp"
#include "runtime_error.hpp"
#include "variable.hpp"
#include "ptc_arguments.hpp"
#include "module_loader.hpp"
#include "air_optimizer.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/statement_sequence.hpp"
#include "../compiler/statement.hpp"
#include "../compiler/expression_unit.hpp"
#include "../llds/avm_rod.hpp"
#include "../utils.hpp"
namespace asteria {
namespace {

void
do_set_rebound(bool& dirty, AIR_Node& res, AIR_Node&& bound)
  {
    dirty = true;
    res = move(bound);
  }

void
do_rebind_nodes(bool& dirty, cow_vector<AIR_Node>& code, Abstract_Context& ctx)
  {
    for(size_t i = 0;  i < code.size();  ++i)
      if(auto qnode = code.at(i).rebind_opt(ctx))
        do_set_rebound(dirty, code.mut(i), move(*qnode));
  }

template<typename xNode>
opt<AIR_Node>
do_return_rebound_opt(bool dirty, xNode&& bound)
  {
    opt<AIR_Node> res;
    if(dirty)
      res.emplace(forward<xNode>(bound));
    return res;
  }

void
do_collect_variables_for_each(Variable_HashMap& staged, Variable_HashMap& temp,
                              const cow_vector<AIR_Node>& code)
  {
    for(size_t i = 0;  i < code.size();  ++i)
      code.at(i).collect_variables(staged, temp);
  }

void
do_solidify_nodes(AVM_Rod& rod, const cow_vector<AIR_Node>& code)
  {
    rod.clear();
    for(size_t i = 0;  i < code.size();  ++i)
      code.at(i).solidify(rod);
    rod.finalize();
  }

template<typename xModifier>
void
do_push_modifier_and_check(Reference& ref, xModifier&& xmod)
  {
    ref.push_modifier(forward<xModifier>(xmod));
    ref.dereference_readonly();
  }

using Uparam  = AVM_Rod::Uparam;
using Header  = AVM_Rod::Header;

template<typename xSparam>
void
do_sparam_ctor(Header* head, void* arg)
  {
    ::rocket::details_variant::wrapped_move_construct<xSparam>(head->sparam, arg);
  }

template<typename xSparam>
void
do_sparam_dtor(Header* head)
  {
    ::rocket::details_variant::wrapped_destroy<xSparam>(head->sparam);
  }

AIR_Status
do_execute_block(const AVM_Rod& rod, const Executive_Context& ctx)
  {
    Executive_Context ctx_next(xtc_plain, ctx);
    AIR_Status status;
    try {
      status = rod.execute(ctx_next);
    }
    catch(Runtime_Error& except) {
      ctx_next.on_scope_exit_exceptional(except);
      throw;
    }
    ctx_next.on_scope_exit_normal(status);
    return status;
  }

AIR_Status
do_evaluate_subexpression(Executive_Context& ctx, bool assign, const AVM_Rod& rod)
  {
    if(rod.empty()) {
      // If the rod is empty, leave the condition on the top of the stack.
      return air_status_next;
    }
    else if(assign) {
      // Evaluate the subexpression and assign the result to the first operand.
      // The result value has to be copied, in case that a reference to an element
      // of the LHS operand is returned.
      rod.execute(ctx);
      auto& val = ctx.stack().mut_top().dereference_copy();
      ctx.stack().pop();
      ctx.stack().top().dereference_mutable() = move(val);
      return air_status_next;
    }
    else {
      // Discard the top which will be overwritten anyway, then evaluate the
      // subexpression. The status code must be forwarded, as PTCs may return
      // `air_status_return_ref`.
      ctx.stack().pop();
      return rod.execute(ctx);
    }
  }

AIR_Status
do_invoke_partial(Reference& self, Executive_Context& ctx, const Source_Location& sloc,
                  PTC_Aware ptc, const cow_function& target)
  {
    ctx.stack().clear_red_zone();
    ctx.global().call_hook(&Abstract_Hooks::on_trap, sloc, ctx);

    if(ROCKET_EXPECT(ptc == ptc_aware_none)) {
      // Perform a plain call.
      ctx.global().call_hook(&Abstract_Hooks::on_call, sloc, target);
      target.invoke(self, ctx.global(), move(ctx.alt_stack()));
      return air_status_next;
    }
    else {
      // Perform a tail call.
      self.set_ptc(::rocket::make_refcnt<PTC_Arguments>(sloc, ptc, target,
                                              move(self), move(ctx.alt_stack())));
      return air_status_return_ref;
    }
  }

template<typename xContainer>
void
do_duplicate_sequence(xContainer& src, int64_t count)
  {
    if(count < 0)
      throw Runtime_Error(xtc_format,
               "Negative duplication count (value was `$2`)", count);

    if(src.empty() || (count == 1))
      return;

    if(count == 0) {
      src.clear();
      return;
    }

    // Calculate the result length with overflow checking.
    int64_t rlen;
    if(ROCKET_MUL_OVERFLOW((int64_t) src.size(), count, &rlen) || (rlen > PTRDIFF_MAX))
      throw Runtime_Error(xtc_format,
               "Data length overflow (`$1` * `$2` > `$3`)",
               src.size(), count, PTRDIFF_MAX);

    // Duplicate elements, using binary exponential backoff.
    src.reserve((size_t) rlen);
    while(src.ssize() < rlen)
      src.append(src.begin(), src.begin() + ::rocket::min(rlen - src.ssize(), src.ssize()));
  }

AIR_Status
do_apply_binary_operator_with_integer(uint32_t uxop, Value& lhs, V_integer irhs)
  {
    switch(uxop)
      {
      case xop_cmp_eq:
        {
          // Check whether the two operands are equal. Unordered values are
          // considered to be unequal.
          lhs = lhs.compare_numeric_partial(irhs) == compare_equal;
          return air_status_next;
        }

      case xop_cmp_ne:
        {
          // Check whether the two operands are not equal. Unordered values are
          // considered to be unequal.
          lhs = lhs.compare_numeric_partial(irhs) != compare_equal;
          return air_status_next;
        }

      case xop_cmp_un:
        {
          // Check whether the two operands are unordered.
          lhs = lhs.compare_numeric_partial(irhs) == compare_unordered;
          return air_status_next;
        }

      case xop_cmp_lt:
        {
          // Check whether the LHS operand is less than the RHS operand. If
          // they are unordered, an exception shall be thrown.
          lhs = lhs.compare_numeric_total(irhs) == compare_less;
          return air_status_next;
        }

      case xop_cmp_gt:
        {
          // Check whether the LHS operand is greater than the RHS operand. If
          // they are unordered, an exception shall be thrown.
          lhs = lhs.compare_numeric_total(irhs) == compare_greater;
          return air_status_next;
        }

      case xop_cmp_lte:
        {
          // Check whether the LHS operand is less than or equal to the RHS
          // operand. If they are unordered, an exception shall be thrown.
          lhs = lhs.compare_numeric_total(irhs) != compare_greater;
          return air_status_next;
        }

      case xop_cmp_gte:
        {
          // Check whether the LHS operand is greater than or equal to the RHS
          // operand. If they are unordered, an exception shall be thrown.
          lhs = lhs.compare_numeric_total(irhs) != compare_less;
          return air_status_next;
        }

      case xop_cmp_3way:
        {
          // Defines a partial ordering on all values. For unordered operands,
          // a string is returned, so `x <=> y` and `(x <=> y) <=> 0` produces
          // the same result.
          int64_t cmp = lhs.compare_numeric_partial(irhs);
          lhs = cmp - compare_equal;
          if(ROCKET_UNEXPECT(cmp == compare_unordered))
            lhs = &"[unordered]";
          return air_status_next;
        }

      case xop_add:
        {
          // Perform logical OR on two boolean values, or get the sum of two
          // arithmetic values, or concatenate two strings.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            int64_t result;
            if(ROCKET_ADD_OVERFLOW(val, other, &result))
              throw Runtime_Error(xtc_format,
                       "Integer addition overflow (operands were `$1` and `$2`)",
                       val, other);

            val = result;
            return air_status_next;
          }

          if(lhs.is_real()) {
            V_real& val = lhs.mut_real();
            V_real other = static_cast<V_real>(irhs);

            val += other;
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Addition not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_sub:
        {
          // Perform logical XOR on two boolean values, or get the difference
          // of two arithmetic values.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            // Perform arithmetic subtraction with overflow checking.
            int64_t result;
            if(ROCKET_SUB_OVERFLOW(val, other, &result))
              throw Runtime_Error(xtc_format,
                       "Integer subtraction overflow (operands were `$1` and `$2`)",
                       val, other);

            val = result;
            return air_status_next;
          }

          if(lhs.is_real()) {
            V_real& val = lhs.mut_real();
            V_real other = static_cast<V_real>(irhs);

            // Overflow will result in an infinity, so this is safe.
            val -= other;
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Subtraction not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_mul:
        {
           // Perform logical AND on two boolean values, or get the product of
           // two arithmetic values, or duplicate a string or array by a given
           // times.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            int64_t result;
            if(ROCKET_MUL_OVERFLOW(val, other, &result))
              throw Runtime_Error(xtc_format,
                       "Integer multiplication overflow (operands were `$1` and `$2`)",
                       val, other);

            val = result;
            return air_status_next;
          }

          if(lhs.is_real()) {
            V_real& val = lhs.mut_real();
            V_real other = static_cast<V_real>(irhs);

            val *= other;
            return air_status_next;
          }

          if(lhs.is_string()) {
            V_string& val = lhs.mut_string();
            V_integer count = irhs;

            do_duplicate_sequence(val, count);
            return air_status_next;
          }

          if(lhs.is_array()) {
            V_array& val = lhs.mut_array();
            V_integer count = irhs;

            do_duplicate_sequence(val, count);
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Multiplication not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_div:
        {
          // Get the quotient of two arithmetic values. If both operands are
          // integers, the result is also an integer, truncated towards zero.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            if(other == 0)
              throw Runtime_Error(xtc_format,
                       "Zero as divisor (operands were `$1` and `$2`)",
                       val, other);

            if((val == INT64_MIN) && (other == -1))
              throw Runtime_Error(xtc_format,
                       "Integer division overflow (operands were `$1` and `$2`)",
                       val, other);

            val /= other;
            return air_status_next;
          }

          if(lhs.is_real()) {
            V_real& val = lhs.mut_real();
            V_real other = static_cast<V_real>(irhs);

            val /= other;
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Division not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_mod:
        {
          // Get the remainder of two arithmetic values. The quotient is
          // truncated towards zero. If both operands are integers, the result
          // is also an integer.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            if(other == 0)
              throw Runtime_Error(xtc_format,
                       "Zero as divisor (operands were `$1` and `$2`)",
                       val, other);

            if((val == INT64_MIN) && (other == -1))
              throw Runtime_Error(xtc_format,
                       "Integer division overflow (operands were `$1` and `$2`)",
                       val, other);

            val %= other;
            return air_status_next;
          }

          if(lhs.is_real()) {
            V_real& val = lhs.mut_real();
            V_real other = static_cast<V_real>(irhs);

            val = ::std::fmod(val, other);
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Modulo not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_andb:
        {
          // Perform the bitwise AND operation on all bits of the operands. If
          // the two operands have different lengths, the result is truncated
          // to the same length as the shorter one.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            val &= other;
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Bitwise AND not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_orb:
        {
          // Perform the bitwise OR operation on all bits of the operands. If
          // the two operands have different lengths, the result is padded to
          // the same length as the longer one, with zeroes.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            val |= other;
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Bitwise OR not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_xorb:
        {
          // Perform the bitwise XOR operation on all bits of the operands. If
          // the two operands have different lengths, the result is padded to
          // the same length as the longer one, with zeroes.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            val ^= other;
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Bitwise XOR not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_addm:
        {
          // Perform modular addition on two integers.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            ROCKET_ADD_OVERFLOW(val, other, &val);
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Modular addition not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_subm:
        {
          // Perform modular subtraction on two integers.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            ROCKET_SUB_OVERFLOW(val, other, &val);
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Modular subtraction not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_mulm:
        {
          // Perform modular multiplication on two integers.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            ROCKET_MUL_OVERFLOW(val, other, &val);
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Modular multiplication not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_adds:
        {
          // Perform saturating addition on two integers.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            if(ROCKET_ADD_OVERFLOW(val, other, &val))
              val = (other >> 63) ^ INT64_MAX;
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Saturating addition not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_subs:
        {
          // Perform saturating subtraction on two integers.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;

            if(ROCKET_SUB_OVERFLOW(val, other, &val))
              val = (other >> 63) ^ INT64_MIN;
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Saturating subtraction not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_muls:
        {
          // Perform saturating multiplication on two integers.
          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();
            V_integer other = irhs;
            V_integer sign = val ^ other;

            if(ROCKET_MUL_OVERFLOW(val, other, &val))
              val = (sign >> 63) ^ INT64_MAX;
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Saturating multiplication not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_sll:
        {
          // Shift the operand to the left. Elements that get shifted out are
          // discarded. Vacuum elements are filled with default values. The
          // width of the operand is unchanged.
          if(irhs < 0)
            throw Runtime_Error(xtc_format,
                     "Negative shift count (operands were `$1` and `$2`)",
                     lhs, irhs);

          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();

            int64_t count = irhs;
            val = (int64_t) ((uint64_t) val << (count & 63));
            val &= ((count - 64) >> 63);
            return air_status_next;
          }

          if(lhs.is_string()) {
            V_string& val = lhs.mut_string();

            size_t tlen = (size_t) ::rocket::min((uint64_t) irhs, val.size());
            val.erase(0, tlen);
            val.append(tlen, '\0');
            return air_status_next;
          }

          if(lhs.is_array()) {
            V_array& val = lhs.mut_array();

            size_t tlen = (size_t) ::rocket::min((uint64_t) irhs, val.size());
            val.erase(0, tlen);
            val.append(tlen);
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Logical left shift not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_srl:
        {
          // Shift the operand to the right. Elements that get shifted out are
          // discarded. Vacuum elements are filled with default values. The
          // width of the operand is unchanged.
          if(irhs < 0)
            throw Runtime_Error(xtc_format,
                     "Negative shift count (operands were `$1` and `$2`)",
                     lhs, irhs);

          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();

            int64_t count = irhs;
            val = (int64_t) ((uint64_t) val >> (count & 63));
            val &= ((count - 64) >> 63);
            return air_status_next;
          }

          if(lhs.is_string()) {
            V_string& val = lhs.mut_string();

            size_t tlen = (size_t) ::rocket::min((uint64_t) irhs, val.size());
            val.pop_back(tlen);
            val.insert(0, tlen, '\0');
            return air_status_next;
          }

          if(lhs.is_array()) {
            V_array& val = lhs.mut_array();

            size_t tlen = (size_t) ::rocket::min((uint64_t) irhs, val.size());
            val.pop_back(tlen);
            val.insert(0, tlen);
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Logical right shift not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_sla:
        {
          // Shift the operand to the left. No element is discarded from the
          // left (for integers this means that bits which get shifted out
          // shall all be the same with the sign bit). Vacuum elements are
          // filled with default values.
          if(irhs < 0)
            throw Runtime_Error(xtc_format,
                     "Negative shift count (operands were `$1` and `$2`)",
                     lhs, irhs);

          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();

            int64_t count = ::rocket::min(irhs, 63);
            if((val != 0) && ((count != irhs)
                              || (((val >> 63) ^ val) >> (63 - count) != 0)))
              throw Runtime_Error(xtc_format,
                       "Arithmetic left shift overflow (operands were `$1` and `$2`)",
                       lhs, irhs);

            reinterpret_cast<uint64_t&>(val) <<= count;
            return air_status_next;
          }

          if(lhs.is_string()) {
            V_string& val = lhs.mut_string();

            size_t tlen = (size_t) ::rocket::min((uint64_t) irhs, val.max_size());
            val.append(tlen, '\0');
            return air_status_next;
          }

          if(lhs.is_array()) {
            V_array& val = lhs.mut_array();

            size_t tlen = (size_t) ::rocket::min((uint64_t) irhs, val.max_size());
            val.append(tlen);
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Arithmetic left shift not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      case xop_sra:
        {
          // Shift the operand to the right. Elements that get shifted out are
          // discarded. No element is filled in the left.
          if(irhs < 0)
            throw Runtime_Error(xtc_format,
                     "Negative shift count (operands were `$1` and `$2`)",
                     lhs, irhs);

          if(lhs.is_integer()) {
            V_integer& val = lhs.mut_integer();

            int64_t count = ::rocket::min(irhs, 63);
            val >>= count;
            return air_status_next;
          }

          if(lhs.is_string()) {
            V_string& val = lhs.mut_string();

            size_t tlen = (size_t) ::rocket::min((uint64_t) irhs, val.size());
            val.pop_back(tlen);
            return air_status_next;
          }

          if(lhs.is_array()) {
            V_array& val = lhs.mut_array();

            size_t tlen = (size_t) ::rocket::min((uint64_t) irhs, val.size());
            val.pop_back(tlen);
            return air_status_next;
          }

          throw Runtime_Error(xtc_format,
                   "Arithmetic right shift not applicable (operands were `$1` and `$2`)",
                   lhs, irhs);
        }

      default:
        ROCKET_UNREACHABLE();
      }
  }

}  // namespace

opt<Value>
AIR_Node::
get_constant_opt() const noexcept
  {
    switch(this->m_stor.index())
      {
      case index_push_bound_reference:
        try {
          const auto& altr = this->m_stor.as<S_push_bound_reference>();
          if(altr.ref.is_temporary())
            return altr.ref.dereference_readonly();
        }
        catch(...) { }
        return nullopt;

      case index_push_constant:
        return this->m_stor.as<S_push_constant>().val;

      default:
        return nullopt;
      }
  }

bool
AIR_Node::
is_terminator() const noexcept
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_clear_stack:
      case index_declare_variable:
      case index_initialize_variable:
      case index_switch_statement:
      case index_do_while_statement:
      case index_while_statement:
      case index_for_each_statement:
      case index_for_statement:
      case index_try_statement:
      case index_assert_statement:
      case index_check_argument:
      case index_push_global_reference:
      case index_push_local_reference:
      case index_push_bound_reference:
      case index_define_function:
      case index_push_unnamed_array:
      case index_push_unnamed_object:
      case index_apply_operator:
      case index_unpack_array:
      case index_unpack_object:
      case index_define_null_variable:
      case index_single_step_trap:
      case index_defer_expression:
      case index_import_call:
      case index_declare_reference:
      case index_initialize_reference:
      case index_catch_expression:
      case index_push_constant:
      case index_alt_clear_stack:
      case index_coalesce_expression:
      case index_member_access:
      case index_apply_operator_bi32:
      case index_return_statement_bi32:
        return false;

      case index_throw_statement:
      case index_return_statement:
        return true;

      case index_simple_status:
        return this->m_stor.as<S_simple_status>().status != air_status_next;

      case index_function_call:
        return this->m_stor.as<S_function_call>().ptc != ptc_aware_none;

      case index_variadic_call:
        return this->m_stor.as<S_variadic_call>().ptc != ptc_aware_none;

      case index_alt_function_call:
        return this->m_stor.as<S_alt_function_call>().ptc != ptc_aware_none;

      case index_execute_block:
        {
          const auto& altr = this->m_stor.as<S_execute_block>();
          return !altr.code_body.empty() && altr.code_body.back().is_terminator();
        }

      case index_if_statement:
        {
          const auto& altr = this->m_stor.as<S_if_statement>();
          return !altr.code_true.empty() && altr.code_true.back().is_terminator()
                 && !altr.code_false.empty() && altr.code_false.back().is_terminator();
        }

      case index_branch_expression:
        {
          const auto& altr = this->m_stor.as<S_branch_expression>();
          return !altr.code_true.empty() && altr.code_true.back().is_terminator()
                 && !altr.code_false.empty() && altr.code_false.back().is_terminator();
        }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
      }
  }

opt<AIR_Node>
AIR_Node::
rebind_opt(Abstract_Context& ctx) const
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_clear_stack:
      case index_declare_variable:
      case index_initialize_variable:
      case index_throw_statement:
      case index_assert_statement:
      case index_simple_status:
      case index_check_argument:
      case index_push_global_reference:
      case index_push_bound_reference:
      case index_function_call:
      case index_push_unnamed_array:
      case index_push_unnamed_object:
      case index_apply_operator:
      case index_unpack_array:
      case index_unpack_object:
      case index_define_null_variable:
      case index_single_step_trap:
      case index_variadic_call:
      case index_import_call:
      case index_declare_reference:
      case index_initialize_reference:
      case index_return_statement:
      case index_push_constant:
      case index_alt_clear_stack:
      case index_alt_function_call:
      case index_member_access:
      case index_apply_operator_bi32:
      case index_return_statement_bi32:
        return nullopt;

      case index_execute_block:
        {
          const auto& altr = this->m_stor.as<S_execute_block>();

          // Rebind the body in a nested scope.
          bool dirty = false;
          auto bound = altr;

          Analytic_Context ctx_body(xtc_plain, ctx);
          do_rebind_nodes(dirty, bound.code_body, ctx_body);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_if_statement:
        {
          const auto& altr = this->m_stor.as<S_if_statement>();

          // Rebind both branches in a nested scope.
          bool dirty = false;
          auto bound = altr;

          Analytic_Context ctx_body(xtc_plain, ctx);
          do_rebind_nodes(dirty, bound.code_true, ctx_body);
          do_rebind_nodes(dirty, bound.code_false, ctx_body);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_switch_statement:
        {
          const auto& altr = this->m_stor.as<S_switch_statement>();

          // Rebind all labels and clauses.
          bool dirty = false;
          auto bound = altr;

          Analytic_Context ctx_body(xtc_plain, ctx);
          for(size_t k = 0;  k < bound.clauses.size();  ++k) {
            // Labels are to be evaluated in the same scope as the condition
            // expression, and are not parts of the body.
            for(size_t i = 0;  i < bound.clauses.at(k).code_labels.size();  ++i)
              if(auto qnode = bound.clauses.at(k).code_labels.at(i).rebind_opt(ctx))
                do_set_rebound(dirty, bound.clauses.mut(k).code_labels.mut(i), move(*qnode));

            for(size_t i = 0;  i < bound.clauses.at(k).code_body.size();  ++i)
              if(auto qnode = bound.clauses.at(k).code_body.at(i).rebind_opt(ctx_body))
                do_set_rebound(dirty, bound.clauses.mut(k).code_body.mut(i), move(*qnode));
          }

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_do_while_statement:
        {
          const auto& altr = this->m_stor.as<S_do_while_statement>();

          // Rebind the body and the condition expression.
          // The condition expression is not a part of the body.
          bool dirty = false;
          auto bound = altr;

          Analytic_Context ctx_body(xtc_plain, ctx);
          do_rebind_nodes(dirty, bound.code_body, ctx_body);

          do_rebind_nodes(dirty, bound.code_cond, ctx);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_while_statement:
        {
          const auto& altr = this->m_stor.as<S_while_statement>();

          // Rebind the condition expression and the body.
          // The condition expression is not a part of the body.
          bool dirty = false;
          auto bound = altr;

          do_rebind_nodes(dirty, bound.code_cond, ctx);

          Analytic_Context ctx_body(xtc_plain, ctx);
          do_rebind_nodes(dirty, bound.code_body, ctx_body);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_for_each_statement:
        {
          const auto& altr = this->m_stor.as<S_for_each_statement>();

          // Rebind the range initializer and the body.
          // The range key and mapped references are declared in a dedicated scope
          // where the initializer is to be evaluated. The body is to be executed
          // in an inner scope, created and destroyed for each iteration.
          bool dirty = false;
          auto bound = altr;

          Analytic_Context ctx_for(xtc_plain, ctx);
          if(!altr.name_key.empty())
            ctx_for.insert_named_reference(altr.name_key);
          ctx_for.insert_named_reference(altr.name_mapped);
          do_rebind_nodes(dirty, bound.code_init, ctx_for);

          Analytic_Context ctx_body(xtc_plain, ctx_for);
          do_rebind_nodes(dirty, bound.code_body, ctx_body);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_for_statement:
        {
          const auto& altr = this->m_stor.as<S_for_statement>();

          // Rebind the initializer, condition expression and step expression. All
          // these are declared in a dedicated scope where the initializer is to be
          // evaluated. The body is to be executed in an inner scope, created and
          // destroyed for each iteration.
          bool dirty = false;
          auto bound = altr;

          Analytic_Context ctx_for(xtc_plain, ctx);
          do_rebind_nodes(dirty, bound.code_init, ctx_for);
          do_rebind_nodes(dirty, bound.code_cond, ctx_for);
          do_rebind_nodes(dirty, bound.code_step, ctx_for);

          Analytic_Context ctx_body(xtc_plain, ctx_for);
          do_rebind_nodes(dirty, bound.code_body, ctx_body);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_try_statement:
        {
          const auto& altr = this->m_stor.as<S_try_statement>();

          // Rebind the `try` and `catch` clauses.
          bool dirty = false;
          auto bound = altr;

          Analytic_Context ctx_try(xtc_plain, ctx);
          do_rebind_nodes(dirty, bound.code_try, ctx_try);

          Analytic_Context ctx_catch(xtc_plain, ctx);
          ctx_catch.insert_named_reference(altr.name_except);
          do_rebind_nodes(dirty, bound.code_catch, ctx_catch);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_push_local_reference:
        {
          const auto& altr = this->m_stor.as<S_push_local_reference>();

          // Get the context.
          const Abstract_Context* qctx = &ctx;
          for(uint32_t k = 0;  k != altr.depth;  ++k)
            qctx = qctx->get_parent_opt();

          if(qctx->is_analytic())
            return nullopt;

          // Look for the name.
          auto qref = qctx->get_named_reference_opt(altr.name);
          if(!qref)
            return nullopt;
          else if(qref->is_invalid())
            throw Runtime_Error(xtc_format,
                     "Initialization of variable or reference `$1` bypassed", altr.name);

          // Bind this reference.
          S_push_bound_reference xnode = { *qref };
          return move(xnode);
        }

      case index_define_function:
        {
          const auto& altr = this->m_stor.as<S_define_function>();

          // Rebind the function body.
          // This is the only scenario where names in the outer scope are visible
          // to the body of a function.
          bool dirty = false;
          auto bound = altr;

          Analytic_Context ctx_func(xtc_function, &ctx, altr.params);
          do_rebind_nodes(dirty, bound.code_body, ctx_func);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_branch_expression:
        {
          const auto& altr = this->m_stor.as<S_branch_expression>();

          // Rebind both branches.
          bool dirty = false;
          auto bound = altr;

          do_rebind_nodes(dirty, bound.code_true, ctx);
          do_rebind_nodes(dirty, bound.code_false, ctx);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_defer_expression:
        {
          const auto& altr = this->m_stor.as<S_defer_expression>();

          // Rebind the expression.
          bool dirty = false;
          auto bound = altr;

          do_rebind_nodes(dirty, bound.code_body, ctx);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_catch_expression:
        {
          const auto& altr = this->m_stor.as<S_catch_expression>();

          // Rebind the expression.
          bool dirty = false;
          auto bound = altr;

          do_rebind_nodes(dirty, bound.code_body, ctx);

          return do_return_rebound_opt(dirty, move(bound));
        }

      case index_coalesce_expression:
        {
          const auto& altr = this->m_stor.as<S_coalesce_expression>();

          // Rebind the null branch.
          bool dirty = false;
          auto bound = altr;

          do_rebind_nodes(dirty, bound.code_null, ctx);

          return do_return_rebound_opt(dirty, move(bound));
        }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
      }
  }

void
AIR_Node::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_clear_stack:
      case index_declare_variable:
      case index_initialize_variable:
      case index_throw_statement:
      case index_assert_statement:
      case index_simple_status:
      case index_check_argument:
      case index_push_global_reference:
      case index_push_local_reference:
      case index_function_call:
      case index_push_unnamed_array:
      case index_push_unnamed_object:
      case index_apply_operator:
      case index_unpack_array:
      case index_unpack_object:
      case index_define_null_variable:
      case index_single_step_trap:
      case index_variadic_call:
      case index_import_call:
      case index_declare_reference:
      case index_initialize_reference:
      case index_return_statement:
      case index_alt_clear_stack:
      case index_alt_function_call:
      case index_member_access:
      case index_apply_operator_bi32:
      case index_return_statement_bi32:
        return;

      case index_execute_block:
        {
          const auto& altr = this->m_stor.as<S_execute_block>();

          // Collect variables from the body.
          do_collect_variables_for_each(staged, temp, altr.code_body);
          return;
        }

      case index_if_statement:
        {
          const auto& altr = this->m_stor.as<S_if_statement>();

          // Collect variables from both branches.
          do_collect_variables_for_each(staged, temp, altr.code_true);
          do_collect_variables_for_each(staged, temp, altr.code_false);
          return;
        }

      case index_switch_statement:
        {
          const auto& altr = this->m_stor.as<S_switch_statement>();

          // Collect variables from all labels and clauses.
          for(const auto& clause : altr.clauses)
            do_collect_variables_for_each(staged, temp, clause.code_labels);
          for(const auto& clause : altr.clauses)
            do_collect_variables_for_each(staged, temp, clause.code_body);
          return;
        }

      case index_do_while_statement:
        {
          const auto& altr = this->m_stor.as<S_do_while_statement>();

          // Collect variables from the body and the condition expression.
          do_collect_variables_for_each(staged, temp, altr.code_body);
          do_collect_variables_for_each(staged, temp, altr.code_cond);
          return;
        }

      case index_while_statement:
        {
          const auto& altr = this->m_stor.as<S_while_statement>();

          // Collect variables from the condition expression and the body.
          do_collect_variables_for_each(staged, temp, altr.code_cond);
          do_collect_variables_for_each(staged, temp, altr.code_body);
          return;
        }

      case index_for_each_statement:
        {
          const auto& altr = this->m_stor.as<S_for_each_statement>();

          // Collect variables from the range initializer and the body.
          do_collect_variables_for_each(staged, temp, altr.code_init);
          do_collect_variables_for_each(staged, temp, altr.code_body);
          return;
        }

      case index_for_statement:
        {
          const auto& altr = this->m_stor.as<S_for_statement>();

          // Collect variables from the initializer, condition expression and
          // step expression.
          do_collect_variables_for_each(staged, temp, altr.code_init);
          do_collect_variables_for_each(staged, temp, altr.code_cond);
          do_collect_variables_for_each(staged, temp, altr.code_step);
          return;
        }

      case index_try_statement:
        {
          const auto& altr = this->m_stor.as<S_try_statement>();

          // Collect variables from the `try` and `catch` clauses.
          do_collect_variables_for_each(staged, temp, altr.code_try);
          do_collect_variables_for_each(staged, temp, altr.code_catch);
          return;
        }

      case index_push_bound_reference:
        {
          const auto& altr = this->m_stor.as<S_push_bound_reference>();

          // Collect variables from the bound reference.
          altr.ref.collect_variables(staged, temp);
          return;
        }

      case index_define_function:
        {
          const auto& altr = this->m_stor.as<S_define_function>();

          // Collect variables from the function body.
          do_collect_variables_for_each(staged, temp, altr.code_body);
          return;
        }

      case index_branch_expression:
        {
          const auto& altr = this->m_stor.as<S_branch_expression>();

          // Collect variables from both branches.
          do_collect_variables_for_each(staged, temp, altr.code_true);
          do_collect_variables_for_each(staged, temp, altr.code_false);
          return;
        }

      case index_defer_expression:
        {
          const auto& altr = this->m_stor.as<S_defer_expression>();

          // Collect variables from the expression.
          do_collect_variables_for_each(staged, temp, altr.code_body);
          return;
        }

      case index_catch_expression:
        {
          const auto& altr = this->m_stor.as<S_catch_expression>();

          // Collect variables from the expression.
          do_collect_variables_for_each(staged, temp, altr.code_body);
          return;
        }

      case index_push_constant:
        {
          const auto& altr = this->m_stor.as<S_push_constant>();

          // Collect variables from the expression.
          altr.val.collect_variables(staged, temp);
          return;
        }

      case index_coalesce_expression:
        {
          const auto& altr = this->m_stor.as<S_coalesce_expression>();

          // Collect variables from the null branch.
          do_collect_variables_for_each(staged, temp, altr.code_null);
          return;
        }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
      }
  }

void
AIR_Node::
solidify(AVM_Rod& rod) const
  {
    switch(static_cast<Index>(this->m_stor.index()))
      {
      case index_clear_stack:
        {
          const auto& altr = this->m_stor.as<S_clear_stack>();

          (void) altr;

          rod.append(
            +[](Executive_Context& ctx, const Header* /*head*/)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                ctx.stack().clear();
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , nullptr
          );
          return;
        }

      case index_execute_block:
        {
          const auto& altr = this->m_stor.as<S_execute_block>();

          struct Sparam
            {
              AVM_Rod rod_body;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_body, altr.code_body);

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Execute the block on a new context. The block may contain control
                // statements, so the status shall be forwarded verbatim.
                return do_execute_block(sp.rod_body, ctx);
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_body.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_declare_variable:
        {
          const auto& altr = this->m_stor.as<S_declare_variable>();

          struct Sparam
            {
              phsh_string name;
            };

          Sparam sp2;
          sp2.name = altr.name;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                const auto& sloc = head->pv_meta->sloc;

                // Allocate a variable and inject it into the current context.
                const auto gcoll = ctx.global().garbage_collector();
                const auto var = gcoll->create_variable();
                ctx.insert_named_reference(sp.name).set_variable(var);
                ctx.global().call_hook(&Abstract_Hooks::on_declare, sloc, sp.name);

                // Push a copy of the reference onto the stack, which we will get
                // back after the initializer finishes execution.
                ctx.stack().push().set_variable(var);
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_initialize_variable:
        {
          const auto& altr = this->m_stor.as<S_initialize_variable>();

          Uparam up2;
          up2.b0 = altr.immutable;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const bool immutable = head->uparam.b0;

                // Read the value of the initializer. The initializer must not have
                // been empty for this function.
                const auto& val = ctx.stack().top().dereference_readonly();
                ctx.stack().pop();

                // Get the variable back.
                auto var = ctx.stack().top().unphase_variable_opt();
                ctx.stack().pop();
                ROCKET_ASSERT(var && !var->is_initialized());

                // Initialize it with this value.
                var->initialize(val);
                var->set_immutable(immutable);
                return air_status_next;
              }

            // Uparam
            , up2

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_if_statement:
        {
          const auto& altr = this->m_stor.as<S_if_statement>();

          Uparam up2;
          up2.b0 = altr.negative;

          struct Sparam
            {
              AVM_Rod rod_true;
              AVM_Rod rod_false;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_true, altr.code_true);
          do_solidify_nodes(sp2.rod_false, altr.code_false);

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const bool negative = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Read the condition and execute the corresponding branch as a block.
                return (ctx.stack().top().dereference_readonly().test() != negative)
                          ? do_execute_block(sp.rod_true, ctx)
                          : do_execute_block(sp.rod_false, ctx);
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_true.collect_variables(staged, temp);
                sp.rod_false.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_switch_statement:
        {
          const auto& altr = this->m_stor.as<S_switch_statement>();

          struct Sparam_switch_clause
            {
              Switch_Clause_Type type;
              Compare cmp2_lower;
              Compare cmp2_upper;
              AVM_Rod rod_labels;
              AVM_Rod rod_body;
              cow_vector<phsh_string> names_added;
            };

          using Sparam = cow_vector<Sparam_switch_clause>;
          Sparam sp2;
          sp2.reserve(altr.clauses.size());

          for(const auto& clause : altr.clauses) {
            auto& r = sp2.emplace_back();
            r.type = clause.type;
            r.cmp2_lower = clause.lower_closed ? compare_equal : compare_greater;
            r.cmp2_upper = clause.upper_closed ? compare_equal : compare_less;
            do_solidify_nodes(r.rod_labels, clause.code_labels);
            do_solidify_nodes(r.rod_body, clause.code_body);
            r.names_added = clause.names_added;
          }

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Read the value of the condition and find the target clause for it.
                // This is different from the `switch` statement in C, where `case`
                // labels must have constant operands.
                auto cond = ctx.stack().top().dereference_readonly();
                size_t target_index = SIZE_MAX;

                for(size_t k = 0;  k < sp.size();  ++k)
                  if(sp.at(k).type == switch_clause_default) {
                    target_index = k;
                  }
                  else if(sp.at(k).type == switch_clause_case) {
                    // Expect an exact match of one value.
                    AIR_Status status = sp.at(k).rod_labels.execute(ctx);
                    ROCKET_ASSERT(status == air_status_next);

                    if(cond.compare_partial(ctx.stack().top().dereference_readonly()) != compare_equal)
                      continue;

                    target_index = k;
                    break;
                  }
                  else if(sp.at(k).type == switch_clause_each) {
                    // Expect an interval of two values.
                    AIR_Status status = sp.at(k).rod_labels.execute(ctx);
                    ROCKET_ASSERT(status == air_status_next);

                    if(::rocket::is_none_of(cond.compare_partial(ctx.stack().top(1).dereference_readonly()),
                                            { compare_greater, sp.at(k).cmp2_lower }))
                      continue;

                    if(::rocket::is_none_of(cond.compare_partial(ctx.stack().top(0).dereference_readonly()),
                                            { compare_less, sp.at(k).cmp2_upper }))
                      continue;

                    target_index = k;
                    break;
                  }

                // Skip this statement if no matching clause has been found.
                if(target_index >= sp.size())
                  return air_status_next;

                // Jump to the target clause.
                Executive_Context ctx_body(xtc_plain, ctx);
                AIR_Status status = air_status_next;
                try {
                  for(size_t i = 0;  i < sp.size();  ++i)
                    if(i < target_index) {
                      // Inject bypassed names into the scope.
                      for(const auto& name : sp.at(i).names_added)
                        ctx_body.insert_named_reference(name);
                    }
                    else {
                      // Execute the body of this clause.
                      AIR_Status next_status = sp.at(i).rod_body.execute(ctx_body);
                      if(next_status != air_status_next) {
                        if(::rocket::is_none_of(next_status, { air_status_break_unspec, air_status_break_switch }))
                          status = next_status;
                        break;
                      }
                    }
                }
                catch(Runtime_Error& except) {
                  ctx_body.on_scope_exit_exceptional(except);
                  throw;
                }
                ctx_body.on_scope_exit_normal(status);
                return status;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                for(const auto& r : sp) {
                  r.rod_labels.collect_variables(staged, temp);
                  r.rod_body.collect_variables(staged, temp);
                }
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_do_while_statement:
        {
          const auto& altr = this->m_stor.as<S_do_while_statement>();

          Uparam up2;
          up2.b0 = altr.negative;

          struct Sparam
            {
              AVM_Rod rods_body;
              AVM_Rod rods_cond;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rods_body, altr.code_body);
          do_solidify_nodes(sp2.rods_cond, altr.code_cond);

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const bool negative = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // This is identical to C.
                AIR_Status status = air_status_next;
                for(;;) {
                  // Execute the body.
                  AIR_Status next_status = do_execute_block(sp.rods_body, ctx);
                  if(::rocket::is_none_of(next_status, { air_status_next, air_status_continue_unspec,
                                                         air_status_continue_while })) {
                    if(::rocket::is_none_of(next_status, { air_status_break_unspec, air_status_break_while }))
                      status = next_status;
                    break;
                  }

                  // Check the condition.
                  next_status = sp.rods_cond.execute(ctx);
                  ROCKET_ASSERT(next_status == air_status_next);
                  if(ctx.stack().top().dereference_readonly().test() == negative)
                    break;
                }
                return status;
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rods_body.collect_variables(staged, temp);
                sp.rods_cond.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_while_statement:
        {
          const auto& altr = this->m_stor.as<S_while_statement>();

          Uparam up2;
          up2.b0 = altr.negative;

          struct Sparam
            {
              AVM_Rod rods_cond;
              AVM_Rod rods_body;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rods_cond, altr.code_cond);
          do_solidify_nodes(sp2.rods_body, altr.code_body);

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const bool negative = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // This is identical to C.
                AIR_Status status = air_status_next;
                for(;;) {
                  // Check the condition.
                  AIR_Status next_status = sp.rods_cond.execute(ctx);
                  ROCKET_ASSERT(next_status == air_status_next);
                  if(ctx.stack().top().dereference_readonly().test() == negative)
                    break;

                  // Execute the body.
                  next_status = do_execute_block(sp.rods_body, ctx);
                  if(::rocket::is_none_of(next_status, { air_status_next, air_status_continue_unspec,
                                                         air_status_continue_while })) {
                    if(::rocket::is_none_of(next_status, { air_status_break_unspec, air_status_break_while }))
                      status = next_status;
                    break;
                  }
                }
                return status;
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rods_cond.collect_variables(staged, temp);
                sp.rods_body.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_for_each_statement:
        {
          const auto& altr = this->m_stor.as<S_for_each_statement>();

          struct Sparam
            {
              phsh_string name_key;
              phsh_string name_mapped;
              Source_Location sloc_init;
              AVM_Rod rod_init;
              AVM_Rod rod_body;
            };

          Sparam sp2;
          sp2.name_key = altr.name_key;
          sp2.name_mapped = altr.name_mapped;
          sp2.sloc_init = altr.sloc_init;
          do_solidify_nodes(sp2.rod_init, altr.code_init);
          do_solidify_nodes(sp2.rod_body, altr.code_body);

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // We have to create an outer context due to the fact that the key
                // and mapped references outlast every iteration.
                Executive_Context ctx_for(xtc_plain, ctx);
                AIR_Status status = air_status_next;
                try {
                  // Create key and mapped references.
                  Reference* qkey_ref = nullptr;
                  if(!sp.name_key.empty())
                    qkey_ref = &(ctx_for.insert_named_reference(sp.name_key));
                  auto& mapped_ref = ctx_for.insert_named_reference(sp.name_mapped);

                  // Evaluate the range initializer and set the range up, which isn't
                  // going to change for all loops.
                  AIR_Status next_status = sp.rod_init.execute(ctx_for);
                  ROCKET_ASSERT(next_status == air_status_next);
                  mapped_ref = move(ctx_for.stack().mut_top());

                  const auto range = mapped_ref.dereference_readonly();
                  if(range.is_array()) {
                    const auto& arr = range.as_array();
                    mapped_ref.push_modifier(Reference_Modifier::S_array_head());  // placeholder
                    for(int64_t i = 0;  i < arr.ssize();  ++i) {
                      // Set the key variable which is the subscript of the mapped
                      // element in the array.
                      if(qkey_ref) {
                        auto key_var = qkey_ref->unphase_variable_opt();
                        if(!key_var) {
                          key_var = ctx.global().garbage_collector()->create_variable();
                          qkey_ref->set_variable(key_var);
                        }
                        key_var->initialize(i);
                        key_var->set_immutable();
                      }

                      // Set the mapped reference.
                      mapped_ref.pop_modifier();
                      Reference_Modifier::S_array_index xmod = { i };
                      do_push_modifier_and_check(mapped_ref, move(xmod));

                      // Execute the loop body.
                      next_status = do_execute_block(sp.rod_body, ctx_for);
                      if(::rocket::is_none_of(next_status, { air_status_next, air_status_continue_unspec,
                                                             air_status_continue_for })) {
                        if(::rocket::is_none_of(next_status, { air_status_break_unspec, air_status_break_for }))
                          status = next_status;
                        break;
                      }
                    }
                  }
                  else if(range.is_object()) {
                    const auto& obj = range.as_object();
                    mapped_ref.push_modifier(Reference_Modifier::S_array_head());  // placeholder
                    for(auto it = obj.begin();  it != obj.end();  ++it) {
                      // Set the key variable which is the name of the mapped element
                      // in the object.
                      if(qkey_ref) {
                        auto key_var = qkey_ref->unphase_variable_opt();
                        if(!key_var) {
                          key_var = ctx.global().garbage_collector()->create_variable();
                          qkey_ref->set_variable(key_var);
                        }
                        key_var->initialize(it->first.rdstr());
                        key_var->set_immutable();
                      }

                      // Set the mapped reference.
                      mapped_ref.pop_modifier();
                      Reference_Modifier::S_object_key xmod = { it->first };
                      do_push_modifier_and_check(mapped_ref, move(xmod));

                      // Execute the loop body.
                      next_status = do_execute_block(sp.rod_body, ctx_for);
                      if(::rocket::is_none_of(next_status, { air_status_next, air_status_continue_unspec,
                                                             air_status_continue_for })) {
                        if(::rocket::is_none_of(next_status, { air_status_break_unspec, air_status_break_for }))
                          status = next_status;
                        break;
                      }
                    }
                  }
                  else if(!range.is_null())
                    throw Runtime_Error(xtc_assert, sp.sloc_init,
                              format_string("Range value not iterable (value `$1`)", range));
                }
                catch(Runtime_Error& except) {
                  ctx_for.on_scope_exit_exceptional(except);
                  throw;
                }
                ctx_for.on_scope_exit_normal(status);
                return status;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_init.collect_variables(staged, temp);
                sp.rod_body.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_for_statement:
        {
          const auto& altr = this->m_stor.as<S_for_statement>();

          struct Sparam
            {
              AVM_Rod rod_init;
              AVM_Rod rod_cond;
              AVM_Rod rod_step;
              AVM_Rod rod_body;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_init, altr.code_init);
          do_solidify_nodes(sp2.rod_cond, altr.code_cond);
          do_solidify_nodes(sp2.rod_step, altr.code_step);
          do_solidify_nodes(sp2.rod_body, altr.code_body);

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // This is the same as the `for` statement in C. We have to create
                // an outer context due to the fact that names declared in the first
                // segment outlast every iteration.
                Executive_Context ctx_for(xtc_plain, ctx);
                AIR_Status status = air_status_next;
                try {
                  // Execute the loop initializer, which shall only be a definition or
                  // an expression statement.
                  status = sp.rod_init.execute(ctx_for);
                  ROCKET_ASSERT(status == air_status_next);
                  for(;;) {
                    // Check the condition. There is a special case: If the condition
                    // is empty then the loop is infinite.
                    AIR_Status next_status = sp.rod_cond.execute(ctx_for);
                    ROCKET_ASSERT(next_status == air_status_next);
                    if(!ctx_for.stack().empty() && !ctx_for.stack().top().dereference_readonly().test())
                      break;

                    // Execute the body.
                    next_status = do_execute_block(sp.rod_body, ctx_for);
                    if(::rocket::is_none_of(next_status, { air_status_next, air_status_continue_unspec,
                                                           air_status_continue_for })) {
                      if(::rocket::is_none_of(next_status, { air_status_break_unspec, air_status_break_for }))
                        status = next_status;
                      break;
                    }

                    // Execute the increment.
                    status = sp.rod_step.execute(ctx_for);
                    ROCKET_ASSERT(status == air_status_next);
                  }
                }
                catch(Runtime_Error& except) {
                  ctx_for.on_scope_exit_exceptional(except);
                  throw;
                }
                ctx_for.on_scope_exit_normal(status);
                return status;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_init.collect_variables(staged, temp);
                sp.rod_cond.collect_variables(staged, temp);
                sp.rod_step.collect_variables(staged, temp);
                sp.rod_body.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_try_statement:
        {
          const auto& altr = this->m_stor.as<S_try_statement>();

          struct Sparam
            {
              AVM_Rod rod_try;
              Source_Location sloc_catch;
              phsh_string name_except;
              AVM_Rod rod_catch;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_try, altr.code_try);
          sp2.sloc_catch = altr.sloc_catch;
          sp2.name_except = altr.name_except;
          do_solidify_nodes(sp2.rod_catch, altr.code_catch);

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& try_sloc = head->pv_meta->sloc;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // This is almost identical to JavaScript but not to C++. Only one
                // `catch` clause is allowed.
                AIR_Status status;
                try {
                  // Execute the `try` block. If no exception is thrown, this will
                  // have little overhead.
                  status = do_execute_block(sp.rod_try, ctx);
                  if(status == air_status_return_ref)
                    ctx.stack().mut_top().check_function_result(ctx.global());
                  return status;
                }
                catch(Runtime_Error& except) {
                  // Append a frame due to exit of the `try` clause.
                  // Reuse the exception object. Don't bother allocating a new one.
                  except.push_frame_try(try_sloc);

                  // This branch must be executed inside this `catch` block.
                  // User-provided bindings may obtain the current exception using
                  // `::std::current_exception`.
                  Executive_Context ctx_catch(xtc_plain, ctx);
                  try {
                    // Set the exception reference.
                    auto& except_ref = ctx_catch.insert_named_reference(sp.name_except);
                    except_ref.set_temporary(except.value());

                    // Set backtrace frames.
                    V_array backtrace;
                    for(size_t k = 0;  k < except.count_frames();  ++k) {
                      V_object r;
                      r.try_emplace(&"frame", ::rocket::sref(describe_frame_type(except.frame(k).type)));
                      r.try_emplace(&"file", except.frame(k).sloc.file());
                      r.try_emplace(&"line", except.frame(k).sloc.line());
                      r.try_emplace(&"column", except.frame(k).sloc.column());
                      r.try_emplace(&"value", except.frame(k).value);
                      backtrace.emplace_back(move(r));
                    }
                    auto& backtrace_ref = ctx_catch.insert_named_reference(&"__backtrace");
                    backtrace_ref.set_temporary(move(backtrace));

                    // Execute the `catch` clause.
                    status = sp.rod_catch.execute(ctx_catch);
                  }
                  catch(Runtime_Error& nested) {
                    ctx_catch.on_scope_exit_exceptional(nested);
                    nested.push_frame_catch(sp.sloc_catch, except.value());
                    throw;
                  }
                  ctx_catch.on_scope_exit_normal(status);
                  return status;
                }
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_try.collect_variables(staged, temp);
                sp.rod_catch.collect_variables(staged, temp);
              }

            // Symbols
            , &(altr.sloc_try)
          );
          return;
        }

      case index_throw_statement:
        {
          const auto& altr = this->m_stor.as<S_throw_statement>();

          struct Sparam
            {
              Source_Location sloc;
            };

          Sparam sp2;
          sp2.sloc = altr.sloc;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__, __noreturn__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Read a value and throw it. The operand expression must not have
                // been empty for this function.
                auto& val = ctx.stack().mut_top().dereference_copy();
                ctx.stack().pop();

                if(val.is_null())
                  throw Runtime_Error(xtc_assert, sp.sloc, &"`null` not throwable");

                ctx.global().call_hook(&Abstract_Hooks::on_throw, sp.sloc, val);
                throw Runtime_Error(xtc_throw, val, sp.sloc);
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , nullptr
          );
          return;
        }

      case index_assert_statement:
        {
          const auto& altr = this->m_stor.as<S_assert_statement>();

          struct Sparam
            {
              Source_Location sloc;
              cow_string msg;
            };

          Sparam sp2;
          sp2.sloc = altr.sloc;
          sp2.msg = altr.msg;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Throw an exception if the assertion fails. This cannot be disabled.
                const auto& tval = ctx.stack().top().dereference_readonly();
                ctx.stack().pop();

                if(ROCKET_EXPECT(tval.test()))
                  return air_status_next;

                throw Runtime_Error(xtc_assert, sp.sloc, sp.msg);
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , nullptr
          );
          return;
        }

      case index_simple_status:
        {
          const auto& altr = this->m_stor.as<S_simple_status>();

          Uparam up2;
          up2.u0 = altr.status;

          rod.append(
            +[](Executive_Context& /*ctx*/, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                return static_cast<AIR_Status>(head->uparam.u0);
              }

            // Uparam
            , up2

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , nullptr
          );
          return;
        }

      case index_check_argument:
        {
          const auto& altr = this->m_stor.as<S_check_argument>();

          Uparam up2;
          up2.b0 = altr.by_ref;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const bool by_ref = head->uparam.b0;

                if(by_ref) {
                  // The argument is passed by reference, so check whether it is
                  // dereferenceable.
                  ctx.stack().top().dereference_readonly();
                  return air_status_next;
                }
                else {
                  // The argument is passed by copy, so convert it to a temporary.
                  ctx.stack().mut_top().dereference_copy();
                  return air_status_next;
                }
              }

            // Uparam
            , up2

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_push_global_reference:
        {
          const auto& altr = this->m_stor.as<S_push_global_reference>();

          struct Sparam
            {
              phsh_string name;
            };

          Sparam sp2;
          sp2.name = altr.name;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__, __flatten__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Look for the name in the global context.
                auto qref = ctx.global().get_named_reference_opt(sp.name);
                if(!qref)
                  throw Runtime_Error(xtc_format,
                           "Undeclared identifier `$1`", sp.name);

                if(qref->is_invalid())
                  throw Runtime_Error(xtc_format,
                           "Global reference `$1` not initialized", sp.name);

                // Push a copy of the reference onto the stack.
                ctx.stack().push() = *qref;
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_push_local_reference:
        {
          const auto& altr = this->m_stor.as<S_push_local_reference>();

          Uparam up2;
          up2.u01 = altr.depth;

          struct Sparam
            {
              phsh_string name;
            };

          Sparam sp2;
          sp2.name = altr.name;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__, __flatten__)) -> AIR_Status
              {
                const uint32_t depth = head->uparam.u01;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Locate the target context.
                const Executive_Context* qctx = &ctx;
                for(uint32_t k = 0;  k != depth;  ++k)
                  qctx = qctx->get_parent_opt();

                // Look for the name in the target context.
                auto qref = qctx->get_named_reference_opt(sp.name);
                if(!qref)
                  throw Runtime_Error(xtc_format,
                           "Undeclared identifier `$1`", sp.name);

                if(qref->is_invalid())
                  throw Runtime_Error(xtc_format,
                           "Initialization of `$1` was bypassed", sp.name);

                // Push a copy of the reference onto the stack.
                ctx.stack().push() = *qref;
                return air_status_next;
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_push_bound_reference:
        {
          const auto& altr = this->m_stor.as<S_push_bound_reference>();

          struct Sparam
            {
              Reference ref;
            };

          Sparam sp2;
          sp2.ref = altr.ref;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__, __flatten__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Push a copy of the captured reference.
                ctx.stack().push() = sp.ref;
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.ref.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_define_function:
        {
          const auto& altr = this->m_stor.as<S_define_function>();

          struct Sparam
            {
              Compiler_Options opts;
              cow_string func;
              cow_vector<phsh_string> params;
              cow_vector<AIR_Node> code_body;
            };

          Sparam sp2;
          sp2.opts = altr.opts;
          sp2.func = altr.func;
          sp2.params = altr.params;
          sp2.code_body = altr.code_body;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                const auto& sloc = head->pv_meta->sloc;

                // Instantiate the function.
                AIR_Optimizer optmz(sp.opts);
                optmz.rebind(&ctx, sp.params, sp.code_body);

                // Push the function as a temporary value.
                ctx.stack().push().set_temporary(optmz.create_function(sloc, sp.func));
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                do_collect_variables_for_each(staged, temp, sp.code_body);
              }

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_branch_expression:
        {
          const auto& altr = this->m_stor.as<S_branch_expression>();

          Uparam up2;
          up2.b0 = altr.assign;

          struct Sparam
            {
              AVM_Rod rod_true;
              AVM_Rod rod_false;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_true, altr.code_true);
          do_solidify_nodes(sp2.rod_false, altr.code_false);

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const bool assign = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Read the condition and evaluate the corresponding subexpression.
                return ctx.stack().top().dereference_readonly().test()
                         ? do_evaluate_subexpression(ctx, assign, sp.rod_true)
                         : do_evaluate_subexpression(ctx, assign, sp.rod_false);
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_true.collect_variables(staged, temp);
                sp.rod_false.collect_variables(staged, temp);
              }

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_function_call:
        {
          const auto& altr = this->m_stor.as<S_function_call>();

          Uparam up2;
          up2.u0 = altr.ptc;
          up2.u2345 = altr.nargs;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const PTC_Aware ptc = static_cast<PTC_Aware>(head->uparam.u0);
                const uint32_t nargs = head->uparam.u2345;
                const auto& sloc = head->pv_meta->sloc;
                const auto sentry = ctx.global().copy_recursion_sentry();

                // Collect arguments from left to right.
                ctx.alt_stack().clear();
                for(uint32_t k = 0;  k != nargs;  ++k)
                  ctx.alt_stack().push() = move(ctx.stack().mut_top(nargs - 1 - k));
                ctx.stack().pop(nargs);

                // Get the target function.
                const auto& target_value = ctx.stack().top().dereference_readonly();
                if(target_value.is_null())
                  throw Runtime_Error(xtc_format,
                           "Target function not found");

                if(!target_value.is_function())
                  throw Runtime_Error(xtc_format,
                           "Non-function value not invocable (target `$1`)", target_value);

                // Set the `this` reference and invoke the target function.
                auto target = target_value.as_function();
                ctx.stack().mut_top().pop_modifier();
                return do_invoke_partial(ctx.stack().mut_top(), ctx, sloc, ptc, target);
              }

            // Uparam
            , up2

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_push_unnamed_array:
        {
          const auto& altr = this->m_stor.as<S_push_unnamed_array>();

          Uparam up2;
          up2.u2345 = altr.nelems;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const uint32_t nelems = head->uparam.u2345;

                // Pop elements from the stack and fill them from right to left.
                V_array arr;
                arr.resize(nelems);
                for(auto it = arr.mut_rbegin();  it != arr.rend();  ++it) {
                  *it = ctx.stack().top().dereference_readonly();
                  ctx.stack().pop();
                }

                // Push the array as a temporary.
                ctx.stack().push().set_temporary(move(arr));
                return air_status_next;
              }

            // Uparam
            , up2

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_push_unnamed_object:
        {
          const auto& altr = this->m_stor.as<S_push_unnamed_object>();

          struct Sparam
            {
              cow_vector<phsh_string> keys;
            };

          Sparam sp2;
          sp2.keys = altr.keys;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Pop elements from the stack and set them from right to left. In case
                // of duplicate keys, the rightmost value takes precedence.
                V_object obj;
                obj.reserve(sp.keys.size());
                for(auto it = sp.keys.rbegin();  it != sp.keys.rend();  ++it) {
                  obj.try_emplace(*it, ctx.stack().top().dereference_readonly());
                  ctx.stack().pop();
                }

                // Push the object as a temporary.
                ctx.stack().push().set_temporary(move(obj));
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_apply_operator:
        {
          const auto& altr = this->m_stor.as<S_apply_operator>();

          Uparam up2;
          up2.b0 = altr.assign;
          up2.u1 = altr.xop;

          switch(altr.xop)
            {
            case xop_inc:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = top.dereference_mutable();

                    // `assign` is `true` for the postfix variant and `false` for
                    // the prefix variant.
                    if(rhs.type() == type_integer) {
                      V_integer& val = rhs.mut_integer();

                      // Increment the value with overflow checking.
                      int64_t result;
                      if(ROCKET_ADD_OVERFLOW(val, 1, &result))
                        throw Runtime_Error(xtc_format,
                                 "Integer increment overflow (operand was `$1`)", val);

                      if(assign)
                        top.set_temporary(val);

                      val = result;
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      V_real& val = rhs.mut_real();

                      // Overflow will result in an infinity, so this is safe.
                      double result = val + 1;

                      if(assign)
                        top.set_temporary(val);

                      val = result;
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Increment not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_dec:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = top.dereference_mutable();

                    // `assign` is `true` for the postfix variant and `false` for
                    // the prefix variant.
                    if(rhs.type() == type_integer) {
                      V_integer& val = rhs.mut_integer();

                      // Decrement the value with overflow checking.
                      int64_t result;
                      if(ROCKET_SUB_OVERFLOW(val, 1, &result))
                        throw Runtime_Error(xtc_format,
                                 "Integer decrement overflow (operand was `$1`)", val);

                      if(assign)
                        top.set_temporary(val);

                      val = result;
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      V_real& val = rhs.mut_real();

                      // Overflow will result in an infinity, so this is safe.
                      double result = val - 1;

                      if(assign)
                        top.set_temporary(val);

                      val = result;
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Decrement not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_unset:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* /*head*/)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    Reference& top = ctx.stack().mut_top();

                    // Unset the last element and return it as a temporary.
                    // `assign` is ignored.
                    Value val = top.dereference_unset();
                    top.set_temporary(move(val));
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_head:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* /*head*/)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    Reference& top = ctx.stack().mut_top();

                    // Push an array head modifier. `assign` is ignored.
                    Reference_Modifier::S_array_head xmod = { };
                    do_push_modifier_and_check(top, move(xmod));
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_tail:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* /*head*/)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    Reference& top = ctx.stack().mut_top();

                    // Push an array tail modifier. `assign` is ignored.
                    Reference_Modifier::S_array_tail xmod = { };
                    do_push_modifier_and_check(top, move(xmod));
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_random:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* /*head*/)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    Reference& top = ctx.stack().mut_top();

                    // Push a random subscript.
                    uint32_t seed = ctx.global().random_engine()->bump();
                    Reference_Modifier::S_array_random xmod = { seed };
                    do_push_modifier_and_check(top, move(xmod));
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_isvoid:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* /*head*/)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    Reference& top = ctx.stack().mut_top();

                    // Check whether the argument is a void reference and save
                    // the result as a temporary boolean. `assign` is ignored.
                    bool val = top.is_void();
                    top.set_temporary(val);
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_assign:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* /*head*/)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    Value& rhs = ctx.stack().mut_top().dereference_copy();
                    Reference& top = ctx.stack().mut_top(1);

                    // `assign` is ignored.
                    top.dereference_mutable() = move(rhs);
                    ctx.stack().pop();
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_index:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* /*head*/)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    Value& rhs = ctx.stack().mut_top().dereference_copy();
                    Reference& top = ctx.stack().mut_top(1);

                    // Push a subscript.
                    if(rhs.type() == type_integer) {
                      Reference_Modifier::S_array_index xmod = { rhs.as_integer() };
                      do_push_modifier_and_check(top, move(xmod));
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(rhs.type() == type_string) {
                      Reference_Modifier::S_object_key xmod = { rhs.as_string() };
                      do_push_modifier_and_check(top, move(xmod));
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Subscript value not valid (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_pos:
              // unary
              rod.append(
                +[](Executive_Context& /*ctx*/, const Header* /*head*/)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    // This operator does nothing.
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_neg:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Get the additive inverse of the operand.
                    if(rhs.type() == type_integer) {
                      V_integer& val = rhs.mut_integer();

                      int64_t result;
                      if(ROCKET_SUB_OVERFLOW(0, val, &result))
                        throw Runtime_Error(xtc_format,
                                 "Integer negation overflow (operand was `$1`)", val);

                      val = result;
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      V_real& val = rhs.mut_real();

                      int64_t bits;
                      bcopy(bits, val);
                      bits ^= INT64_MIN;

                      bcopy(val, bits);
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Arithmetic negation not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_notb:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Flip all bits (of all bytes) in the operand.
                    if(rhs.type() == type_boolean) {
                      V_boolean& val = rhs.mut_boolean();
                      val = !val;
                      return air_status_next;
                    }

                    if(rhs.type() == type_integer) {
                      V_integer& val = rhs.mut_integer();
                      val = ~val;
                      return air_status_next;
                    }

                    if(rhs.type() == type_string) {
                      V_string& val = rhs.mut_string();
                      for(auto it = val.mut_begin();  it != val.end();  ++it)
                        *it = static_cast<char>(*it ^ -1);
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Bitwise NOT not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_notl:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform the builtin boolean conversion and negate the result.
                    rhs = !rhs.test();
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_countof:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Get the number of elements in the operand.
                    if(rhs.type() == type_null) {
                      rhs = V_integer(0);
                      return air_status_next;
                    }

                    if(rhs.type() == type_string) {
                      rhs = V_integer(rhs.as_string().size());
                      return air_status_next;
                    }

                    if(rhs.type() == type_array) {
                      rhs = V_integer(rhs.as_array().size());
                      return air_status_next;
                    }

                    if(rhs.type() == type_object) {
                      rhs = V_integer(rhs.as_object().size());
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`countof` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_typeof:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Get the type of the operand as a string.
                    rhs = ::rocket::sref(describe_type(rhs.type()));
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sqrt:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Get the arithmetic square root of the operand, as a real
                    // number always.
                    if(rhs.is_real()) {
                      rhs = ::std::sqrt(rhs.as_real());
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__sqrt` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_isnan:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Checks whether the operand is a NaN. The operand must be
                    // of an arithmetic type. An integer is never a NaN.
                    if(rhs.type() == type_integer) {
                      rhs = false;
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      rhs = ::std::isnan(rhs.as_real());
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__isnan` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_isinf:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Checks whether the operand is an infinity. The operand must
                    // be of an arithmetic type. An integer is never an infinity.
                    if(rhs.type() == type_integer) {
                      rhs = false;
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      rhs = ::std::isinf(rhs.as_real());
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__isinf` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_abs:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Get the absolute value of the operand.
                    if(rhs.type() == type_integer) {
                      V_integer& val = rhs.mut_integer();

                      V_integer neg_val;
                      if(ROCKET_SUB_OVERFLOW(0, val, &neg_val))
                        throw Runtime_Error(xtc_format,
                                 "Integer negation overflow (operand was `$1`)", val);

                      val ^= (val ^ neg_val) & (val >> 63);
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      V_real& val = rhs.mut_real();

                      double result = ::std::fabs(val);

                      val = result;
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__abs` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sign:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Get the sign bit of the operand as a boolean value.
                    if(rhs.type() == type_integer) {
                      rhs = rhs.as_integer() < 0;
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      rhs = ::std::signbit(rhs.as_real());
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__sign` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_round:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Round the operand to the nearest integer of the same type.
                    if(rhs.type() == type_integer) {
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      rhs.mut_real() = ::std::round(rhs.as_real());
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__round` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_floor:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Round the operand to the nearest integer of the same type,
                    // towards negative infinity.
                    if(rhs.type() == type_integer) {
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      rhs.mut_real() = ::std::floor(rhs.as_real());
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__floor` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_ceil:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Round the operand to the nearest integer of the same type,
                    // towards positive infinity.
                    if(rhs.type() == type_integer) {
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      rhs.mut_real() = ::std::ceil(rhs.as_real());
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__ceil` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_trunc:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Truncate the operand to the nearest integer towards zero.
                    if(rhs.type() == type_integer) {
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      rhs.mut_real() = ::std::trunc(rhs.as_real());
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__trunc` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_iround:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Round the operand to the nearest integer.
                    if(rhs.type() == type_integer) {
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      rhs = safe_double_to_int64(::std::round(rhs.as_real()));
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__iround` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_ifloor:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Round the operand to the nearest integer towards negative
                    // infinity.
                    if(rhs.type() == type_integer) {
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      rhs = safe_double_to_int64(::std::floor(rhs.as_real()));
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__ifloor` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_iceil:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Round the operand to the nearest integer towards positive
                    //  infinity.
                    if(rhs.type() == type_integer) {
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      rhs = safe_double_to_int64(::std::ceil(rhs.as_real()));
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__iceil` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_itrunc:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Truncate the operand to the nearest integer towards zero.
                    if(rhs.type() == type_integer) {
                      return air_status_next;
                    }

                    if(rhs.type() == type_real) {
                      rhs = safe_double_to_int64(::std::trunc(rhs.as_real()));
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__itrunc` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_lzcnt:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Get the number of leading zeroes in the operand.
                    if(rhs.type() == type_integer) {
                      V_integer& val = rhs.mut_integer();

                      val = (int64_t) ROCKET_LZCNT64((uint64_t) val);
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__lzcnt` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_tzcnt:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Get the number of trailing zeroes in the operand.
                    if(rhs.type() == type_integer) {
                      V_integer& val = rhs.mut_integer();

                      val = (int64_t) ROCKET_TZCNT64((uint64_t) val);
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__tzcnt` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_popcnt:
              // unary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    Reference& top = ctx.stack().mut_top();
                    Value& rhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Get the number of ones in the operand.
                    if(rhs.type() == type_integer) {
                      V_integer& val = rhs.mut_integer();

                      val = (int64_t) ROCKET_POPCNT64((uint64_t) val);
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__popcnt` not applicable (operand was `$1`)", rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_eq:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const auto& rhs = ctx.stack().top().dereference_readonly();
                    auto& top = ctx.stack().mut_top(1);
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Check whether the two operands are equal. Unordered values
                    // are considered unequal.
                    lhs = lhs.compare_partial(rhs) == compare_equal;
                    ctx.stack().pop();
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_ne:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Check whether the two operands are not equal. Unordered
                    // values are considered unequal.
                    lhs = lhs.compare_partial(rhs) != compare_equal;
                    ctx.stack().pop();
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_un:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Check whether the two operands are unordered.
                    lhs = lhs.compare_partial(rhs) == compare_unordered;
                    ctx.stack().pop();
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_lt:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Check whether the LHS operand is less than the RHS operand.
                    // If they are unordered, an exception shall be thrown.
                    lhs = lhs.compare_total(rhs) == compare_less;
                    ctx.stack().pop();
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_gt:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Check whether the LHS operand is greater than the RHS operand.
                    // If they are unordered, an exception shall be thrown.
                    lhs = lhs.compare_total(rhs) == compare_greater;
                    ctx.stack().pop();
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_lte:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Check whether the LHS operand is less than or equal to the
                    // RHS operand. If they are unordered, an exception shall be
                    // thrown.
                    lhs = lhs.compare_total(rhs) != compare_greater;
                    ctx.stack().pop();
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_gte:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Check whether the LHS operand is greater than or equal to
                    // the RHS operand. If they are unordered, an exception shall
                    // be thrown.
                    lhs = lhs.compare_total(rhs) != compare_less;
                    ctx.stack().pop();
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_3way:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Defines a partial ordering on all values. For unordered
                    // operands, a string is returned, so `x <=> y` and
                    // `(x <=> y) <=> 0` produces the same result.
                    int64_t cmp = lhs.compare_partial(rhs);
                    if(ROCKET_UNEXPECT(cmp == compare_unordered))
                      lhs = &"[unordered]";
                    else
                      lhs = cmp - compare_equal;
                    ctx.stack().pop();
                    return air_status_next;
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_add:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform logical OR on two boolean values, or get the sum
                    // of two arithmetic values, or concatenate two strings.
                    if(lhs.is_integer() && rhs.is_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      int64_t result;
                      if(ROCKET_ADD_OVERFLOW(val, other, &result))
                        throw Runtime_Error(xtc_format,
                                 "Integer addition overflow (operands were `$1` and `$2`)",
                                 val, other);

                      val = result;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_real() && rhs.is_real()) {
                      V_real& val = lhs.mut_real();
                      V_real other = rhs.as_real();

                      val += other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_string() && rhs.is_string()) {
                      V_string& val = lhs.mut_string();
                      const V_string& other = rhs.as_string();

                      val.append(other);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_boolean() && rhs.is_boolean()) {
                      V_boolean& val = lhs.mut_boolean();
                      V_boolean other = rhs.as_boolean();

                      val |= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Addition not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sub:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform logical XOR on two boolean values, or get the
                    // difference of two arithmetic values.
                    if(lhs.is_integer() && rhs.is_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      // Perform arithmetic subtraction with overflow checking.
                      int64_t result;
                      if(ROCKET_SUB_OVERFLOW(val, other, &result))
                        throw Runtime_Error(xtc_format,
                                 "Integer subtraction overflow (operands were `$1` and `$2`)",
                                 val, other);

                      val = result;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_real() && rhs.is_real()) {
                      V_real& val = lhs.mut_real();
                      V_real other = rhs.as_real();

                      // Overflow will result in an infinity, so this is safe.
                      val -= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_boolean() && rhs.is_boolean()) {
                      V_boolean& val = lhs.mut_boolean();
                      V_boolean other = rhs.as_boolean();

                      // Perform logical XOR of the operands.
                      val ^= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Subtraction not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_mul:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform logical AND on two boolean values, or get the product
                    // of two arithmetic values, or duplicate a string or array by
                    // the given times.
                    if(lhs.is_integer() && rhs.is_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      int64_t result;
                      if(ROCKET_MUL_OVERFLOW(val, other, &result))
                        throw Runtime_Error(xtc_format,
                                 "Integer multiplication overflow (operands were `$1` and `$2`)",
                                 val, other);

                      val = result;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_real() && rhs.is_real()) {
                      V_real& val = lhs.mut_real();
                      V_real other = rhs.as_real();

                      val *= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_integer() && rhs.is_string()) {
                      V_integer count = lhs.as_integer();
                      lhs = rhs.as_string();
                      V_string& val = lhs.mut_string();

                      do_duplicate_sequence(val, count);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_string() && rhs.is_integer()) {
                      V_string& val = lhs.mut_string();
                      V_integer count = rhs.as_integer();

                      do_duplicate_sequence(val, count);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_integer() && rhs.is_array()) {
                      V_integer count = lhs.as_integer();
                      lhs = rhs.as_array();
                      V_array& val = lhs.mut_array();

                      do_duplicate_sequence(val, count);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_array() && rhs.is_integer()) {
                      V_array& val = lhs.mut_array();
                      V_integer count = rhs.as_integer();

                      do_duplicate_sequence(val, count);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_boolean() && rhs.is_boolean()) {
                      V_boolean& val = lhs.mut_boolean();
                      V_boolean other = rhs.as_boolean();

                      val &= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Multiplication not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_div:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Get the quotient of two arithmetic values. If both operands
                    // are integers, the result is also an integer, truncated
                    // towards zero.
                    if(lhs.is_integer() && rhs.is_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      if(other == 0)
                        throw Runtime_Error(xtc_format,
                                 "Zero as divisor (operands were `$1` and `$2`)",
                                 val, other);

                      if((val == INT64_MIN) && (other == -1))
                        throw Runtime_Error(xtc_format,
                                 "Integer division overflow (operands were `$1` and `$2`)",
                                 val, other);

                      val /= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_real() && rhs.is_real()) {
                      V_real& val = lhs.mut_real();
                      V_real other = rhs.as_real();

                      val /= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Division not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_mod:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Get the remainder of two arithmetic values. The quotient is
                    // truncated towards zero. If both operands are integers, the
                    // result is also an integer.
                    if(lhs.is_integer() && rhs.is_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      if(other == 0)
                        throw Runtime_Error(xtc_format,
                                 "Zero as divisor (operands were `$1` and `$2`)",
                                 val, other);

                      if((val == INT64_MIN) && (other == -1))
                        throw Runtime_Error(xtc_format,
                                 "Integer division overflow (operands were `$1` and `$2`)",
                                 val, other);

                      val %= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_real() && rhs.is_real()) {
                      V_real& val = lhs.mut_real();
                      V_real other = rhs.as_real();

                      val = ::std::fmod(val, other);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Modulo not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_andb:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform the bitwise AND operation on all bits of the operands.
                    // If the two operands have different lengths, the result is
                    // truncated to the same length as the shorter one.
                    if(lhs.is_integer() && rhs.is_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      val &= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_string() && rhs.is_string()) {
                      V_string& val = lhs.mut_string();
                      const V_string& mask = rhs.as_string();

                      if(val.size() > mask.size())
                        val.erase(mask.size());
                      auto maskp = mask.begin();
                      for(auto it = val.mut_begin();  it != val.end();  ++it, ++maskp)
                        *it = static_cast<char>(*it & *maskp);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_boolean() && rhs.is_boolean()) {
                      V_boolean& val = lhs.mut_boolean();
                      V_boolean other = rhs.as_boolean();

                      val &= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Bitwise AND not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_orb:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform the bitwise OR operation on all bits of the operands.
                    // If the two operands have different lengths, the result is
                    // padded to the same length as the longer one, with zeroes.
                    if(lhs.is_integer() && rhs.is_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      val |= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_string() && rhs.is_string()) {
                      V_string& val = lhs.mut_string();
                      const V_string& mask = rhs.as_string();

                      if(val.size() < mask.size())
                        val.append(mask.size() - val.size(), 0);
                      auto valp = val.mut_begin();
                      for(auto it = mask.begin();  it != mask.end();  ++it, ++valp)
                        *valp = static_cast<char>(*valp | *it);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_boolean() && rhs.is_boolean()) {
                      V_boolean& val = lhs.mut_boolean();
                      V_boolean other = rhs.as_boolean();

                      val |= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Bitwise OR not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_xorb:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform the bitwise XOR operation on all bits of the operands.
                    // If the two operands have different lengths, the result is
                    // padded to the same length as the longer one, with zeroes.
                    if(lhs.is_integer() && rhs.is_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      val ^= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_string() && rhs.is_string()) {
                      V_string& val = lhs.mut_string();
                      const V_string& mask = rhs.as_string();

                      if(val.size() < mask.size())
                        val.append(mask.size() - val.size(), 0);
                      auto valp = val.mut_begin();
                      for(auto it = mask.begin();  it != mask.end();  ++it, ++valp)
                        *valp = static_cast<char>(*valp ^ *it);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_boolean() && rhs.is_boolean()) {
                      V_boolean& val = lhs.mut_boolean();
                      V_boolean other = rhs.as_boolean();

                      val ^= other;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Bitwise XOR not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_addm:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform modular addition on two integers.
                    if(lhs.is_integer() && rhs.as_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      ROCKET_ADD_OVERFLOW(val, other, &val);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Modular addition not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_subm:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform modular subtraction on two integers.
                    if(lhs.is_integer() && rhs.as_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      ROCKET_SUB_OVERFLOW(val, other, &val);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Modular subtraction not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_mulm:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform modular multiplication on two integers.
                    if(lhs.is_integer() && rhs.as_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      ROCKET_MUL_OVERFLOW(val, other, &val);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Modular multiplication not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_adds:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform saturating addition on two integers.
                    if(lhs.is_integer() && rhs.as_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      if(ROCKET_ADD_OVERFLOW(val, other, &val))
                        val = (other >> 63) ^ INT64_MAX;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Saturating addition not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_subs:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform saturating subtraction on two integers.
                    if(lhs.is_integer() && rhs.as_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();

                      if(ROCKET_SUB_OVERFLOW(val, other, &val))
                        val = (other >> 63) ^ INT64_MIN;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Saturating subtraction not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_muls:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform saturating multiplication on two integers.
                    if(lhs.is_integer() && rhs.as_integer()) {
                      V_integer& val = lhs.mut_integer();
                      V_integer other = rhs.as_integer();
                      V_integer sign = val ^ other;

                      if(ROCKET_MUL_OVERFLOW(val, other, &val))
                        val = (sign >> 63) ^ INT64_MAX;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Saturating multiplication not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sll:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.type() != type_integer)
                      throw Runtime_Error(xtc_format,
                               "Invalid shift count (operands were `$1` and `$2`)",
                               lhs, rhs);

                    if(rhs.as_integer() < 0)
                      throw Runtime_Error(xtc_format,
                               "Negative shift count (operands were `$1` and `$2`)",
                               lhs, rhs);

                    // Shift the operand to the left. Elements that get shifted out
                    // are discarded. Vacuum elements are filled with default values.
                    // The width of the operand is unchanged.
                    if(lhs.is_integer()) {
                      V_integer& val = lhs.mut_integer();

                      int64_t count = rhs.as_integer();
                      val = (int64_t) ((uint64_t) val << (count & 63));
                      val &= ((count - 64) >> 63);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_string()) {
                      V_string& val = lhs.mut_string();

                      size_t tlen = (size_t) ::rocket::min(rhs.as_integer(), val.ssize());
                      val.erase(0, tlen);
                      val.append(tlen, '\0');
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_array()) {
                      V_array& val = lhs.mut_array();

                      size_t tlen = (size_t) ::rocket::min(rhs.as_integer(), val.ssize());
                      val.erase(0, tlen);
                      val.append(tlen);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Logical left shift not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_srl:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.type() != type_integer)
                      throw Runtime_Error(xtc_format,
                               "Invalid shift count (operands were `$1` and `$2`)",
                               lhs, rhs);

                    if(rhs.as_integer() < 0)
                      throw Runtime_Error(xtc_format,
                               "Negative shift count (operands were `$1` and `$2`)",
                               lhs, rhs);

                    // Shift the operand to the right. Elements that get shifted out
                    // are discarded. Vacuum elements are filled with default values.
                    // The width of the operand is unchanged.
                    if(lhs.is_integer()) {
                      V_integer& val = lhs.mut_integer();

                      int64_t count = rhs.as_integer();
                      val = (int64_t) ((uint64_t) val >> (count & 63));
                      val &= ((count - 64) >> 63);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_string()) {
                      V_string& val = lhs.mut_string();

                      size_t tlen = (size_t) ::rocket::min(rhs.as_integer(), val.ssize());
                      val.pop_back(tlen);
                      val.insert(0, tlen, '\0');
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_array()) {
                      V_array& val = lhs.mut_array();

                      size_t tlen = (size_t) ::rocket::min(rhs.as_integer(), val.ssize());
                      val.pop_back(tlen);
                      val.insert(0, tlen);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Logical right shift not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sla:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.type() != type_integer)
                      throw Runtime_Error(xtc_format,
                               "Invalid shift count (operands were `$1` and `$2`)",
                               lhs, rhs);

                    if(rhs.as_integer() < 0)
                      throw Runtime_Error(xtc_format,
                               "Negative shift count (operands were `$1` and `$2`)",
                               lhs, rhs);

                    // Shift the operand to the left. No element is discarded from
                    // the left (for integers this means that bits which get shifted
                    // out shall all be the same with the sign bit). Vacuum elements
                    // are filled with default values.
                    if(lhs.is_integer()) {
                      V_integer& val = lhs.mut_integer();

                      int64_t count = ::rocket::min(rhs.as_integer(), 63);
                      if((val != 0) && ((count != rhs.as_integer()) || (val >> (63 - count) != val >> 63)))
                        throw Runtime_Error(xtc_format,
                                 "Arithmetic left shift overflow (operands were `$1` and `$2`)",
                                 lhs, rhs);

                      reinterpret_cast<uint64_t&>(val) <<= count;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_string()) {
                      V_string& val = lhs.mut_string();

                      size_t tlen = (size_t) ::rocket::min(rhs.as_integer(), val.ssize());
                      val.append(tlen, '\0');
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_array()) {
                      V_array& val = lhs.mut_array();

                      size_t tlen = (size_t) ::rocket::min(rhs.as_integer(), val.ssize());
                      val.append(tlen);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Arithmetic left shift not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_sra:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const Value& rhs = ctx.stack().top().dereference_readonly();
                    Reference& top = ctx.stack().mut_top(1);
                    Value& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    if(rhs.type() != type_integer)
                      throw Runtime_Error(xtc_format,
                               "Invalid shift count (operands were `$1` and `$2`)",
                               lhs, rhs);

                    if(rhs.as_integer() < 0)
                      throw Runtime_Error(xtc_format,
                               "Negative shift count (operands were `$1` and `$2`)",
                               lhs, rhs);

                    // Shift the operand to the right. Elements that get shifted
                    // out are discarded. No element is filled in the left.
                    if(lhs.is_integer()) {
                      V_integer& val = lhs.mut_integer();

                      int64_t count = ::rocket::min(rhs.as_integer(), 63);
                      val >>= count;
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_string()) {
                      V_string& val = lhs.mut_string();

                      size_t tlen = (size_t) ::rocket::min(rhs.as_integer(), PTRDIFF_MAX);
                      val.pop_back(tlen);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    if(lhs.is_array()) {
                      V_array& val = lhs.mut_array();

                      size_t tlen = (size_t) ::rocket::min(rhs.as_integer(), PTRDIFF_MAX);
                      val.pop_back(tlen);
                      ctx.stack().pop();
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "Arithmetic right shift not applicable (operands were `$1` and `$2`)",
                             lhs, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_fma:
              // fused multiply-add; ternary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const auto& rhs = ctx.stack().top().dereference_readonly();
                    ctx.stack().pop();
                    const auto& mid = ctx.stack().top().dereference_readonly();
                    ctx.stack().pop();
                    auto& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    // Perform floating-point fused multiply-add.
                    if(lhs.is_real() && mid.is_real() && rhs.is_real()) {
                      V_real& val = lhs.mut_real();
                      V_real y_mul = mid.as_real();
                      V_real z_add = rhs.as_real();

                      val = ::std::fma(val, y_mul, z_add);
                      return air_status_next;
                    }

                    throw Runtime_Error(xtc_format,
                             "`__fma` not applicable (operands were `$1`, `$2` and `$3`)",
                             lhs, mid, rhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            default:
              ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), altr.xop);
            }
        }

      case index_unpack_array:
        {
          const auto& altr = this->m_stor.as<S_unpack_array>();

          Uparam up2;
          up2.b0 = altr.immutable;
          up2.u2345 = altr.nelems;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const bool immutable = head->uparam.b0;
                const uint32_t nelems = head->uparam.u2345;

                // Read the value of the initializer.
                const auto& init = ctx.stack().top().dereference_readonly();
                ctx.stack().pop();

                // Make sure it is really an array.
                if(!init.is_null() && !init.is_array())
                  throw Runtime_Error(xtc_format,
                           "Initializer was not an array (value was `$1`)", init);

                for(uint32_t i = nelems - 1;  i != UINT32_MAX;  --i) {
                  // Pop variables from from right to left.
                  auto var = ctx.stack().top().unphase_variable_opt();
                  ctx.stack().pop();
                  ROCKET_ASSERT(var && !var->is_initialized());

                  if(ROCKET_EXPECT(init.is_array()))
                    if(auto ielem = init.as_array().ptr(i))
                      var->initialize(*ielem);

                  if(ROCKET_UNEXPECT(!var->is_initialized()))
                    var->initialize(nullopt);

                  var->set_immutable(immutable);
                }
                return air_status_next;
              }

            // Uparam
            , up2

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_unpack_object:
        {
          const auto& altr = this->m_stor.as<S_unpack_object>();

          Uparam up2;
          up2.b0 = altr.immutable;

          struct Sparam
            {
              cow_vector<phsh_string> keys;
            };

          Sparam sp2;
          sp2.keys = altr.keys;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const bool immutable = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Read the value of the initializer.
                const auto& init = ctx.stack().top().dereference_readonly();
                ctx.stack().pop();

                // Make sure it is really an object.
                if(!init.is_null() && !init.is_object())
                  throw Runtime_Error(xtc_format,
                           "Initializer was not an object (value was `$1`)", init);

                for(auto it = sp.keys.rbegin();  it != sp.keys.rend();  ++it) {
                  // Pop variables from from right to left.
                  auto var = ctx.stack().top().unphase_variable_opt();
                  ctx.stack().pop();
                  ROCKET_ASSERT(var && !var->is_initialized());

                  if(ROCKET_EXPECT(init.is_object()))
                    if(auto ielem = init.as_object().ptr(*it))
                      var->initialize(*ielem);

                  if(ROCKET_UNEXPECT(!var->is_initialized()))
                    var->initialize(nullopt);

                  var->set_immutable(immutable);
                }
                return air_status_next;
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_define_null_variable:
        {
          const auto& altr = this->m_stor.as<S_define_null_variable>();

          Uparam up2;
          up2.b0 = altr.immutable;

          struct Sparam
            {
              phsh_string name;
            };

          Sparam sp2;
          sp2.name = altr.name;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const bool immutable = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                const auto& sloc = head->pv_meta->sloc;

                // Allocate a variable and inject it into the current context.
                const auto gcoll = ctx.global().garbage_collector();
                const auto var = gcoll->create_variable();
                ctx.insert_named_reference(sp.name).set_variable(var);
                ctx.global().call_hook(&Abstract_Hooks::on_declare, sloc, sp.name);

                // Initialize it to null.
                var->initialize(nullopt);
                var->set_immutable(immutable);
                return air_status_next;
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_single_step_trap:
        {
          const auto& altr = this->m_stor.as<S_single_step_trap>();

          (void) altr;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sloc = head->pv_meta->sloc;
                ctx.global().call_hook(&Abstract_Hooks::on_trap, sloc, ctx);
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_variadic_call:
        {
          const auto& altr = this->m_stor.as<S_variadic_call>();

          Uparam up2;
          up2.u0 = altr.ptc;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const PTC_Aware ptc = static_cast<PTC_Aware>(head->uparam.u0);
                const auto& sloc = head->pv_meta->sloc;
                const auto sentry = ctx.global().copy_recursion_sentry();

                auto temp_value = ctx.stack().top().dereference_readonly();
                if(temp_value.is_null()) {
                  // There is no argument for the target function.
                  ctx.alt_stack().clear();
                  ctx.stack().pop();
                }
                else if(temp_value.is_array()) {
                  // Push all values from left to right, as temporary values.
                  ctx.alt_stack().clear();
                  for(const auto& val : temp_value.as_array())
                    ctx.alt_stack().push().set_temporary(val);
                  ctx.stack().pop();
                }
                else if(temp_value.is_function()) {
                  // Invoke the generator function with no argument to get the number
                  // of variadic arguments. This destroys its self reference, so we have
                  // to stash it first.
                  auto va_generator = temp_value.as_function();
                  ctx.stack().mut_top().pop_modifier();
                  ctx.stack().push();
                  ctx.stack().mut_top() = ctx.stack().mut_top(1);
                  ctx.alt_stack().clear();
                  do_invoke_partial(ctx.stack().mut_top(), ctx, sloc, ptc_aware_none, va_generator);
                  temp_value = ctx.stack().top().dereference_readonly();
                  ctx.stack().pop();

                  if(temp_value.type() != type_integer)
                    throw Runtime_Error(xtc_format,
                             "Invalid number of variadic arguments (value `$1`)", temp_value);

                  if((temp_value.as_integer() < 0) || (temp_value.as_integer() > INT_MAX))
                    throw Runtime_Error(xtc_format,
                             "Invalid number of variadic arguments (value `$1`)", temp_value);

                  uint32_t nargs = static_cast<uint32_t>(temp_value.as_integer());
                  if(nargs == 0) {
                    // There is no argument for the target function.
                    ctx.alt_stack().clear();
                    ctx.stack().pop();
                  }
                  else {
                    // Initialize `this` references for variadic arguments.
                    for(uint32_t k = 0;  k != nargs - 1;  ++k) {
                      ctx.stack().push();
                      ctx.stack().mut_top() = ctx.stack().top(1);
                    }

                    // Generate varaidic arguments, and store them on `stack` from
                    // right to left for later use.
                    for(uint32_t k = 0;  k != nargs;  ++k) {
                      ctx.alt_stack().clear();
                      ctx.alt_stack().push().set_temporary(V_integer(k));
                      do_invoke_partial(ctx.stack().mut_top(k), ctx, sloc, ptc_aware_none, va_generator);
                      ctx.stack().top(k).dereference_readonly();
                    }

                    // Move arguments into `alt_stack` from left to right.
                    ctx.alt_stack().clear();
                    for(uint32_t k = 0;  k != nargs;  ++k)
                      ctx.alt_stack().push() = move(ctx.stack().mut_top(k));
                    ctx.stack().pop(nargs);
                  }
                }
                else
                  throw Runtime_Error(xtc_format,
                           "Invalid variadic argument generator (value `$1`)", temp_value);

                // Get the target function.
                temp_value = ctx.stack().top().dereference_readonly();
                if(temp_value.is_null())
                  throw Runtime_Error(xtc_format,
                           "Target function not found");

                if(!temp_value.is_function())
                  throw Runtime_Error(xtc_format,
                           "Non-function value not invocable (target `$1`)", temp_value);

                // Invoke the target function with arguments from `alt_stack`.
                const auto& target = temp_value.as_function();
                ctx.stack().mut_top().pop_modifier();
                return do_invoke_partial(ctx.stack().mut_top(), ctx, sloc, ptc, target);
              }

            // Uparam
            , up2

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_defer_expression:
        {
          const auto& altr = this->m_stor.as<S_defer_expression>();

          struct Sparam
            {
              cow_vector<AIR_Node> code_body;
            };

          Sparam sp2;
          sp2.code_body = altr.code_body;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                const auto& sloc = head->pv_meta->sloc;

                // Capture local references at this time.
                bool dirty = false;
                auto bound_body = sp.code_body;
                do_rebind_nodes(dirty, bound_body, ctx);

                // Instantiate the expression and push it to the current context.
                AVM_Rod rod_body;
                do_solidify_nodes(rod_body, bound_body);
                ctx.mut_defer().emplace_back(sloc, move(rod_body));
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                do_collect_variables_for_each(staged, temp, sp.code_body);
              }

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_import_call:
        {
          const auto& altr = this->m_stor.as<S_import_call>();

          Uparam up2;
          up2.u2345 = altr.nargs;

          struct Sparam
            {
              Compiler_Options opts;
            };

          Sparam sp2;
          sp2.opts = altr.opts;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const uint32_t nargs = head->uparam.u2345;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                const auto& sloc = head->pv_meta->sloc;
                const auto sentry = ctx.global().copy_recursion_sentry();

                // Collect arguments from left to right.
                ctx.alt_stack().clear();
                for(uint32_t k = 0;  k != nargs - 1;  ++k)
                  ctx.alt_stack().push() = move(ctx.stack().mut_top(nargs - 2 - k));
                ctx.stack().pop(nargs - 1);

                // Get the path to import.
                const auto& path_val = ctx.stack().top().dereference_readonly();
                if(path_val.type() != type_string)
                  throw Runtime_Error(xtc_format,
                           "Path was not a string (value `$1`)", path_val);

                if(path_val.as_string() == "")
                  throw Runtime_Error(xtc_format, "Path was empty");

                cow_string abs_path = path_val.as_string();
                if(abs_path[0] != '/') {
                  const auto& src_file = sloc.file();
                  if(src_file[0] == '/') {
                    size_t pos = src_file.rfind('/');
                    ROCKET_ASSERT(pos != cow_string::npos);
                    abs_path.insert(0, src_file.c_str(), pos + 1);
                  }
                }

                abs_path = get_real_path(abs_path);
                Source_Location script_sloc(abs_path, 0, 0);

                Module_Loader::Unique_Stream istrm;
                istrm.reset(ctx.global().module_loader(), abs_path);

                Token_Stream tstrm(sp.opts);
                tstrm.reload(abs_path, 1, move(istrm.get()));

                Statement_Sequence stmtq(sp.opts);
                stmtq.reload(move(tstrm));

                // Instantiate the script as a variadic function.
                AIR_Optimizer optmz(sp.opts);
                cow_vector<phsh_string> script_params;
                script_params.emplace_back(&"...");
                optmz.reload(nullptr, script_params, ctx.global(), stmtq.get_statements());

                // Invoke it without `this`.
                auto target = optmz.create_function(script_sloc, &"[file scope]");
                ctx.stack().mut_top().set_void();
                return do_invoke_partial(ctx.stack().mut_top(), ctx, sloc, ptc_aware_none, target);
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_declare_reference:
        {
          const auto& altr = this->m_stor.as<S_declare_reference>();

          struct Sparam
            {
              phsh_string name;
            };

          Sparam sp2;
          sp2.name = altr.name;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Declare a void reference.
                ctx.insert_named_reference(sp.name).clear();
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , nullptr
          );
          return;
        }

      case index_initialize_reference:
        {
          const auto& altr = this->m_stor.as<S_initialize_reference>();

          struct Sparam
            {
              phsh_string name;
            };

          Sparam sp2;
          sp2.name = altr.name;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Move a reference from the stack into the current context.
                ctx.insert_named_reference(sp.name) = move(ctx.stack().mut_top());
                ctx.stack().pop();
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_catch_expression:
        {
          const auto& altr = this->m_stor.as<S_catch_expression>();

          struct Sparam
            {
              AVM_Rod rod_body;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_body, altr.code_body);

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Save the stack.
                Reference_Stack saved_stack = move(ctx.stack());
                ctx.swap_stacks();

                // Evaluate the expression in a `try` block.
                Value exval;
                try {
                  AIR_Status status = sp.rod_body.execute(ctx);
                  ROCKET_ASSERT(status == air_status_next);
                }
                catch(Runtime_Error& except) {
                  exval = except.value();
                }

                // Restore the stack after the evaluation completes.
                ctx.swap_stacks();
                ctx.stack() = move(saved_stack);

                // Push the exception object.
                ctx.stack().push().set_temporary(move(exval));
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_body.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_return_statement:
        {
          const auto& altr = this->m_stor.as<S_return_statement>();

          Uparam up2;
          up2.b0 = altr.by_ref;
          up2.b1 = altr.is_void;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__, __flatten__)) -> AIR_Status
              {
                const bool by_ref = head->uparam.b0;
                const bool is_void = head->uparam.b1;
                const auto& sloc = head->pv_meta->sloc;

                ctx.global().call_hook(&Abstract_Hooks::on_return, sloc, ptc_aware_none);

                if(is_void || ctx.stack().top().is_void()) {
                  // Discard the result.
                  return air_status_return_void;
                }
                else if(by_ref) {
                  // The result is passed by reference, so check whether it is
                  // dereferenceable.
                  ctx.stack().top().dereference_readonly();
                  return air_status_return_ref;
                }
                else {
                  // The result is passed by copy, so convert it to a temporary.
                  ctx.stack().mut_top().dereference_copy();
                  return air_status_return_ref;
                }
              }

            // Uparam
            , up2

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_push_constant:
        {
          const auto& altr = this->m_stor.as<S_push_constant>();

          struct Sparam
            {
              Value val;
            };

          Sparam sp2;
          sp2.val = altr.val;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                ctx.stack().push().set_temporary(sp.val);
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.val.collect_variables(staged, temp);
              }

            // Symbols
            , nullptr
          );
          return;
        }

      case index_alt_clear_stack:
        {
          const auto& altr = this->m_stor.as<S_alt_clear_stack>();

          (void) altr;

          rod.append(
            +[](Executive_Context& ctx, const Header* /*head*/)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                ctx.swap_stacks();
                ctx.stack().clear();
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , nullptr
          );
          return;
        }

      case index_alt_function_call:
        {
          const auto& altr = this->m_stor.as<S_alt_function_call>();

          Uparam up2;
          up2.u0 = altr.ptc;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const PTC_Aware ptc = static_cast<PTC_Aware>(head->uparam.u0);
                const auto& sloc = head->pv_meta->sloc;
                const auto sentry = ctx.global().copy_recursion_sentry();

                ctx.swap_stacks();

                // Get the target function.
                const auto& target_value = ctx.stack().top().dereference_readonly();
                if(target_value.is_null())
                  throw Runtime_Error(xtc_format,
                           "Target function not found");

                if(!target_value.is_function())
                  throw Runtime_Error(xtc_format,
                           "Non-function value not invocable (target `$1`)", target_value);

                // Set the `this` reference and invoke the target function.
                auto target = target_value.as_function();
                ctx.stack().mut_top().pop_modifier();
                return do_invoke_partial(ctx.stack().mut_top(), ctx, sloc, ptc, target);
              }

            // Uparam
            , up2

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_coalesce_expression:
        {
          const auto& altr = this->m_stor.as<S_coalesce_expression>();

          Uparam up2;
          up2.b0 = altr.assign;

          struct Sparam
            {
              AVM_Rod rod_null;
            };

          Sparam sp2;
          do_solidify_nodes(sp2.rod_null, altr.code_null);

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const bool assign = head->uparam.b0;
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Read the condition and evaluate the corresponding subexpression.
                return ctx.stack().top().dereference_readonly().is_null()
                         ? do_evaluate_subexpression(ctx, assign, sp.rod_null)
                         : air_status_next;
              }

            // Uparam
            , up2

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , +[](Variable_HashMap& staged, Variable_HashMap& temp, const Header* head)
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);
                sp.rod_null.collect_variables(staged, temp);
              }

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_member_access:
        {
          const auto& altr = this->m_stor.as<S_member_access>();

          struct Sparam
            {
              phsh_string key;
            };

          Sparam sp2;
          sp2.key = altr.key;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__)) -> AIR_Status
              {
                const auto& sp = *reinterpret_cast<const Sparam*>(head->sparam);

                // Push a modifier.
                Reference_Modifier::S_object_key xmod = { sp.key };
                do_push_modifier_and_check(ctx.stack().mut_top(), move(xmod));
                return air_status_next;
              }

            // Uparam
            , Uparam()

            // Sparam
            , sizeof(sp2), do_sparam_ctor<Sparam>, &sp2, do_sparam_dtor<Sparam>

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }

      case index_apply_operator_bi32:
        {
          const auto& altr = this->m_stor.as<S_apply_operator_bi32>();

          Uparam up2;
          up2.b0 = altr.assign;
          up2.u1 = altr.xop;
          up2.i2345 = altr.irhs;

          switch(altr.xop)
            {
            case xop_inc:
            case xop_dec:
            case xop_unset:
            case xop_head:
            case xop_tail:
            case xop_random:
            case xop_isvoid:
            case xop_pos:
            case xop_neg:
            case xop_notb:
            case xop_notl:
            case xop_countof:
            case xop_typeof:
            case xop_sqrt:
            case xop_isnan:
            case xop_isinf:
            case xop_abs:
            case xop_sign:
            case xop_round:
            case xop_floor:
            case xop_ceil:
            case xop_trunc:
            case xop_iround:
            case xop_ifloor:
            case xop_iceil:
            case xop_itrunc:
            case xop_lzcnt:
            case xop_tzcnt:
            case xop_popcnt:
            case xop_fma:
              ASTERIA_TERMINATE(("Constant folding not implemented for `$1`"), altr.xop);

            case xop_assign:
            case xop_index:
              // binary
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__)) -> AIR_Status
                  {
                    const uint32_t uxop = head->uparam.u1;
                    const V_integer irhs = head->uparam.i2345;
                    auto& top = ctx.stack().mut_top();

                    switch(uxop)
                      {
                      case xop_assign:
                        {
                          // `assign` is ignored.
                          top.dereference_mutable() = irhs;
                          return air_status_next;
                        }

                      case xop_index:
                        {
                          // Push a subscript.
                          Reference_Modifier::S_array_index xmod = { irhs };
                          do_push_modifier_and_check(top, move(xmod));
                          return air_status_next;
                        }

                      default:
                        ROCKET_UNREACHABLE();
                      }
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            case xop_cmp_eq:
            case xop_cmp_ne:
            case xop_cmp_un:
            case xop_cmp_lt:
            case xop_cmp_gt:
            case xop_cmp_lte:
            case xop_cmp_gte:
            case xop_cmp_3way:
            case xop_add:
            case xop_sub:
            case xop_mul:
            case xop_div:
            case xop_mod:
            case xop_andb:
            case xop_orb:
            case xop_xorb:
            case xop_addm:
            case xop_subm:
            case xop_mulm:
            case xop_adds:
            case xop_subs:
            case xop_muls:
            case xop_sll:
            case xop_srl:
            case xop_sla:
            case xop_sra:
              // binary, shift
              rod.append(
                +[](Executive_Context& ctx, const Header* head)
                  __attribute__((__always_inline__, __flatten__)) -> AIR_Status
                  {
                    const bool assign = head->uparam.b0;
                    const uint32_t uxop = head->uparam.u1;
                    const V_integer irhs = head->uparam.i2345;
                    auto& top = ctx.stack().mut_top();
                    auto& lhs = assign ? top.dereference_mutable() : top.dereference_copy();

                    return do_apply_binary_operator_with_integer(uxop, lhs, irhs);
                  }

                // Uparam
                , up2

                // Sparam
                , 0, nullptr, nullptr, nullptr

                // Collector
                , nullptr

                // Symbols
                , &(altr.sloc)
              );
              return;

            default:
              ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
            }
        }

      case index_return_statement_bi32:
        {
          const auto& altr = this->m_stor.as<S_return_statement_bi32>();

          Uparam up2;
          up2.u0 = altr.type;
          up2.i2345 = altr.irhs;

          rod.append(
            +[](Executive_Context& ctx, const Header* head)
              __attribute__((__always_inline__, __flatten__)) -> AIR_Status
              {
                const Type type = static_cast<Type>(head->uparam.u0);
                const V_integer irhs = head->uparam.i2345;
                const auto& sloc = head->pv_meta->sloc;

                ctx.global().call_hook(&Abstract_Hooks::on_return, sloc, ptc_aware_none);

                if(type == type_null) {
                  // null
                  ctx.stack().push().set_temporary(nullopt);
                  return air_status_return_ref;
                }
                else if(type == type_boolean) {
                  // boolean; unnormalized
                  ctx.stack().push().set_temporary(-irhs < 0);
                  return air_status_return_ref;
                }
                else {
                  // integer
                  ctx.stack().push().set_temporary(irhs);
                  return air_status_return_ref;
                }
              }

            // Uparam
            , up2

            // Sparam
            , 0, nullptr, nullptr, nullptr

            // Collector
            , nullptr

            // Symbols
            , &(altr.sloc)
          );
          return;
        }
    }
  }

}  // namespace asteria
