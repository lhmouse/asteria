// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/statement.hpp"
#include "../src/context.hpp"

using namespace Asteria;

int main()
  {
    Vector<Statement> text;
    // var i = 0;
    Vector<Xpnode> expr;
    expr.emplace_back(Xpnode::S_literal { D_integer(0) });
    text.emplace_back(Statement::S_var_def { String::shallow("i"), false, std::move(expr) });
    // const data = [ 1,2,3,2*5 ];
    Vector<Vector<Xpnode>> elems;
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
    // for(each k, v in data){
    //   i += k * v;
    // }
    Vector<Xpnode> range;
    range.emplace_back(Xpnode::S_named_reference { String::shallow("data") });
    expr.clear();
    expr.emplace_back(Xpnode::S_named_reference { String::shallow("k") });
    expr.emplace_back(Xpnode::S_named_reference { String::shallow("v") });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_mul, false });
    expr.emplace_back(Xpnode::S_named_reference { String::shallow("i") });
    expr.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_add, true });
    Vector<Statement> body;
    body.emplace_back(Statement::S_expression { std::move(expr) });
    text.emplace_back(Statement::S_for_each { String::shallow("k"), String::shallow("v"), std::move(range), std::move(body) });

    auto ctx = allocate<Context>(nullptr, false);
    Reference ref;
    auto status = execute_block_in_place(ref, ctx, text);
    ASTERIA_TEST_CHECK(status == Statement::status_next);
    auto qref = ctx->get_named_reference_opt(String::shallow("i"));
    ASTERIA_TEST_CHECK(qref != nullptr);
    ASTERIA_TEST_CHECK(read_reference(*qref).as<D_integer>() == 38);
    qref = ctx->get_named_reference_opt(String::shallow("data"));
    ASTERIA_TEST_CHECK(qref != nullptr);
    ASTERIA_TEST_CHECK(read_reference(*qref).as<D_array>().size() == 4);
    ASTERIA_TEST_CHECK(read_reference(*qref).as<D_array>().at(0).as<D_integer>() ==  1);
    ASTERIA_TEST_CHECK(read_reference(*qref).as<D_array>().at(1).as<D_integer>() ==  2);
    ASTERIA_TEST_CHECK(read_reference(*qref).as<D_array>().at(2).as<D_integer>() ==  3);
    ASTERIA_TEST_CHECK(read_reference(*qref).as<D_array>().at(3).as<D_integer>() == 10);
    qref = ctx->get_named_reference_opt(String::shallow("k"));
    ASTERIA_TEST_CHECK(qref == nullptr);
    qref = ctx->get_named_reference_opt(String::shallow("v"));
    ASTERIA_TEST_CHECK(qref == nullptr);
  }
