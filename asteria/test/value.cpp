// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/value.hpp"
#include <cmath>

using namespace Asteria;

int main()
  {
    Value value(true);
    ASTERIA_TEST_CHECK(value.is_boolean());
    ASTERIA_TEST_CHECK(value.as_boolean() == true);
    ASTERIA_TEST_CHECK_CATCH(value.as_string());
    ASTERIA_TEST_CHECK(!value.is_real());

    value = G_integer(42);
    ASTERIA_TEST_CHECK(value.is_integer());
    ASTERIA_TEST_CHECK(value.as_integer() == 42);

    value = G_real(1.5);
    ASTERIA_TEST_CHECK(value.is_real());
    ASTERIA_TEST_CHECK(value.as_real() == 1.5);

    value = G_string(rocket::sref("hello"));
    ASTERIA_TEST_CHECK(value.is_string());
    ASTERIA_TEST_CHECK(value.as_string() == "hello");

    G_array array;
    array.emplace_back(G_boolean(true));
    array.emplace_back(G_string("world"));
    value = std::move(array);
    ASTERIA_TEST_CHECK(value.is_array());
    ASTERIA_TEST_CHECK(value.as_array().at(0).as_boolean() == true);
    ASTERIA_TEST_CHECK(value.as_array().at(1).as_string() == "world");

    G_object object;
    object.try_emplace(PreHashed_String(rocket::sref("one")), G_boolean(true));
    object.try_emplace(PreHashed_String(rocket::sref("two")), G_string("world"));
    value = std::move(object);
    ASTERIA_TEST_CHECK(value.is_object());
    ASTERIA_TEST_CHECK(value.as_object().at(PreHashed_String(rocket::sref("one"))).as_boolean() == true);
    ASTERIA_TEST_CHECK(value.as_object().at(PreHashed_String(rocket::sref("two"))).as_string() == "world");

    value = nullptr;
    Value cmp(nullptr);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);

    cmp = G_boolean(true);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);

    value = G_boolean(true);
    cmp = G_boolean(true);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);

    value = G_boolean(false);
    cmp = G_boolean(true);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_greater);

    value = G_integer(42);
    cmp = G_boolean(true);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);

    value = G_integer(5);
    cmp = G_integer(6);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_greater);

    value = G_integer(3);
    cmp = G_integer(3);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);

    value = G_real(-2.5);
    cmp = G_real(11.0);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_greater);

    value = G_real(1.0);
    cmp = G_real(NAN);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);

    value = G_string(rocket::sref("hello"));
    cmp = G_string(rocket::sref("world"));
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_greater);

    array.clear();
    array.emplace_back(G_boolean(true));
    array.emplace_back(G_string("world"));
    value = array;
    cmp = std::move(array);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_equal);

    value.mut_array().mut(1) = G_string(rocket::sref("hello"));
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_greater);

    value.mut_array().mut(1) = G_boolean(true);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
    value.mut_array().erase(std::prev(value.as_array().end()));
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_less);

    object.clear();
    object.try_emplace(PreHashed_String(rocket::sref("one")), G_boolean(true));
    object.try_emplace(PreHashed_String(rocket::sref("two")), G_string("world"));
    value = std::move(object);
    cmp = value;
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
    std::swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare(cmp) == Value::compare_unordered);
  }
