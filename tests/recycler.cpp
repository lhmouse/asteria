// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/recycler.hpp"
#include "../src/variable.hpp"
#include "../src/stored_value.hpp"

using namespace Asteria;

int main(){
	auto recycler = std::make_shared<Recycler>();

	Xptr<Variable> obj;
	recycler->set_variable(obj, D_object());
	auto pair = obj->get<D_object>().emplace("int", std::make_shared<Variable>(D_integer(42)));
	auto weak_int = std::weak_ptr<Variable>(pair.first->second);
	pair = obj->get<D_object>().emplace("str", std::make_shared<Variable>(D_string("hello")));
	auto weak_str = std::weak_ptr<Variable>(pair.first->second);

	recycler->clear_variables();
	ASTERIA_TEST_CHECK(weak_int.expired());
	ASTERIA_TEST_CHECK(weak_str.expired());
}
