// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/executive_context.hpp"
#include "../src/reference.hpp"

using namespace Asteria;

int main()
  {
    Executive_context ctx;
    ctx.set_named_reference(String::shallow("test"), Reference(Reference_root::S_constant{ D_integer(42) }));
    auto qref = ctx.get_named_reference_opt(String::shallow("test"));
    ASTERIA_TEST_CHECK(qref != nullptr);
    ASTERIA_TEST_CHECK(qref->read().check<D_integer>() == 42);

    qref = ctx.get_named_reference_opt(String::shallow("nonexistent"));
    ASTERIA_TEST_CHECK(qref == nullptr);
  }
