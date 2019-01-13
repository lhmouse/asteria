// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/runtime/executive_context.hpp"
#include "../asteria/src/runtime/reference.hpp"

using namespace Asteria;

int main()
  {
    Executive_context ctx(nullptr);
    ctx.open_named_reference(std::ref("test")) = Reference_root::S_constant{ D_integer(42) };
    auto qref = ctx.get_named_reference_opt(std::ref("test"));
    ASTERIA_TEST_CHECK(qref != nullptr);
    ASTERIA_TEST_CHECK(qref->read().check<D_integer>() == 42);

    qref = ctx.get_named_reference_opt(std::ref("nonexistent"));
    ASTERIA_TEST_CHECK(qref == nullptr);
  }
