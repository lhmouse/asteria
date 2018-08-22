// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/context.hpp"

using namespace Asteria;

int main()
  {
    auto ctx = allocate<Context>(nullptr, false);

    ctx->set_named_reference(String::shallow("test"), Reference(Reference_root::S_constant{ D_integer(42) }));
    auto ref = ctx->get_named_reference_opt(String::shallow("test"));
    ASTERIA_TEST_CHECK(ref != nullptr);
    ASTERIA_TEST_CHECK(ref->get_root().index() == Reference_root::index_constant);
    ASTERIA_TEST_CHECK(read_reference(*ref).as<D_integer>() == 42);

    ref = ctx->get_named_reference_opt(String::shallow("nonexistent"));
    ASTERIA_TEST_CHECK(ref == nullptr);
  }
