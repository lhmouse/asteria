// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/runtime/value.hpp"
#include <cmath>

using namespace Asteria;

int main()
  {
    Value value(true);
    ASTERIA_TEST_CHECK(value.type() == type_boolean);
    ASTERIA_TEST_CHECK(value.check<D_boolean>() == true);
    ASTERIA_TEST_CHECK_CATCH(value.check<D_string>());
    ASTERIA_TEST_CHECK(value.opt<D_real>() == nullptr);

    value = D_integer(42);
    ASTERIA_TEST_CHECK(value.type() == type_integer);
    ASTERIA_TEST_CHECK(value.check<D_integer>() == 42);

    value = D_real(1.5);
    ASTERIA_TEST_CHECK(value.type() == type_real);
    ASTERIA_TEST_CHECK(value.check<D_real>() == 1.5);

    value = D_string(rocket::sref("hello"));
    ASTERIA_TEST_CHECK(value.type() == type_string);
    ASTERIA_TEST_CHECK(value.check<D_string>() == "hello");

    D_array array;
    array.emplace_back(D_boolean(true));
    array.emplace_back(D_string("world"));
    value = std::move(array);
    ASTERIA_TEST_CHECK(value.type() == type_array);
    ASTERIA_TEST_CHECK(value.check<D_array>().at(0).check<D_boolean>() == true);
    ASTERIA_TEST_CHECK(value.check<D_array>().at(1).check<D_string>() == "world");

    D_object object;
    object.try_emplace(PreHashed_String(rocket::sref("one")), D_boolean(true));
    object.try_emplace(PreHashed_String(rocket::sref("two")), D_string("world"));
    value = std::move(object);
    ASTERIA_TEST_CHECK(value.type() == type_object);
    ASTERIA_TEST_CHECK(value.check<D_object>().at(PreHashed_String(rocket::sref("one"))).check<D_boolean>() == true);
    ASTERIA_TEST_CHECK(value.check<D_object>().at(PreHashed_String(rocket::sref("two"))).check<D_string>() == "world");

    value = nullptr;
    Value cmp(nullptr);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);

    cmp = D_boolean(true);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_greater);

    value = D_boolean(true);
    cmp = D_boolean(true);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);

    value = D_boolean(false);
    cmp = D_boolean(true);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_greater);

    value = D_integer(42);
    cmp = D_boolean(true);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);

    value = D_integer(5);
    cmp = D_integer(6);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_greater);

    value = D_integer(3);
    cmp = D_integer(3);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);

    value = D_real(-2.5);
    cmp = D_real(11.0);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_greater);

    value = D_real(1.0);
    cmp = D_real(NAN);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);

    value = D_string(rocket::sref("hello"));
    cmp = D_string(rocket::sref("world"));
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_greater);

    array.clear();
    array.emplace_back(D_boolean(true));
    array.emplace_back(D_string("world"));
    value = array;
    cmp = std::move(array);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);

    value.check<D_array>().mut(1) = D_string(rocket::sref("hello"));
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_greater);

    value.check<D_array>().mut(1) = D_boolean(true);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
    value.check<D_array>().erase(std::prev(value.check<D_array>().end()));
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);

    object.clear();
    object.try_emplace(PreHashed_String(rocket::sref("one")), D_boolean(true));
    object.try_emplace(PreHashed_String(rocket::sref("two")), D_string("world"));
    value = std::move(object);
    cmp = value;
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
  }
