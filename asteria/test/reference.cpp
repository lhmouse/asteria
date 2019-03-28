// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/runtime/reference.hpp"

using namespace Asteria;

int main()
  {
    auto ref = Reference(Reference_Root::S_constant { D_string(rocket::sref("meow")) });
    auto val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == dtype_string);
    ASTERIA_TEST_CHECK(val.check<D_string>() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.open() = D_boolean(true));
    auto ref2 = ref;
    val = ref2.read();
    ASTERIA_TEST_CHECK(val.type() == dtype_string);
    ASTERIA_TEST_CHECK(val.check<D_string>() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.open() = D_boolean(true));

    ref = Reference_Root::S_temporary { D_integer(42) };
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == dtype_integer);
    ASTERIA_TEST_CHECK(val.check<D_integer>() == 42);
    ASTERIA_TEST_CHECK_CATCH(ref.open() = D_boolean(true));
    val = ref2.read();
    ASTERIA_TEST_CHECK(val.type() == dtype_string);
    ASTERIA_TEST_CHECK(val.check<D_string>() == "meow");
    ASTERIA_TEST_CHECK_CATCH(ref.open() = D_boolean(true));

    auto var = rocket::make_refcnt<Variable>(Source_Location(rocket::sref("test"), 42));
    var->reset(Source_Location(rocket::sref("test"), 43), D_null(), false);
    ref = Reference_Root::S_variable { var };
    ref.zoom_in(Reference_Modifier::S_array_index { -3 });
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == dtype_null);
    ref.open() = D_integer(36);
    ref.zoom_out();
    ref.zoom_in(Reference_Modifier::S_array_index { 0 });
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == dtype_integer);
    ASTERIA_TEST_CHECK(val.check<D_integer>() == 36);
    ref.zoom_out();

    ref.zoom_in(Reference_Modifier::S_array_index { 2 });
    ref.zoom_in(Reference_Modifier::S_object_key { PreHashed_String(rocket::sref("my_key")) });
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == dtype_null);
    ref.open() = D_real(10.5);
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == dtype_real);
    ASTERIA_TEST_CHECK(val.check<D_real>() == 10.5);
    ref.zoom_out();
    ref.zoom_out();
    ref.zoom_in(Reference_Modifier::S_array_index { -1 });
    ref.zoom_in(Reference_Modifier::S_object_key { PreHashed_String(rocket::sref("my_key")) });
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == dtype_real);
    ASTERIA_TEST_CHECK(val.check<D_real>() == 10.5);
    ref.zoom_in(Reference_Modifier::S_object_key { PreHashed_String(rocket::sref("invalid_access")) });
    ASTERIA_TEST_CHECK_CATCH(val = ref.read());
    ref.zoom_out();

    val = ref.unset();
    ASTERIA_TEST_CHECK(val.type() == dtype_real);
    ASTERIA_TEST_CHECK(val.check<D_real>() == 10.5);
    val = ref.read();
    ASTERIA_TEST_CHECK(val.type() == dtype_null);
    val = ref.unset();
    ASTERIA_TEST_CHECK(val.type() == dtype_null);
  }
