// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/xpnode.hpp"
#include "../src/context.hpp"

using namespace Asteria;

int main()
  {
    const auto ctx = allocate<Context>(nullptr, false);
    auto &cond = *(ctx->set_named_reference_opt(String::shallow("cond"), Reference(Reference_root::S_constant { D_null()      })).first);
    auto &dval = *(ctx->set_named_reference_opt(String::shallow("dval"), Reference(Reference_root::S_constant { D_double(1.5) })).first);
    auto &ival = *(ctx->set_named_reference_opt(String::shallow("ival"), Reference(Reference_root::S_constant { D_integer(3)  })).first);
    auto &aval = *(ctx->set_named_reference_opt(String::shallow("aval"), Reference(Reference_root::S_constant { D_array()     })).first);

    materialize_reference(cond);
    materialize_reference(dval);
    materialize_reference(ival);
    materialize_reference(aval);

    // Plain: aval[1] = !cond ? (dval++ + 0.25) : (ival * "hello,");
    // RPN:   cond ! ?: 1 aval [] =          ::= expr
    //               |\-- 0.25 dval ++ +     ::= branch_true
    //               \--- "hello," ival *    ::= branch_false
    Vector<Xpnode> branch_true;
      {
        branch_true.emplace_back(Xpnode::S_literal { D_double(0.25) });
        branch_true.emplace_back(Xpnode::S_named_reference { String::shallow("dval") });
        branch_true.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_postfix_inc, false });
        branch_true.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_add, false });
      }
    Vector<Xpnode> branch_false;
      {
        branch_false.emplace_back(Xpnode::S_literal { D_string("hello,") });
        branch_false.emplace_back(Xpnode::S_named_reference { String::shallow("ival") });
        branch_false.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_mul, false });
      }
    Vector<Xpnode> expr;
      {
        expr.emplace_back(Xpnode::S_named_reference { String::shallow("cond") });
        expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_prefix_notl, false });
        expr.emplace_back(Xpnode::S_branch { std::move(branch_true), std::move(branch_false) });
        expr.emplace_back(Xpnode::S_literal { D_integer(1) });
        expr.emplace_back(Xpnode::S_named_reference { String::shallow("aval") });
        expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_postfix_at, false });
        expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_assign, false });
      }

    auto result = evaluate_expression(expr, ctx);
    auto value = read_reference(dval);
    ASTERIA_TEST_CHECK(value.as<D_double>() == 2.5);
    value = read_reference(ival);
    ASTERIA_TEST_CHECK(value.as<D_integer>() == 3);
    value = read_reference(aval);
    ASTERIA_TEST_CHECK(value.as<D_array>().at(1).as<D_double>() == 1.75);
    value = read_reference(result);
    ASTERIA_TEST_CHECK(value.as<D_double>() == 1.75);

    write_reference(cond, D_integer(42));
    result = evaluate_expression(expr, ctx);
    value = read_reference(dval);
    ASTERIA_TEST_CHECK(value.as<D_double>() == 2.5);
    value = read_reference(ival);
    ASTERIA_TEST_CHECK(value.as<D_integer>() == 3);
    value = read_reference(aval);
    ASTERIA_TEST_CHECK(value.as<D_array>().at(1).as<D_string>() == "hello,hello,hello,");
    value = read_reference(result);
    ASTERIA_TEST_CHECK(value.as<D_string>() == "hello,hello,hello,");
 }
