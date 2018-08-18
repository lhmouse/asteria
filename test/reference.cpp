// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/reference.hpp"

using namespace Asteria;

int main()
  {
    Reference ref(Reference_root::S_constant { D_string(String::shallow("meow")) });
    auto val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.which() == Value::type_string);
    ASTERIA_TEST_CHECK(val.as<D_string>() == "meow");
    ASTERIA_TEST_CHECK_CATCH(write_reference(ref, D_boolean(true)));

    ref.set_root(Reference_root::S_temporary_value { D_integer(42) });
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.which() == Value::type_integer);
    ASTERIA_TEST_CHECK(val.as<D_integer>() == 42);
    ASTERIA_TEST_CHECK_CATCH(write_reference(ref, D_boolean(true)));

    materialize_reference(ref);
    ASTERIA_TEST_CHECK(ref.get_root().which() == Reference_root::type_variable);
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.which() == Value::type_integer);
    ASTERIA_TEST_CHECK(val.as<D_integer>() == 42);
    write_reference(ref, D_boolean(true));
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.which() == Value::type_boolean);
    ASTERIA_TEST_CHECK(val.as<D_boolean>() == true);

    ref.set_root(Reference_root::S_temporary_value { D_null() });
    materialize_reference(ref);
    ref.push_member_designator(Reference_member_designator::S_array{ -3 });
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.which() == Value::type_null);
    write_reference(ref, D_integer(36));
    ref.pop_member_designator();
    ref.push_member_designator(Reference_member_designator::S_array{ 0 });
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.which() == Value::type_integer);
    ASTERIA_TEST_CHECK(val.as<D_integer>() == 36);

    ref.clear_member_designators();
    ref.push_member_designator(Reference_member_designator::S_array{ 2 });
    ref.push_member_designator(Reference_member_designator::S_object{ String::shallow("my_key") });
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.which() == Value::type_null);
    write_reference(ref, D_double(10.5));
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.which() == Value::type_double);
    ASTERIA_TEST_CHECK(val.as<D_double>() == 10.5);
    ref.clear_member_designators();
    ref.push_member_designator(Reference_member_designator::S_array{ -1 });
    ref.push_member_designator(Reference_member_designator::S_object{ String::shallow("my_key") });
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.which() == Value::type_double);
    ASTERIA_TEST_CHECK(val.as<D_double>() == 10.5);
    ref.push_member_designator(Reference_member_designator::S_object{ String::shallow("invalid_access") });
    ASTERIA_TEST_CHECK_CATCH(val = read_reference(ref));
  }
