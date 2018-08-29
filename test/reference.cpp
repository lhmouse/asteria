// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/reference.hpp"

using namespace Asteria;

int main()
  {
    auto ref = Reference(Reference_root::S_constant { D_string(String::shallow("meow")) });
    auto val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.type() == Value::type_string);
    ASTERIA_TEST_CHECK(val.check<D_string>() == "meow");
    ASTERIA_TEST_CHECK_CATCH(write_reference(ref, D_boolean(true)));
    auto ref2 = ref;
    val = read_reference(ref2);
    ASTERIA_TEST_CHECK(val.type() == Value::type_string);
    ASTERIA_TEST_CHECK(val.check<D_string>() == "meow");
    ASTERIA_TEST_CHECK_CATCH(write_reference(ref, D_boolean(true)));

    ref.set_root(Reference_root::S_temporary { D_integer(42) });
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.type() == Value::type_integer);
    ASTERIA_TEST_CHECK(val.check<D_integer>() == 42);
    ASTERIA_TEST_CHECK_CATCH(write_reference(ref, D_boolean(true)));
    val = read_reference(ref2);
    ASTERIA_TEST_CHECK(val.type() == Value::type_string);
    ASTERIA_TEST_CHECK(val.check<D_string>() == "meow");
    ASTERIA_TEST_CHECK_CATCH(write_reference(ref, D_boolean(true)));

    materialize_reference(ref);
    ASTERIA_TEST_CHECK(ref.get_root().is_lvalue());
    ASTERIA_TEST_CHECK(ref.get_root().is_unique());
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.type() == Value::type_integer);
    ASTERIA_TEST_CHECK(val.check<D_integer>() == 42);
    write_reference(ref, D_boolean(true));
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.type() == Value::type_boolean);
    ASTERIA_TEST_CHECK(val.check<D_boolean>() == true);

    ref.set_root(Reference_root::S_temporary { D_null() });
    materialize_reference(ref);
    ref.push_modifier(Reference_modifier::S_array_index { -3 });
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.type() == Value::type_null);
    write_reference(ref, D_integer(36));
    ref.pop_modifier();
    ref.push_modifier(Reference_modifier::S_array_index { 0 });
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.type() == Value::type_integer);
    ASTERIA_TEST_CHECK(val.check<D_integer>() == 36);

    ref.clear_modifiers();
    ref.push_modifier(Reference_modifier::S_array_index { 2 });
    ref.push_modifier(Reference_modifier::S_object_key { String::shallow("my_key") });
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.type() == Value::type_null);
    write_reference(ref, D_double(10.5));
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.type() == Value::type_double);
    ASTERIA_TEST_CHECK(val.check<D_double>() == 10.5);
    ref.clear_modifiers();
    ref.push_modifier(Reference_modifier::S_array_index { -1 });
    ref.push_modifier(Reference_modifier::S_object_key { String::shallow("my_key") });
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.type() == Value::type_double);
    ASTERIA_TEST_CHECK(val.check<D_double>() == 10.5);
    ref.push_modifier(Reference_modifier::S_object_key { String::shallow("invalid_access") });
    ASTERIA_TEST_CHECK_CATCH(val = read_reference(ref));

    ref.pop_modifier();
    val = unset_reference(ref);
    ASTERIA_TEST_CHECK(val.type() == Value::type_double);
    ASTERIA_TEST_CHECK(val.check<D_double>() == 10.5);
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.type() == Value::type_null);
    val = unset_reference(ref);
    ASTERIA_TEST_CHECK(val.type() == Value::type_null);
  }
