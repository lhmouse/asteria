// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/recycler.hpp"
#include "../src/variable.hpp"
#include "../src/stored_value.hpp"

using namespace Asteria;

int main(){
	auto recycler = create_shared<Recycler>();

	Value_ptr<Variable> obj;
	recycler->set_variable(obj, Object());
	auto pair = obj->get<Object>().emplace("int", create_shared<Variable>(Integer(42)));
	auto weak_int = std::weak_ptr<Variable>(pair.first->second);
	pair = obj->get<Object>().emplace("str", create_shared<Variable>(String("hello")));
	auto weak_str = std::weak_ptr<Variable>(pair.first->second);

	recycler->clear_variables();
	ASTERIA_TEST_CHECK(weak_int.expired());
	ASTERIA_TEST_CHECK(weak_str.expired());
}
