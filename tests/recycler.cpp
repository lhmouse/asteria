// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/recycler.hpp"
#include "../src/variable.hpp"
#include "../src/stored_value.hpp"

using namespace Asteria;

int main(){
	auto recycler = std::make_shared<Recycler>();

	Xptr<Variable> obj, var;
	set_variable(obj, recycler, D_object());
	set_variable(var, recycler, D_integer(42));
	auto pair = obj->get<D_object>().emplace("int", std::move(var));
	auto weak_int = std::weak_ptr<Variable>(pair.first->second);
	set_variable(var, recycler, D_string("hello"));
	pair = obj->get<D_object>().emplace("str", std::move(var));
	auto weak_str = std::weak_ptr<Variable>(pair.first->second);

	ASTERIA_TEST_CHECK(weak_int.expired() == false);
	ASTERIA_TEST_CHECK(weak_str.expired() == false);
	recycler->purge_variables();
	ASTERIA_TEST_CHECK(weak_int.expired() == true);
	ASTERIA_TEST_CHECK(weak_str.expired() == true);
}
