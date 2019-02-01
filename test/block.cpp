// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/syntax/block.hpp"
#include "../asteria/src/syntax/xpnode.hpp"
#include "../asteria/src/syntax/statement.hpp"
#include "../asteria/src/runtime/global_context.hpp"
#include "../asteria/src/runtime/executive_context.hpp"

using namespace Asteria;

int main()
  {
    CoW_Vector<Statement> text;
    // var res = 0;
    CoW_Vector<Xpnode> expr;
    expr.emplace_back(Xpnode::S_literal { D_integer(0) });
    text.emplace_back(Statement::S_variable { Source_Location(rocket::sref("nonexistent"), 1), rocket::sref("res"), false, std::move(expr) });
    // const data = [ 1, 2, 3, 2 * 5 ];
    expr.clear();
    expr.emplace_back(Xpnode::S_literal { D_integer(1) });
    expr.emplace_back(Xpnode::S_literal { D_integer(2) });
    expr.emplace_back(Xpnode::S_literal { D_integer(3) });
    expr.emplace_back(Xpnode::S_literal { D_integer(2) });
    expr.emplace_back(Xpnode::S_literal { D_integer(5) });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_mul, false });
    expr.emplace_back(Xpnode::S_unnamed_array { 4 });
    text.emplace_back(Statement::S_variable { Source_Location(rocket::sref("nonexistent"), 2), rocket::sref("data"), true, std::move(expr) });
    // for(each k, v in data) {
    //   res += k * v;
    // }
    CoW_Vector<Xpnode> range;
    range.emplace_back(Xpnode::S_named_reference { rocket::sref("data") });
    expr.clear();
    expr.emplace_back(Xpnode::S_named_reference { rocket::sref("res") });
    expr.emplace_back(Xpnode::S_named_reference { rocket::sref("k") });
    expr.emplace_back(Xpnode::S_named_reference { rocket::sref("v") });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_mul, false });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_add, true });
    CoW_Vector<Statement> body;
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
    expr.emplace_back(Xpnode::S_named_reference { rocket::sref("res") });
    expr.emplace_back(Xpnode::S_named_reference { rocket::sref("data") });
    expr.emplace_back(Xpnode::S_named_reference { rocket::sref("j") });
    expr.emplace_back(Xpnode::S_subscript { rocket::sref("") });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_add, true });
    body.emplace_back(Statement::S_expression { std::move(expr) });
    expr.clear();
    expr.emplace_back(Xpnode::S_named_reference { rocket::sref("data") });
    expr.emplace_back(Xpnode::S_named_reference { rocket::sref("j") });
    expr.emplace_back(Xpnode::S_subscript { rocket::sref("") });
    expr.emplace_back(Xpnode::S_literal { D_integer(2) });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_cmp_eq, false });
    CoW_Vector<Statement> branch_true;
    branch_true.emplace_back(Statement::S_break { Statement::target_unspec });
    body.emplace_back(Statement::S_if { false, std::move(expr), std::move(branch_true), Block() });
    expr.clear();
    expr.emplace_back(Xpnode::S_literal { D_integer(0) });
    CoW_Vector<Statement> init;
    init.emplace_back(Statement::S_variable { Source_Location(rocket::sref("nonexistent"), 3), rocket::sref("j"), false, std::move(expr) });
    CoW_Vector<Xpnode> cond;
    cond.emplace_back(Xpnode::S_named_reference { rocket::sref("j") });
    cond.emplace_back(Xpnode::S_literal { D_integer(3) });
    cond.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_cmp_lte, false });
    CoW_Vector<Xpnode> step;
    step.emplace_back(Xpnode::S_named_reference { rocket::sref("j") });
    step.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_prefix_inc, false });
    text.emplace_back(Statement::S_for { std::move(init), std::move(cond), std::move(step), std::move(body) });
    auto block = Block(std::move(text));

    Global_Context global;
    Executive_Context ctx(nullptr);
    Reference ref;
    auto status = block.execute_in_place(ref, ctx, rocket::sref("dummy_function"), global);
    ASTERIA_TEST_CHECK(status == Block::status_next);
    auto qref = ctx.get_named_reference_opt(rocket::sref("res"));
    ASTERIA_TEST_CHECK(qref != nullptr);
    ASTERIA_TEST_CHECK(qref->read().check<D_integer>() == 41);
    qref = ctx.get_named_reference_opt(rocket::sref("data"));
    ASTERIA_TEST_CHECK(qref != nullptr);
    ASTERIA_TEST_CHECK(qref->read().check<D_array>().size() == 4);
    ASTERIA_TEST_CHECK(qref->read().check<D_array>().at(0).check<D_integer>() ==  1);
    ASTERIA_TEST_CHECK(qref->read().check<D_array>().at(1).check<D_integer>() ==  2);
    ASTERIA_TEST_CHECK(qref->read().check<D_array>().at(2).check<D_integer>() ==  3);
    ASTERIA_TEST_CHECK(qref->read().check<D_array>().at(3).check<D_integer>() == 10);
    qref = ctx.get_named_reference_opt(rocket::sref("k"));
    ASTERIA_TEST_CHECK(qref == nullptr);
    qref = ctx.get_named_reference_opt(rocket::sref("v"));
    ASTERIA_TEST_CHECK(qref == nullptr);
    qref = ctx.get_named_reference_opt(rocket::sref("j"));
    ASTERIA_TEST_CHECK(qref == nullptr);
  }
