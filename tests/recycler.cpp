// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/recycler.hpp"
#include "../src/variable.hpp"
#include "../src/stored_value.hpp"

using namespace Asteria;

int main(){
	auto recycler = std::make_shared<Recycler>();

	auto nested_int = std::make_shared<Variable>(Integer(42));
	ASTERIA_TEST_CHECK(nested_int->get_type() == Variable::type_integer);
	ASTERIA_TEST_CHECK(nested_int->get<Integer>() == 42);
	auto nested_str = std::make_shared<Variable>(String("hello"));
	ASTERIA_TEST_CHECK(nested_str->get_type() == Variable::type_string);
	ASTERIA_TEST_CHECK(nested_str->get<String>() == "hello");

	auto obj = recycler->create_variable_opt(Object());
	obj->get<Object>().emplace("int", std::shared_ptr<Variable>(nested_int));
	obj->get<Object>().emplace("str", std::shared_ptr<Variable>(nested_str));

	recycler->clear_variables();
	ASTERIA_TEST_CHECK(nested_int->get_type() == Variable::type_boolean);
	ASTERIA_TEST_CHECK(nested_int->get<Boolean>() == false);
	ASTERIA_TEST_CHECK(nested_str->get_type() == Variable::type_boolean);
	ASTERIA_TEST_CHECK(nested_str->get<Boolean>() == false);
}
