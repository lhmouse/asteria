// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/value.hpp"

using namespace Asteria;

int main()
  {
    Value value(true);
    ASTERIA_TEST_CHECK(value.type() == Value::type_boolean);
    ASTERIA_TEST_CHECK(value.as<D_boolean>() == true);
    ASTERIA_TEST_CHECK_CATCH(value.as<D_string>());
    ASTERIA_TEST_CHECK(value.opt<D_double>() == nullptr);

    value.set(D_integer(42));
    ASTERIA_TEST_CHECK(value.type() == Value::type_integer);
    ASTERIA_TEST_CHECK(value.as<D_integer>() == 42);

    value.set(D_double(1.5));
    ASTERIA_TEST_CHECK(value.type() == Value::type_double);
    ASTERIA_TEST_CHECK(value.as<D_double>() == 1.5);

    value.set(D_string(String::shallow("hello")));
    ASTERIA_TEST_CHECK(value.type() == Value::type_string);
    ASTERIA_TEST_CHECK(value.as<D_string>() == String::shallow("hello"));

    D_array array;
    array.emplace_back(D_boolean(true));
    array.emplace_back(D_string("world"));
    value.set(std::move(array));
    ASTERIA_TEST_CHECK(value.type() == Value::type_array);
    ASTERIA_TEST_CHECK(value.as<D_array>().at(0).as<D_boolean>() == true);
    ASTERIA_TEST_CHECK(value.as<D_array>().at(1).as<D_string>() == String::shallow("world"));

    D_object object;
    object.try_emplace(String::shallow("one"), D_boolean(true));
    object.try_emplace(String::shallow("two"), D_string("world"));
    value.set(std::move(object));
    ASTERIA_TEST_CHECK(value.type() == Value::type_object);
    ASTERIA_TEST_CHECK(value.as<D_object>().at(String::shallow("one")).as<D_boolean>() == true);
    ASTERIA_TEST_CHECK(value.as<D_object>().at(String::shallow("two")).as<D_string>() == String::shallow("world"));

    value.set(nullptr);
    Value cmp(nullptr);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_equal);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_equal);

    cmp.set(D_boolean(true));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_greater);

    value.set(D_boolean(true));
    cmp.set(D_boolean(true));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_equal);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_equal);

    value.set(D_boolean(false));
    cmp.set(D_boolean(true));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_greater);

    value.set(D_integer(42));
    cmp.set(D_boolean(true));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_unordered);

    value.set(D_integer(5));
    cmp.set(D_integer(6));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_greater);

    value.set(D_integer(3));
    cmp.set(D_integer(3));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_equal);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_equal);

    value.set(D_double(-2.5));
    cmp.set(D_double(11.0));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_greater);

    value.set(D_double(1.0));
    cmp.set(D_double(NAN));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_unordered);

    value.set(D_string(String::shallow("hello")));
    cmp.set(D_string(String::shallow("world")));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_greater);

    array.clear();
    array.emplace_back(D_boolean(true));
    array.emplace_back(D_string("world"));
    value.set(array);
    cmp.set(std::move(array));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_equal);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_equal);

    value.as<D_array>().mut(1).set(D_string(String::shallow("hello")));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_greater);

    value.as<D_array>().mut(1).set(D_boolean(true));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_unordered);
    value.as<D_array>().erase(std::prev(value.as<D_array>().end()));
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_less);

    object.clear();
    object.try_emplace(String::shallow("one"), D_boolean(true));
    object.try_emplace(String::shallow("two"), D_string("world"));
    value.set(std::move(object));
    cmp = value;
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::compare_unordered);
  }
