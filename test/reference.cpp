// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/reference.hpp"
#include "../src/variable.hpp"

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

    auto var = materialize_reference(ref);
    ASTERIA_TEST_CHECK(var->get_value().which() == Value::type_integer);
    ASTERIA_TEST_CHECK(var->get_value().as<D_integer>() == 42);
    ASTERIA_TEST_CHECK(ref.get_root().which() == Reference_root::type_variable);
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.which() == Value::type_integer);
    ASTERIA_TEST_CHECK(val.as<D_integer>() == 42);
    write_reference(ref, D_boolean(true));
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.which() == Value::type_boolean);
    ASTERIA_TEST_CHECK(val.as<D_boolean>() == true);

    ref.set_root(Reference_root::S_temporary_value { D_null() });
    var = materialize_reference(ref);
    ASTERIA_TEST_CHECK(var->get_value().which() == Value::type_null);
    ref.push_member_designator(Reference_member_designator::S_array{ -3 });
    val = read_reference(ref);
    ASTERIA_TEST_CHECK(val.which() == Value::type_null);
    ASTERIA_TEST_CHECK(var->get_value().which() == Value::type_null);
    write_reference(ref, D_integer(36));
    ASTERIA_TEST_CHECK(var->get_value().which() == Value::type_array);
    ASTERIA_TEST_CHECK(var->get_value().as<D_array>().size() == 3);
    ASTERIA_TEST_CHECK(var->get_value().as<D_array>().at(0).which() == Value::type_integer);
    ASTERIA_TEST_CHECK(var->get_value().as<D_array>().at(0).as<D_integer>() == 36);
    ASTERIA_TEST_CHECK(var->get_value().as<D_array>().at(1).which() == Value::type_null);
    ASTERIA_TEST_CHECK(var->get_value().as<D_array>().at(2).which() == Value::type_null);
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
    ASTERIA_TEST_CHECK(var->get_value().which() == Value::type_array);
    ASTERIA_TEST_CHECK(var->get_value().as<D_array>().size() == 3);
    write_reference(ref, D_double(10.5));
    ASTERIA_TEST_CHECK(var->get_value().which() == Value::type_array);
    ASTERIA_TEST_CHECK(var->get_value().as<D_array>().size() == 3);
    ASTERIA_TEST_CHECK(var->get_value().as<D_array>().at(0).which() == Value::type_integer);
    ASTERIA_TEST_CHECK(var->get_value().as<D_array>().at(0).as<D_integer>() == 36);
    ASTERIA_TEST_CHECK(var->get_value().as<D_array>().at(1).which() == Value::type_null);
    ASTERIA_TEST_CHECK(var->get_value().as<D_array>().at(2).which() == Value::type_object);
    ASTERIA_TEST_CHECK(var->get_value().as<D_array>().at(2).as<D_object>().at(String::shallow("my_key")).as<D_double>() == 10.5);
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
