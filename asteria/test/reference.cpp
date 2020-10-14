// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "util.hpp"
#include "../src/runtime/reference.hpp"
#include "../src/runtime/variable.hpp"

using namespace asteria;

int main()
  {
    auto ref = Reference(Reference::S_constant { V_string(::rocket::sref("meow")) });
    auto val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_string());
    ASTERIA_TEST_CHECK(val.as_string() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.dereference_mutable() = V_boolean(true));
    auto ref2 = ref;
    val = ref2.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_string());
    ASTERIA_TEST_CHECK(val.as_string() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.dereference_mutable() = V_boolean(true));

    ref = Reference::S_temporary { V_integer(42) };
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_integer());
    ASTERIA_TEST_CHECK(val.as_integer() == 42);
    ASTERIA_TEST_CHECK_CATCH(ref.dereference_mutable() = V_boolean(true));
    val = ref2.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_string());
    ASTERIA_TEST_CHECK(val.as_string() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.dereference_mutable() = V_boolean(true));

    auto var = ::rocket::make_refcnt<Variable>();
    var->initialize(V_null(), false);
    ref = Reference::S_variable { var };
    ref.zoom_in(Reference::M_array_index { -3 });
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_null());
    ref.dereference_mutable() = V_integer(36);
    ref.zoom_out();
    ref.zoom_in(Reference::M_array_index { 0 });
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_integer());
    ASTERIA_TEST_CHECK(val.as_integer() == 36);
    ref.zoom_out();

    ref.zoom_in(Reference::M_array_index { 2 });
    ref.zoom_in(Reference::M_object_key { phsh_string(::rocket::sref("my_key")) });
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_null());
    ref.dereference_mutable() = V_real(10.5);
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_real());
    ASTERIA_TEST_CHECK(val.as_real() == 10.5);
    ref.zoom_out();
    ref.zoom_out();
    ref.zoom_in(Reference::M_array_index { -1 });
    ref.zoom_in(Reference::M_object_key { phsh_string(::rocket::sref("my_key")) });
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_real());
    ASTERIA_TEST_CHECK(val.as_real() == 10.5);
    ref.zoom_in(Reference::M_object_key { phsh_string(::rocket::sref("invalid_access")) });
    ASTERIA_TEST_CHECK_CATCH(val = ref.dereference_readonly());
    ref.zoom_out();

    val = ref.dereference_unset();
    ASTERIA_TEST_CHECK(val.is_real());
    ASTERIA_TEST_CHECK(val.as_real() == 10.5);
    val = ref.dereference_readonly();
    ASTERIA_TEST_CHECK(val.is_null());
    val = ref.dereference_unset();
    ASTERIA_TEST_CHECK(val.is_null());
  }
