// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "utilities.hpp"
#include "../src/runtime/reference.hpp"
#include "../src/runtime/variable.hpp"

using namespace Asteria;

int main()
  {
    auto ref = Reference(Reference_root::S_constant { V_string(::rocket::sref("meow")) });
    auto val = ref.read();
    ASTERIA_TEST_CHECK(val.is_string());
    ASTERIA_TEST_CHECK(val.as_string() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.open() = V_boolean(true));
    auto ref2 = ref;
    val = ref2.read();
    ASTERIA_TEST_CHECK(val.is_string());
    ASTERIA_TEST_CHECK(val.as_string() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.open() = V_boolean(true));

    ref = Reference_root::S_temporary { V_integer(42) };
    val = ref.read();
    ASTERIA_TEST_CHECK(val.is_integer());
    ASTERIA_TEST_CHECK(val.as_integer() == 42);
    ASTERIA_TEST_CHECK_CATCH(ref.open() = V_boolean(true));
    val = ref2.read();
    ASTERIA_TEST_CHECK(val.is_string());
    ASTERIA_TEST_CHECK(val.as_string() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.open() = V_boolean(true));

    auto var = ::rocket::make_refcnt<Variable>();
    var->initialize(V_null(), false);
    ref = Reference_root::S_variable { var };
    ref.zoom_in(Reference_modifier::S_array_index { -3 });
    val = ref.read();
    ASTERIA_TEST_CHECK(val.is_null());
    ref.open() = V_integer(36);
    ref.zoom_out();
    ref.zoom_in(Reference_modifier::S_array_index { 0 });
    val = ref.read();
    ASTERIA_TEST_CHECK(val.is_integer());
    ASTERIA_TEST_CHECK(val.as_integer() == 36);
    ref.zoom_out();

    ref.zoom_in(Reference_modifier::S_array_index { 2 });
    ref.zoom_in(Reference_modifier::S_object_key { phsh_string(::rocket::sref("my_key")) });
    val = ref.read();
    ASTERIA_TEST_CHECK(val.is_null());
    ref.open() = V_real(10.5);
    val = ref.read();
    ASTERIA_TEST_CHECK(val.is_real());
    ASTERIA_TEST_CHECK(val.as_real() == 10.5);
    ref.zoom_out();
    ref.zoom_out();
    ref.zoom_in(Reference_modifier::S_array_index { -1 });
    ref.zoom_in(Reference_modifier::S_object_key { phsh_string(::rocket::sref("my_key")) });
    val = ref.read();
    ASTERIA_TEST_CHECK(val.is_real());
    ASTERIA_TEST_CHECK(val.as_real() == 10.5);
    ref.zoom_in(Reference_modifier::S_object_key { phsh_string(::rocket::sref("invalid_access")) });
    ASTERIA_TEST_CHECK_CATCH(val = ref.read());
    ref.zoom_out();

    val = ref.unset();
    ASTERIA_TEST_CHECK(val.is_real());
    ASTERIA_TEST_CHECK(val.as_real() == 10.5);
    val = ref.read();
    ASTERIA_TEST_CHECK(val.is_null());
    val = ref.unset();
    ASTERIA_TEST_CHECK(val.is_null());
  }
