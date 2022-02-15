// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/runtime/reference.hpp"
#include "../src/runtime/variable.hpp"

using namespace ::asteria;

int main()
  {
    Reference ref;
    ref.set_temporary(V_string(sref("meow")));
    auto val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_string());
    ASTERIA_TEST_CHECK(val.as_string() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.dereference_mutable() = V_boolean(true));
    auto ref2 = ref;
    val = ref2.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_string());
    ASTERIA_TEST_CHECK(val.as_string() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.dereference_mutable() = V_boolean(true));

    ref.set_temporary(V_integer(42));
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_integer());
    ASTERIA_TEST_CHECK(val.as_integer() == 42);
    ASTERIA_TEST_CHECK_CATCH(ref.dereference_mutable() = V_boolean(true));
    val = ref2.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_string());
    ASTERIA_TEST_CHECK(val.as_string() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.dereference_mutable() = V_boolean(true));

    auto var = ::rocket::make_refcnt<Variable>();
    var->initialize(V_null());
    ref.set_variable(var);
    ref.push_modifier_array_index(-3);
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_null());
    ref.dereference_mutable() = V_integer(36);
    ref.pop_modifier();
    ref.push_modifier_array_index(0);
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_integer());
    ASTERIA_TEST_CHECK(val.as_integer() == 36);
    ref.pop_modifier();

    ref.push_modifier_array_index(2);
    ref.push_modifier_object_key(sref("my_key"));
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_null());
    ref.dereference_mutable() = V_real(10.5);
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_real());
    ASTERIA_TEST_CHECK(val.as_real() == 10.5);
    ref.pop_modifier();
    ref.pop_modifier();
    ref.push_modifier_array_index(-1);
    ref.push_modifier_object_key(sref("my_key"));
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_real());
    ASTERIA_TEST_CHECK(val.as_real() == 10.5);
    ref.push_modifier_object_key(sref("invalid_access"));
    ASTERIA_TEST_CHECK_CATCH(val = ref.dereference_readonly());
    ref.pop_modifier();

    val = ref.dereference_unset();
    ASTERIA_TEST_CHECK(val.is_real());
    ASTERIA_TEST_CHECK(val.as_real() == 10.5);
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_null());
    val = ref.dereference_unset();
    ASTERIA_TEST_CHECK(val.is_null());
  }
