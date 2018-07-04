// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/value.hpp"
#include "../src/stored_reference.hpp"
#include <cmath> // NAN

using namespace Asteria;

int main(){
	Vp<Value> value;
	set_value(value, true);
	ASTERIA_TEST_CHECK(value->get_type() == Value::type_boolean);
	ASTERIA_TEST_CHECK(value->get<D_boolean>() == true);
	ASTERIA_TEST_CHECK_CATCH(value->get<D_string>());
	ASTERIA_TEST_CHECK(value->get_opt<D_double>() == nullptr);

	set_value(value, D_integer(42));
	ASTERIA_TEST_CHECK(value->get_type() == Value::type_integer);
	ASTERIA_TEST_CHECK(value->get<D_integer>() == 42);

	set_value(value, D_double(1.5));
	ASTERIA_TEST_CHECK(value->get_type() == Value::type_double);
	ASTERIA_TEST_CHECK(value->get<D_double>() == 1.5);

	set_value(value, D_string(D_string::shallow("hello")));
	ASTERIA_TEST_CHECK(value->get_type() == Value::type_string);
	ASTERIA_TEST_CHECK(value->get<D_string>() == D_string::shallow("hello"));

	D_array array;
	set_value(value, D_boolean(true));
	array.emplace_back(std::move(value));
	set_value(value, D_string(D_string::shallow("world")));
	array.emplace_back(std::move(value));
	set_value(value, std::move(array));
	ASTERIA_TEST_CHECK(value->get_type() == Value::type_array);
	ASTERIA_TEST_CHECK(value->get<D_array>().at(0)->get<D_boolean>() == true);
	ASTERIA_TEST_CHECK(value->get<D_array>().at(1)->get<D_string>() == D_string::shallow("world"));

	D_object object;
	set_value(value, D_boolean(true));
	object.emplace(D_string::shallow("one"), std::move(value));
	set_value(value, D_string(D_string::shallow("world")));
	object.emplace(D_string::shallow("two"), std::move(value));
	set_value(value, std::move(object));
	ASTERIA_TEST_CHECK(value->get_type() == Value::type_object);
	ASTERIA_TEST_CHECK(value->get<D_object>().at(D_string::shallow("one"))->get<D_boolean>() == true);
	ASTERIA_TEST_CHECK(value->get<D_object>().at(D_string::shallow("two"))->get<D_string>() == D_string::shallow("world"));

	Vp<Value> cmp;
	clear_value(value);
	clear_value(cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_equal);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_equal);

	clear_value(value);
	set_value(cmp, D_boolean(true));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_less);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_greater);

	set_value(value, D_boolean(true));
	set_value(cmp, D_boolean(true));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_equal);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_equal);

	set_value(value, D_boolean(false));
	set_value(cmp, D_boolean(true));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_less);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_greater);

	set_value(value, D_integer(42));
	set_value(cmp, D_boolean(true));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_unordered);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_unordered);

	set_value(value, D_integer(5));
	set_value(cmp, D_integer(6));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_less);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_greater);

	set_value(value, D_integer(3));
	set_value(cmp, D_integer(3));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_equal);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_equal);

	set_value(value, D_double(-2.5));
	set_value(cmp, D_double(11.0));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_less);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_greater);

	set_value(value, D_double(1.0));
	set_value(cmp, D_double(NAN));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_unordered);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_unordered);

	set_value(value, D_string(D_string::shallow("hello")));
	set_value(cmp, D_string(D_string::shallow("world")));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_less);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_greater);

	array.clear();
	set_value(value, D_boolean(true));
	array.emplace_back(std::move(value));
	set_value(value, D_string(D_string::shallow("world")));
	array.emplace_back(std::move(value));
	set_value(value, std::move(array));
	copy_value(cmp, value);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_equal);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_equal);

	value->get<D_array>().at(1)->set(D_string(D_string::shallow("hello")));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_less);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_greater);

	value->get<D_array>().at(1)->set(D_boolean(true));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_unordered);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_unordered);
	value->get<D_array>().erase(std::prev(value->get<D_array>().end()));
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_less);

	object.clear();
	set_value(value, D_boolean(true));
	object.emplace(D_string::shallow("one"), std::move(value));
	set_value(value, D_string(D_string::shallow("world")));
	object.emplace(D_string::shallow("two"), std::move(value));
	set_value(value, std::move(object));
	copy_value(cmp, value);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_unordered);
	swap(value, cmp);
	ASTERIA_TEST_CHECK(compare_values(value, cmp) == Value::comparison_result_unordered);
}
