// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/recycler.hpp"
#include "../src/stored_value.hpp"

using namespace Asteria;

int main(){
	auto recycler = std::make_shared<Recycler>();

	Xptr<Variable> obj, var;
	set_variable(obj, recycler, D_object());
	set_variable(var, recycler, D_integer(42));
	auto pair = obj->get<D_object>().emplace(D_string::shallow("int"), std::move(var));
	auto weak_int = std::weak_ptr<Variable>(pair.first->second);
	set_variable(var, recycler, D_string(D_string::shallow("hello")));
	pair = obj->get<D_object>().emplace(D_string::shallow("str"), std::move(var));
	auto weak_str = std::weak_ptr<Variable>(pair.first->second);

	ASTERIA_TEST_CHECK(weak_int.expired() == false);
	ASTERIA_TEST_CHECK(weak_str.expired() == false);
	recycler.reset();
	ASTERIA_TEST_CHECK(weak_int.expired() == true);
	ASTERIA_TEST_CHECK(weak_str.expired() == true);
}
