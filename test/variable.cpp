// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/stored_value.hpp"
#include "../src/stored_reference.hpp"
#include "../src/recycler.hpp"
#include <cmath> // NAN

using namespace Asteria;

int main(){
	const auto recycler = std::make_shared<Recycler>();

	Xptr<Variable> var;
	set_variable(var, recycler, true);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	ASTERIA_TEST_CHECK(var->get<D_boolean>() == true);
	ASTERIA_TEST_CHECK_CATCH(var->get<D_string>());
	ASTERIA_TEST_CHECK(var->get_opt<D_double>() == nullptr);

	set_variable(var, recycler, D_integer(42));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_integer);
	ASTERIA_TEST_CHECK(var->get<D_integer>() == 42);

	set_variable(var, recycler, D_double(1.5));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_double);
	ASTERIA_TEST_CHECK(var->get<D_double>() == 1.5);

	set_variable(var, recycler, D_string("hello"));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_string);
	ASTERIA_TEST_CHECK(var->get<D_string>() == "hello");

	D_array array;
	set_variable(var, recycler, D_boolean(true));
	array.emplace_back(std::move(var));
	set_variable(var, recycler, D_string("world"));
	array.emplace_back(std::move(var));
	set_variable(var, recycler, std::move(array));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_array);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(0)->get<D_boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(1)->get<D_string>() == "world");

	D_object object;
	set_variable(var, recycler, D_boolean(true));
	object.emplace("one", std::move(var));
	set_variable(var, recycler, D_string("world"));
	object.emplace("two", std::move(var));
	set_variable(var, recycler, std::move(object));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_object);
	ASTERIA_TEST_CHECK(var->get<D_object>().at("one")->get<D_boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<D_object>().at("two")->get<D_string>() == "world");

	Xptr<Variable> cmp;
	set_variable(var, recycler, D_null());
	set_variable(cmp, recycler, D_null());
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_equal);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_equal);

	set_variable(var, recycler, D_null());
	set_variable(cmp, recycler, D_boolean(true));
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_less);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_greater);

	set_variable(var, recycler, D_boolean(true));
	set_variable(cmp, recycler, D_boolean(true));
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_equal);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_equal);

	set_variable(var, recycler, D_boolean(false));
	set_variable(cmp, recycler, D_boolean(true));
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_less);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_greater);

	set_variable(var, recycler, D_integer(42));
	set_variable(cmp, recycler, D_boolean(true));
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_unordered);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_unordered);

	set_variable(var, recycler, D_integer(5));
	set_variable(cmp, recycler, D_integer(6));
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_less);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_greater);

	set_variable(var, recycler, D_integer(3));
	set_variable(cmp, recycler, D_integer(3));
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_equal);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_equal);

	set_variable(var, recycler, D_double(-2.5));
	set_variable(cmp, recycler, D_double(11.0));
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_less);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_greater);

	set_variable(var, recycler, D_double(1.0));
	set_variable(cmp, recycler, D_double(NAN));
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_unordered);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_unordered);

	set_variable(var, recycler, D_string("hello"));
	set_variable(cmp, recycler, D_string("world"));
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_less);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_greater);

	array.clear();
	set_variable(var, recycler, D_boolean(true));
	array.emplace_back(std::move(var));
	set_variable(var, recycler, D_string("world"));
	array.emplace_back(std::move(var));
	set_variable(var, recycler, std::move(array));
	copy_variable(cmp, recycler, var);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_equal);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_equal);

	var->get<D_array>().at(1)->set(D_string("hello"));
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_less);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_greater);

	var->get<D_array>().at(1)->set(D_boolean(true));
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_unordered);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_unordered);
	var->get<D_array>().erase(std::prev(var->get<D_array>().end()));
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_less);

	object.clear();
	set_variable(var, recycler, D_boolean(true));
	object.emplace("one", std::move(var));
	set_variable(var, recycler, D_string("world"));
	object.emplace("two", std::move(var));
	set_variable(var, recycler, std::move(object));
	copy_variable(cmp, recycler, var);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_unordered);
	swap(var, cmp);
	ASTERIA_TEST_CHECK(compare_variables(var, cmp) == Variable::comparison_result_unordered);
}
