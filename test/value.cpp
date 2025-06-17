// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/value.hpp"
#include <cmath>
using namespace ::asteria;

int main()
  {
    Value value(true);
    ASTERIA_TEST_CHECK(value.is_boolean());
    ASTERIA_TEST_CHECK(value.as_boolean() == true);
    ASTERIA_TEST_CHECK_CATCH(value.as_string());
    ASTERIA_TEST_CHECK(!value.is_real());

    value = V_integer(42);
    ASTERIA_TEST_CHECK(value.is_integer());
    ASTERIA_TEST_CHECK(value.as_integer() == 42);

    value = V_real(1.5);
    ASTERIA_TEST_CHECK(value.is_real());
    ASTERIA_TEST_CHECK(value.as_real() == 1.5);

    value = V_string(&"hello");
    ASTERIA_TEST_CHECK(value.is_string());
    ASTERIA_TEST_CHECK(value.as_string() == "hello");

    V_array array;
    array.emplace_back(V_boolean(true));
    array.emplace_back(V_string("world"));
    value = move(array);
    ASTERIA_TEST_CHECK(value.is_array());
    ASTERIA_TEST_CHECK(value.as_array().at(0).as_boolean() == true);
    ASTERIA_TEST_CHECK(value.as_array().at(1).as_string() == "world");

    V_object object;
    object.try_emplace(&"one", V_boolean(true));
    object.try_emplace(&"two", V_string("world"));
    value = move(object);
    ASTERIA_TEST_CHECK(value.is_object());
    ASTERIA_TEST_CHECK(value.as_object().at(&"one").as_boolean() == true);
    ASTERIA_TEST_CHECK(value.as_object().at(&"two").as_string() == "world");

    value = nullopt;
    Value cmp(nullopt);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_equal);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_equal);

    cmp = V_boolean(true);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_unordered);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_unordered);

    value = V_boolean(true);
    cmp = V_boolean(true);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_equal);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_equal);

    value = V_boolean(false);
    cmp = V_boolean(true);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_less);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_greater);

    value = V_integer(42);
    cmp = V_boolean(true);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_unordered);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_unordered);

    value = V_integer(5);
    cmp = V_integer(6);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_less);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_greater);

    value = V_integer(3);
    cmp = V_integer(3);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_equal);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_equal);

    ASTERIA_TEST_CHECK(value.as_real() == 3.0);
    ASTERIA_TEST_CHECK(value.is_integer());
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_equal);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_equal);

    ASTERIA_TEST_CHECK(value.open_real() == 3.0);
    ASTERIA_TEST_CHECK(value.is_real());
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_equal);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_equal);

    value = V_real(-2.5);
    cmp = V_real(11.0);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_less);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_greater);

    value = V_real(1.0);
    cmp = V_real(NAN);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_unordered);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_unordered);

    value = V_string(&"hello");
    cmp = V_string(&"world");
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_less);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_greater);

    array.clear();
    array.emplace_back(V_boolean(true));
    array.emplace_back(V_string("world"));
    value = array;
    cmp = move(array);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_equal);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_equal);

    value.open_array().mut(1) = V_string(&"hello");
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_less);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_greater);

    value.open_array().mut(1) = V_boolean(true);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_unordered);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_unordered);
    value.open_array().erase(::std::prev(value.as_array().end()));
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_less);

    object.clear();
    object.try_emplace(&"one", V_boolean(true));
    object.try_emplace(&"two", V_string("world"));
    value = move(object);
    cmp = value;
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_unordered);
    swap(value, cmp);
    ASTERIA_TEST_CHECK(value.compare_partial(cmp) == compare_unordered);
  }
