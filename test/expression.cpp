// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/syntax/expression.hpp"
#include "../asteria/src/syntax/xpnode.hpp"
#include "../asteria/src/syntax/statement.hpp"
#include "../asteria/src/runtime/global_context.hpp"
#include "../asteria/src/runtime/executive_context.hpp"

using namespace Asteria;

int main()
  {
    Global_Context global;
    Executive_Context ctx(nullptr);

    const auto cond = rocket::make_refcnt<Variable>();
    cond->reset(D_null(), false);
    ctx.open_named_reference(rocket::sref("cond")) = Reference_Root::S_variable { cond };

    const auto dval = rocket::make_refcnt<Variable>();
    ctx.open_named_reference(rocket::sref("dval")) = Reference_Root::S_variable { dval };
    dval->reset(D_real(1.5), false);

    const auto ival = rocket::make_refcnt<Variable>();
    ctx.open_named_reference(rocket::sref("ival")) = Reference_Root::S_variable { ival };
    ival->reset(D_integer(3), false);

    const auto aval = rocket::make_refcnt<Variable>();
    aval->reset(D_array(), false);
    ctx.open_named_reference(rocket::sref("aval")) = Reference_Root::S_variable { aval };

    // Plain: aval[1] = !cond ? (dval++ + 0.25) : (ival * "hello,");
    // RPN:   aval 1 [] cond ! ?: =                    ::= expr
    //                         |\-- dval ++ 0.25 +     ::= branch_true
    //                         \--- ival "hello," *    ::= branch_false
    CoW_Vector<Xpnode> branch_true;
    {
      branch_true.emplace_back(Xpnode::S_named_reference { rocket::sref("dval") });
      branch_true.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_postfix_inc, false });
      branch_true.emplace_back(Xpnode::S_literal { D_real(0.25) });
      branch_true.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_add, false });
    }
    CoW_Vector<Xpnode> branch_false;
    {
      branch_false.emplace_back(Xpnode::S_named_reference { rocket::sref("ival") });
      branch_false.emplace_back(Xpnode::S_literal { D_string("hello,") });
      branch_false.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_mul, false });
    }
    CoW_Vector<Xpnode> nodes;
    {
      nodes.emplace_back(Xpnode::S_named_reference { rocket::sref("aval") });
      nodes.emplace_back(Xpnode::S_literal { D_integer(1) });
      nodes.emplace_back(Xpnode::S_subscript { CoW_String() });
      nodes.emplace_back(Xpnode::S_named_reference { rocket::sref("cond") });
      nodes.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_prefix_notl, false });
      nodes.emplace_back(Xpnode::S_branch { std::move(branch_true), std::move(branch_false), false });
      nodes.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_assign, false });
    }
    auto expr = Expression(std::move(nodes));

    Reference result;
    expr.evaluate(result, rocket::sref("dummy_function"), global, ctx);
    auto value = dval->get_value();
    ASTERIA_TEST_CHECK(value.check<D_real>() == 2.5);
    value = ival->get_value();
    ASTERIA_TEST_CHECK(value.check<D_integer>() == 3);
    value = aval->get_value();
    ASTERIA_TEST_CHECK(value.check<D_array>().at(1).check<D_real>() == 1.75);
    value = result.read();
    ASTERIA_TEST_CHECK(value.check<D_real>() == 1.75);

    cond->open_value() = D_integer(42);
    expr.evaluate(result, rocket::sref("dummy_function"), global, ctx);
    value = dval->get_value();
    ASTERIA_TEST_CHECK(value.check<D_real>() == 2.5);
    value = ival->get_value();
    ASTERIA_TEST_CHECK(value.check<D_integer>() == 3);
    value = aval->get_value();
    ASTERIA_TEST_CHECK(value.check<D_array>().at(1).check<D_string>() == "hello,hello,hello,");
    value = result.read();
    ASTERIA_TEST_CHECK(value.check<D_string>() == "hello,hello,hello,");
 }
