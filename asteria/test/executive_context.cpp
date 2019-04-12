// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/executive_context.hpp"
#include "../src/runtime/reference.hpp"

using namespace Asteria;

int main()
  {
    Executive_Context ctx(nullptr);
    ctx.open_named_reference(rocket::sref("test")) = Reference_Root::S_constant{ G_integer(42) };
    auto qref = ctx.get_named_reference_opt(rocket::sref("test"));
    ASTERIA_TEST_CHECK(qref != nullptr);
    ASTERIA_TEST_CHECK(qref->read().as_integer() == 42);

    qref = ctx.get_named_reference_opt(rocket::sref("nonexistent"));
    ASTERIA_TEST_CHECK(qref == nullptr);
  }
