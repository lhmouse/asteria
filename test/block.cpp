// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/block.hpp"
#include "../src/xpnode.hpp"
#include "../src/statement.hpp"
#include "../src/executive_context.hpp"

using namespace Asteria;

int main()
  {
    Vector<Statement> text;
    // var res = 0;
    Vector<Xpnode> expr;
    expr.emplace_back(Xpnode::S_literal { D_integer(0) });
    text.emplace_back(Statement::S_var_def { String::shallow("res"), false, std::move(expr) });
    // const data = [ 1, 2, 3, 2 * 5 ];
    Vector<Expression> elems;
    for(int i = 1; i <= 3; ++i) {
      expr.clear();
      expr.emplace_back(Xpnode::S_literal { D_integer(i) });
      elems.emplace_back(std::move(expr));
    }
    expr.clear();
    expr.emplace_back(Xpnode::S_literal { D_integer(2) });
    expr.emplace_back(Xpnode::S_literal { D_integer(5) });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_mul, false });
    elems.emplace_back(std::move(expr));
    expr.clear();
    expr.emplace_back(Xpnode::S_unnamed_array { std::move(elems) });
    text.emplace_back(Statement::S_var_def { String::shallow("data"), true, std::move(expr) });
    // for(each k, v in data) {
    //   res += k * v;
    // }
    Vector<Xpnode> range;
    range.emplace_back(Xpnode::S_named_reference { String::shallow("data") });
    expr.clear();
    expr.emplace_back(Xpnode::S_named_reference { String::shallow("k") });
    expr.emplace_back(Xpnode::S_named_reference { String::shallow("v") });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_mul, false });
    expr.emplace_back(Xpnode::S_named_reference { String::shallow("res") });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_add, true });
    Vector<Statement> body;
    body.emplace_back(Statement::S_expression { std::move(expr) });
    text.emplace_back(Statement::S_for_each { String::shallow("k"), String::shallow("v"), std::move(range), std::move(body) });
    // for(var j = 0; j <= 3; ++j) {
    //   res += data[j];
    //   if(data[j] == 2) {
    //     break;
    //   }
    // }
    body.clear();
    expr.clear();
    expr.emplace_back(Xpnode::S_named_reference { String::shallow("j") });
    expr.emplace_back(Xpnode::S_named_reference { String::shallow("data") });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_postfix_at, false });
    expr.emplace_back(Xpnode::S_named_reference { String::shallow("res") });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_add, true });
    body.emplace_back(Statement::S_expression { std::move(expr) });
    expr.clear();
    expr.emplace_back(Xpnode::S_literal { D_integer(2) });
    expr.emplace_back(Xpnode::S_named_reference { String::shallow("j") });
    expr.emplace_back(Xpnode::S_named_reference { String::shallow("data") });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_postfix_at, false });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_cmp_eq, false });
    Vector<Statement> branch_true;
    branch_true.emplace_back(Statement::S_break { Statement::target_scope_unspec });
    body.emplace_back(Statement::S_if { std::move(expr), std::move(branch_true), { } });
    expr.clear();
    expr.emplace_back(Xpnode::S_literal { D_integer(0) });
    Vector<Xpnode> cond;
    cond.emplace_back(Xpnode::S_literal { D_integer(3) });
    cond.emplace_back(Xpnode::S_named_reference { String::shallow("j") });
    cond.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_cmp_lte, false });
    Vector<Xpnode> step;
    step.emplace_back(Xpnode::S_named_reference { String::shallow("j") });
    step.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_prefix_inc, false });
    text.emplace_back(Statement::S_for { String::shallow("j"), false, std::move(expr), std::move(cond), std::move(step), std::move(body) });
    auto block = Block(std::move(text));

    Executive_context ctx;
    Reference ref;
    auto status = block.execute_in_place(ref, ctx);
    ASTERIA_TEST_CHECK(status == Block::status_next);
    auto qref = ctx.get_named_reference_opt(String::shallow("res"));
    ASTERIA_TEST_CHECK(qref != nullptr);
    ASTERIA_TEST_CHECK(qref->read().check<D_integer>() == 41);
    qref = ctx.get_named_reference_opt(String::shallow("data"));
    ASTERIA_TEST_CHECK(qref != nullptr);
    ASTERIA_TEST_CHECK(qref->read().check<D_array>().size() == 4);
    ASTERIA_TEST_CHECK(qref->read().check<D_array>().at(0).check<D_integer>() ==  1);
    ASTERIA_TEST_CHECK(qref->read().check<D_array>().at(1).check<D_integer>() ==  2);
    ASTERIA_TEST_CHECK(qref->read().check<D_array>().at(2).check<D_integer>() ==  3);
    ASTERIA_TEST_CHECK(qref->read().check<D_array>().at(3).check<D_integer>() == 10);
    qref = ctx.get_named_reference_opt(String::shallow("k"));
    ASTERIA_TEST_CHECK(qref == nullptr);
    qref = ctx.get_named_reference_opt(String::shallow("v"));
    ASTERIA_TEST_CHECK(qref == nullptr);
    qref = ctx.get_named_reference_opt(String::shallow("j"));
    ASTERIA_TEST_CHECK(qref == nullptr);
  }
