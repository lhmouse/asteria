// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/syntax/statement.hpp"
#include "../src/syntax/xprunit.hpp"
#include "../src/runtime/global_context.hpp"
#include "../src/runtime/executive_context.hpp"
#include "../src/runtime/air_node.hpp"
#include "../src/runtime/evaluation_stack.hpp"
#include "../src/runtime/analytic_context.hpp"

using namespace Asteria;

int main()
  {
    Global_Context global;
    Executive_Context ctx(nullptr);

    Cow_Vector<Statement> text;
    // var res = 0;
    Cow_Vector<Xprunit> expr;
    expr.emplace_back(Xprunit::S_literal { G_integer(0) });
    text.emplace_back(Statement::S_variable { Source_Location(rocket::sref("nonexistent"), 1), false,
                                              { { rocket::sref("res"), std::move(expr) } } });
    // const data = [ 1, 2, 3, 2 * 5 ];
    expr.clear();
    expr.emplace_back(Xprunit::S_literal { G_integer(1) });
    expr.emplace_back(Xprunit::S_literal { G_integer(2) });
    expr.emplace_back(Xprunit::S_literal { G_integer(3) });
    expr.emplace_back(Xprunit::S_literal { G_integer(2) });
    expr.emplace_back(Xprunit::S_literal { G_integer(5) });
    expr.emplace_back(Xprunit::S_operator_rpn { Xprunit::xop_infix_mul, false });
    expr.emplace_back(Xprunit::S_unnamed_array { 4 });
    text.emplace_back(Statement::S_variable { Source_Location(rocket::sref("nonexistent"), 2), true,
                                              { { rocket::sref("data"), std::move(expr) } } });
    // for(each k, v in data) {
    //   res += k * v;
    // }
    Cow_Vector<Xprunit> range;
    range.emplace_back(Xprunit::S_named_reference { rocket::sref("data") });
    expr.clear();
    expr.emplace_back(Xprunit::S_named_reference { rocket::sref("res") });
    expr.emplace_back(Xprunit::S_named_reference { rocket::sref("k") });
    expr.emplace_back(Xprunit::S_named_reference { rocket::sref("v") });
    expr.emplace_back(Xprunit::S_operator_rpn { Xprunit::xop_infix_mul, false });
    expr.emplace_back(Xprunit::S_operator_rpn { Xprunit::xop_infix_add, true });
    Cow_Vector<Statement> body;
    body.emplace_back(Statement::S_expression { std::move(expr) });
    text.emplace_back(Statement::S_for_each { rocket::sref("k"), rocket::sref("v"), std::move(range), std::move(body) });
    // for(var j = 0; j <= 3; ++j) {
    //   res += data[j];
    //   if(data[j] == 2) {
    //     break;
    //   }
    // }
    body.clear();
    expr.clear();
    expr.emplace_back(Xprunit::S_named_reference { rocket::sref("res") });
    expr.emplace_back(Xprunit::S_named_reference { rocket::sref("data") });
    expr.emplace_back(Xprunit::S_named_reference { rocket::sref("j") });
    expr.emplace_back(Xprunit::S_operator_rpn { Xprunit::xop_postfix_at, false });
    expr.emplace_back(Xprunit::S_operator_rpn { Xprunit::xop_infix_add, true });
    body.emplace_back(Statement::S_expression { std::move(expr) });
    expr.clear();
    expr.emplace_back(Xprunit::S_named_reference { rocket::sref("data") });
    expr.emplace_back(Xprunit::S_named_reference { rocket::sref("j") });
    expr.emplace_back(Xprunit::S_operator_rpn { Xprunit::xop_postfix_at, false });
    expr.emplace_back(Xprunit::S_literal { G_integer(2) });
    expr.emplace_back(Xprunit::S_operator_rpn { Xprunit::xop_infix_cmp_eq, false });
    Cow_Vector<Statement> branch_true;
    branch_true.emplace_back(Statement::S_break { Statement::target_unspec });
    body.emplace_back(Statement::S_if { false, std::move(expr), std::move(branch_true), { } });
    expr.clear();
    expr.emplace_back(Xprunit::S_literal { G_integer(0) });
    Cow_Vector<Statement> init;
    init.emplace_back(Statement::S_variable { Source_Location(rocket::sref("nonexistent"), 3), false,
                                              { { rocket::sref("j"), std::move(expr) } } });
    Cow_Vector<Xprunit> cond;
    cond.emplace_back(Xprunit::S_named_reference { rocket::sref("j") });
    cond.emplace_back(Xprunit::S_literal { G_integer(3) });
    cond.emplace_back(Xprunit::S_operator_rpn { Xprunit::xop_infix_cmp_lte, false });
    Cow_Vector<Xprunit> step;
    step.emplace_back(Xprunit::S_named_reference { rocket::sref("j") });
    step.emplace_back(Xprunit::S_operator_rpn { Xprunit::xop_prefix_inc, false });
    text.emplace_back(Statement::S_for { std::move(init), std::move(cond), std::move(step), std::move(body) });
    // Generate code.
    Cow_Vector<Air_Node> stmt_code;
    Analytic_Context actx(&ctx);
    rocket::for_each(text, [&](const Statement& stmt) { stmt.generate_code(stmt_code, nullptr, actx);  });

    auto status = Air_Node::status_next;
    Evaluation_Stack stack;
    rocket::any_of(stmt_code, [&](const Air_Node& node) { return (status = node.execute(stack, ctx, rocket::sref("dummy_function"), global)) != Air_Node::status_next;  });
    ASTERIA_TEST_CHECK(status == Air_Node::status_next);
    auto qref = ctx.get_named_reference_opt(rocket::sref("res"));
    ASTERIA_TEST_CHECK(qref != nullptr);
    ASTERIA_TEST_CHECK(qref->read().check<G_integer>() == 41);
    qref = ctx.get_named_reference_opt(rocket::sref("data"));
    ASTERIA_TEST_CHECK(qref != nullptr);
    ASTERIA_TEST_CHECK(qref->read().check<G_array>().size() == 4);
    ASTERIA_TEST_CHECK(qref->read().check<G_array>().at(0).check<G_integer>() ==  1);
    ASTERIA_TEST_CHECK(qref->read().check<G_array>().at(1).check<G_integer>() ==  2);
    ASTERIA_TEST_CHECK(qref->read().check<G_array>().at(2).check<G_integer>() ==  3);
    ASTERIA_TEST_CHECK(qref->read().check<G_array>().at(3).check<G_integer>() == 10);
    qref = ctx.get_named_reference_opt(rocket::sref("k"));
    ASTERIA_TEST_CHECK(qref == nullptr);
    qref = ctx.get_named_reference_opt(rocket::sref("v"));
    ASTERIA_TEST_CHECK(qref == nullptr);
    qref = ctx.get_named_reference_opt(rocket::sref("j"));
    ASTERIA_TEST_CHECK(qref == nullptr);
  }
