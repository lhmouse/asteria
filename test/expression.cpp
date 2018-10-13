// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/expression.hpp"
#include "../asteria/src/xpnode.hpp"
#include "../asteria/src/statement.hpp"
#include "../asteria/src/global_context.hpp"
#include "../asteria/src/executive_context.hpp"

using namespace Asteria;

int main()
  {
    Global_context global;
    Executive_context ctx;

    const auto cond = rocket::make_refcounted<Variable>();
    const auto dval = rocket::make_refcounted<Variable>(D_real(1.5), false);
    const auto ival = rocket::make_refcounted<Variable>(D_integer(3), false);
    const auto aval = rocket::make_refcounted<Variable>(D_array(), false);

    ctx.set_named_reference(String::shallow("cond"), Reference_root::S_variable { cond });
    ctx.set_named_reference(String::shallow("dval"), Reference_root::S_variable { dval });
    ctx.set_named_reference(String::shallow("ival"), Reference_root::S_variable { ival });
    ctx.set_named_reference(String::shallow("aval"), Reference_root::S_variable { aval });

    // Plain: aval[1] = !cond ? (dval++ + 0.25) : (ival * "hello,");
    // RPN:   aval 1 [] cond ! ?: =                    ::= expr
    //                         |\-- dval ++ 0.25 +     ::= branch_true
    //                         \--- ival "hello," *    ::= branch_false
    Vector<Xpnode> branch_true;
    {
      branch_true.emplace_back(Xpnode::S_named_reference { String::shallow("dval") });
      branch_true.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_postfix_inc, false });
      branch_true.emplace_back(Xpnode::S_literal { D_real(0.25) });
      branch_true.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_add, false });
    }
    Vector<Xpnode> branch_false;
    {
      branch_false.emplace_back(Xpnode::S_named_reference { String::shallow("ival") });
      branch_false.emplace_back(Xpnode::S_literal { D_string("hello,") });
      branch_false.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_mul, false });
    }
    Vector<Xpnode> nodes;
    {
      nodes.emplace_back(Xpnode::S_named_reference { String::shallow("aval") });
      nodes.emplace_back(Xpnode::S_literal { D_integer(1) });
      nodes.emplace_back(Xpnode::S_subscript { String() });
      nodes.emplace_back(Xpnode::S_named_reference { String::shallow("cond") });
      nodes.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_prefix_notl, false });
      nodes.emplace_back(Xpnode::S_branch { false, std::move(branch_true), std::move(branch_false) });
      nodes.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_assign, false });
    }
    auto expr = Expression(std::move(nodes));

    auto result = expr.evaluate(global, ctx);
    auto value = dval->get_value();
    ASTERIA_TEST_CHECK(value.check<D_real>() == 2.5);
    value = ival->get_value();
    ASTERIA_TEST_CHECK(value.check<D_integer>() == 3);
    value = aval->get_value();
    ASTERIA_TEST_CHECK(value.check<D_array>().at(1).check<D_real>() == 1.75);
    value = result.read();
    ASTERIA_TEST_CHECK(value.check<D_real>() == 1.75);

    cond->set_value(D_integer(42));
    result = expr.evaluate(global, ctx);
    value = dval->get_value();
    ASTERIA_TEST_CHECK(value.check<D_real>() == 2.5);
    value = ival->get_value();
    ASTERIA_TEST_CHECK(value.check<D_integer>() == 3);
    value = aval->get_value();
    ASTERIA_TEST_CHECK(value.check<D_array>().at(1).check<D_string>() == "hello,hello,hello,");
    value = result.read();
    ASTERIA_TEST_CHECK(value.check<D_string>() == "hello,hello,hello,");
 }
