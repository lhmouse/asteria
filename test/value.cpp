// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/value.hpp"

using namespace Asteria;

int main(){
	Value value(true);
	ASTERIA_TEST_CHECK(value.which() == Value::type_boolean);
	ASTERIA_TEST_CHECK(value.as<D_boolean>() == true);
	ASTERIA_TEST_CHECK_CATCH(value.as<D_string>());
	ASTERIA_TEST_CHECK(value.get_opt<D_double>() == nullptr);

	value.set(D_integer(42));
	ASTERIA_TEST_CHECK(value.which() == Value::type_integer);
	ASTERIA_TEST_CHECK(value.as<D_integer>() == 42);

	value.set(D_double(1.5));
	ASTERIA_TEST_CHECK(value.which() == Value::type_double);
	ASTERIA_TEST_CHECK(value.as<D_double>() == 1.5);

	value.set(D_string(D_string::shallow("hello")));
	ASTERIA_TEST_CHECK(value.which() == Value::type_string);
	ASTERIA_TEST_CHECK(value.as<D_string>() == D_string::shallow("hello"));

	D_array array;
	array.emplace_back(D_boolean(true));
	array.emplace_back(D_string("world"));
	value.set(std::move(array));
	ASTERIA_TEST_CHECK(value.which() == Value::type_array);
	ASTERIA_TEST_CHECK(value.as<D_array>().at(0).as<D_boolean>() == true);
	ASTERIA_TEST_CHECK(value.as<D_array>().at(1).as<D_string>() == D_string::shallow("world"));

	D_object object;
	object.emplace(D_string::shallow("one"), D_boolean(true));
	object.emplace(D_string::shallow("two"), D_string("world"));
	value.set(std::move(object));
	ASTERIA_TEST_CHECK(value.which() == Value::type_object);
	ASTERIA_TEST_CHECK(value.as<D_object>().at(D_string::shallow("one")).as<D_boolean>() == true);
	ASTERIA_TEST_CHECK(value.as<D_object>().at(D_string::shallow("two")).as<D_string>() == D_string::shallow("world"));

	value.set(nullptr);
	Value cmp(nullptr);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_equal);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_equal);

	cmp.set(D_boolean(true));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_less);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_greater);

	value.set(D_boolean(true));
	cmp.set(D_boolean(true));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_equal);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_equal);

	value.set(D_boolean(false));
	cmp.set(D_boolean(true));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_less);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_greater);

	value.set(D_integer(42));
	cmp.set(D_boolean(true));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_unordered);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_unordered);

	value.set(D_integer(5));
	cmp.set(D_integer(6));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_less);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_greater);

	value.set(D_integer(3));
	cmp.set(D_integer(3));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_equal);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_equal);

	value.set(D_double(-2.5));
	cmp.set(D_double(11.0));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_less);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_greater);

	value.set(D_double(1.0));
	cmp.set(D_double(NAN));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_unordered);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_unordered);

	value.set(D_string(D_string::shallow("hello")));
	cmp.set(D_string(D_string::shallow("world")));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_less);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_greater);

	array.clear();
	array.emplace_back(D_boolean(true));
	array.emplace_back(D_string("world"));
	value.set(array);
	cmp.set(std::move(array));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_equal);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_equal);

	value.as<D_array>().at(1).set(D_string(D_string::shallow("hello")));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_less);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_greater);

	value.as<D_array>().at(1).set(D_boolean(true));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_unordered);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_unordered);
	value.as<D_array>().erase(std::prev(value.as<D_array>().end()));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_less);

	object.clear();
	object.emplace(D_string::shallow("one"), D_boolean(true));
	object.emplace(D_string::shallow("two"), D_string("world"));
	value.set(std::move(object));
	cmp = value;
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_unordered);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_unordered);
}
